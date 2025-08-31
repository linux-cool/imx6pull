/*
 * USB Camera Driver for IMX6ULL Pro
 * 
 * This driver provides V4L2 interface for USB UVC cameras
 * Optimized for IMX6ULL Pro platform with limited resources
 * 
 * Author: Camera Driver Team
 * License: GPL v2
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#define DRIVER_NAME "imx6ull_camera"
#define DRIVER_VERSION "1.0.0"

/* Maximum number of buffers */
#define MAX_BUFFERS 4
#define MIN_BUFFERS 2

/* Supported formats */
#define CAMERA_FORMAT_MJPEG V4L2_PIX_FMT_MJPEG
#define CAMERA_FORMAT_YUYV  V4L2_PIX_FMT_YUYV

/* Device states */
enum camera_state {
    CAMERA_STATE_DISCONNECTED = 0,
    CAMERA_STATE_CONNECTED,
    CAMERA_STATE_STREAMING,
    CAMERA_STATE_ERROR
};

/* Buffer structure */
struct camera_buffer {
    struct vb2_v4l2_buffer vb;
    struct list_head list;
    void *vaddr;
    size_t size;
};

/* Main device structure */
struct camera_device {
    struct v4l2_device v4l2_dev;
    struct video_device *vdev;
    struct usb_device *udev;
    struct usb_interface *intf;
    
    /* Device state */
    enum camera_state state;
    struct mutex lock;
    
    /* Video buffer queue */
    struct vb2_queue queue;
    struct list_head buf_list;
    spinlock_t buf_lock;
    
    /* Format information */
    struct v4l2_format format;
    struct v4l2_streamparm parm;
    
    /* USB streaming */
    struct usb_host_endpoint *streaming_ep;
    struct urb *urbs[MAX_BUFFERS];
    int num_urbs;
    
    /* Statistics */
    atomic_t frames_received;
    atomic_t frames_dropped;
    u64 last_frame_time;
};

/* Supported pixel formats */
static struct v4l2_fmtdesc formats[] = {
    {
        .index = 0,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .flags = V4L2_FMT_FLAG_COMPRESSED,
        .description = "Motion-JPEG",
        .pixelformat = V4L2_PIX_FMT_MJPEG,
    },
    {
        .index = 1,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .flags = 0,
        .description = "YUYV 4:2:2",
        .pixelformat = V4L2_PIX_FMT_YUYV,
    }
};

/* Supported frame sizes */
static struct v4l2_frmsizeenum frame_sizes[] = {
    { .index = 0, .pixel_format = V4L2_PIX_FMT_MJPEG,
      .type = V4L2_FRMSIZE_TYPE_DISCRETE,
      .discrete = { .width = 640, .height = 480 } },
    { .index = 1, .pixel_format = V4L2_PIX_FMT_MJPEG,
      .type = V4L2_FRMSIZE_TYPE_DISCRETE,
      .discrete = { .width = 1280, .height = 720 } },
    { .index = 0, .pixel_format = V4L2_PIX_FMT_YUYV,
      .type = V4L2_FRMSIZE_TYPE_DISCRETE,
      .discrete = { .width = 640, .height = 480 } },
};

/* USB device ID table */
static struct usb_device_id camera_id_table[] = {
    { USB_INTERFACE_INFO(USB_CLASS_VIDEO, 1, 0) }, /* UVC VideoControl */
    { USB_INTERFACE_INFO(USB_CLASS_VIDEO, 2, 0) }, /* UVC VideoStreaming */
    { }
};
MODULE_DEVICE_TABLE(usb, camera_id_table);

/* Forward declarations */
static int camera_probe(struct usb_interface *intf, const struct usb_device_id *id);
static void camera_disconnect(struct usb_interface *intf);

/* USB driver structure */
static struct usb_driver camera_driver = {
    .name = DRIVER_NAME,
    .probe = camera_probe,
    .disconnect = camera_disconnect,
    .id_table = camera_id_table,
};

/*
 * V4L2 Operations
 */

/* Query device capabilities */
static int camera_querycap(struct file *file, void *priv,
                          struct v4l2_capability *cap)
{
    struct camera_device *dev = video_drvdata(file);
    
    strscpy(cap->driver, DRIVER_NAME, sizeof(cap->driver));
    strscpy(cap->card, "IMX6ULL Camera", sizeof(cap->card));
    usb_make_path(dev->udev, cap->bus_info, sizeof(cap->bus_info));
    
    cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
    cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
    
    return 0;
}

/* Enumerate supported formats */
static int camera_enum_fmt(struct file *file, void *priv,
                          struct v4l2_fmtdesc *f)
{
    if (f->index >= ARRAY_SIZE(formats))
        return -EINVAL;
    
    *f = formats[f->index];
    return 0;
}

/* Get current format */
static int camera_g_fmt(struct file *file, void *priv,
                       struct v4l2_format *f)
{
    struct camera_device *dev = video_drvdata(file);
    
    *f = dev->format;
    return 0;
}

/* Set format */
static int camera_s_fmt(struct file *file, void *priv,
                       struct v4l2_format *f)
{
    struct camera_device *dev = video_drvdata(file);
    int ret;
    
    if (vb2_is_busy(&dev->queue))
        return -EBUSY;
    
    ret = camera_try_fmt(file, priv, f);
    if (ret)
        return ret;
    
    dev->format = *f;
    return 0;
}

/* Try format */
static int camera_try_fmt(struct file *file, void *priv,
                         struct v4l2_format *f)
{
    struct v4l2_pix_format *pix = &f->fmt.pix;
    
    /* Validate pixel format */
    if (pix->pixelformat != V4L2_PIX_FMT_MJPEG &&
        pix->pixelformat != V4L2_PIX_FMT_YUYV) {
        pix->pixelformat = V4L2_PIX_FMT_MJPEG;
    }
    
    /* Clamp dimensions */
    pix->width = clamp(pix->width, 160U, 1280U);
    pix->height = clamp(pix->height, 120U, 720U);
    
    /* Align to 16-byte boundary for better performance */
    pix->width = ALIGN(pix->width, 16);
    pix->height = ALIGN(pix->height, 2);
    
    /* Calculate bytes per line and image size */
    if (pix->pixelformat == V4L2_PIX_FMT_YUYV) {
        pix->bytesperline = pix->width * 2;
        pix->sizeimage = pix->bytesperline * pix->height;
    } else {
        pix->bytesperline = 0; /* Compressed format */
        pix->sizeimage = pix->width * pix->height; /* Estimate */
    }
    
    pix->field = V4L2_FIELD_NONE;
    pix->colorspace = V4L2_COLORSPACE_SRGB;
    
    return 0;
}

/*
 * Videobuf2 Operations
 */

/* Queue setup */
static int camera_queue_setup(struct vb2_queue *q,
                             unsigned int *nbuffers,
                             unsigned int *nplanes,
                             unsigned int sizes[],
                             struct device *alloc_devs[])
{
    struct camera_device *dev = vb2_get_drv_priv(q);
    unsigned int size = dev->format.fmt.pix.sizeimage;
    
    if (*nplanes)
        return sizes[0] < size ? -EINVAL : 0;
    
    *nplanes = 1;
    sizes[0] = size;
    
    /* Ensure minimum number of buffers */
    if (*nbuffers < MIN_BUFFERS)
        *nbuffers = MIN_BUFFERS;
    
    /* Limit maximum buffers for memory conservation */
    if (*nbuffers > MAX_BUFFERS)
        *nbuffers = MAX_BUFFERS;
    
    return 0;
}

/* Buffer preparation */
static int camera_buf_prepare(struct vb2_buffer *vb)
{
    struct camera_device *dev = vb2_get_drv_priv(vb->vb2_queue);
    unsigned long size = dev->format.fmt.pix.sizeimage;
    
    if (vb2_plane_size(vb, 0) < size) {
        dev_err(&dev->udev->dev, "Buffer too small (%lu < %lu)\n",
                vb2_plane_size(vb, 0), size);
        return -EINVAL;
    }
    
    vb2_set_plane_payload(vb, 0, size);
    return 0;
}

/* Buffer queue */
static void camera_buf_queue(struct vb2_buffer *vb)
{
    struct camera_device *dev = vb2_get_drv_priv(vb->vb2_queue);
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
    struct camera_buffer *buf = container_of(vbuf, struct camera_buffer, vb);
    unsigned long flags;
    
    spin_lock_irqsave(&dev->buf_lock, flags);
    list_add_tail(&buf->list, &dev->buf_list);
    spin_unlock_irqrestore(&dev->buf_lock, flags);
}

/* Start streaming */
static int camera_start_streaming(struct vb2_queue *q, unsigned int count)
{
    struct camera_device *dev = vb2_get_drv_priv(q);
    int ret;
    
    dev_info(&dev->udev->dev, "Starting video stream\n");
    
    /* Initialize USB streaming */
    ret = camera_init_streaming(dev);
    if (ret) {
        dev_err(&dev->udev->dev, "Failed to start streaming: %d\n", ret);
        return ret;
    }
    
    dev->state = CAMERA_STATE_STREAMING;
    atomic_set(&dev->frames_received, 0);
    atomic_set(&dev->frames_dropped, 0);
    
    return 0;
}

/* Stop streaming */
static void camera_stop_streaming(struct vb2_queue *q)
{
    struct camera_device *dev = vb2_get_drv_priv(q);
    
    dev_info(&dev->udev->dev, "Stopping video stream\n");
    
    /* Stop USB streaming */
    camera_stop_usb_streaming(dev);
    
    /* Return all buffers */
    camera_return_all_buffers(dev, VB2_BUF_STATE_ERROR);
    
    dev->state = CAMERA_STATE_CONNECTED;
}

/* Videobuf2 operations structure */
static struct vb2_ops camera_vb2_ops = {
    .queue_setup = camera_queue_setup,
    .buf_prepare = camera_buf_prepare,
    .buf_queue = camera_buf_queue,
    .start_streaming = camera_start_streaming,
    .stop_streaming = camera_stop_streaming,
    .wait_prepare = vb2_ops_wait_prepare,
    .wait_finish = vb2_ops_wait_finish,
};

/* V4L2 ioctl operations */
static const struct v4l2_ioctl_ops camera_ioctl_ops = {
    .vidioc_querycap = camera_querycap,
    .vidioc_enum_fmt_vid_cap = camera_enum_fmt,
    .vidioc_g_fmt_vid_cap = camera_g_fmt,
    .vidioc_s_fmt_vid_cap = camera_s_fmt,
    .vidioc_try_fmt_vid_cap = camera_try_fmt,
    
    .vidioc_reqbufs = vb2_ioctl_reqbufs,
    .vidioc_querybuf = vb2_ioctl_querybuf,
    .vidioc_qbuf = vb2_ioctl_qbuf,
    .vidioc_dqbuf = vb2_ioctl_dqbuf,
    .vidioc_streamon = vb2_ioctl_streamon,
    .vidioc_streamoff = vb2_ioctl_streamoff,
};

/* V4L2 file operations */
static const struct v4l2_file_operations camera_fops = {
    .owner = THIS_MODULE,
    .open = v4l2_fh_open,
    .release = vb2_fop_release,
    .read = vb2_fop_read,
    .poll = vb2_fop_poll,
    .unlocked_ioctl = video_ioctl2,
    .mmap = vb2_fop_mmap,
};

/*
 * Helper Functions
 */

/* Initialize VB2 queue */
static int camera_init_vb2_queue(struct camera_device *dev)
{
    struct vb2_queue *q = &dev->queue;

    q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    q->io_modes = VB2_MMAP | VB2_READ;
    q->drv_priv = dev;
    q->buf_struct_size = sizeof(struct camera_buffer);
    q->ops = &camera_vb2_ops;
    q->mem_ops = &vb2_vmalloc_memops;
    q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
    q->lock = &dev->lock;

    return vb2_queue_init(q);
}

/* Create video device */
static int camera_create_video_device(struct camera_device *dev)
{
    struct video_device *vdev;
    int ret;

    vdev = video_device_alloc();
    if (!vdev)
        return -ENOMEM;

    vdev->v4l2_dev = &dev->v4l2_dev;
    vdev->fops = &camera_fops;
    vdev->ioctl_ops = &camera_ioctl_ops;
    vdev->release = video_device_release;
    vdev->lock = &dev->lock;
    vdev->queue = &dev->queue;

    strscpy(vdev->name, "IMX6ULL Camera", sizeof(vdev->name));
    vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

    video_set_drvdata(vdev, dev);

    /* Set default format */
    dev->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    dev->format.fmt.pix.width = 640;
    dev->format.fmt.pix.height = 480;
    dev->format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    dev->format.fmt.pix.field = V4L2_FIELD_NONE;
    dev->format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

    ret = video_register_device(vdev, VFL_TYPE_GRABBER, -1);
    if (ret) {
        video_device_release(vdev);
        return ret;
    }

    dev->vdev = vdev;
    return 0;
}

/* Initialize USB streaming */
static int camera_init_streaming(struct camera_device *dev)
{
    /* This would contain USB-specific streaming initialization */
    /* For now, return success as a placeholder */
    return 0;
}

/* Stop USB streaming */
static void camera_stop_usb_streaming(struct camera_device *dev)
{
    /* Stop USB transfers and clean up URBs */
    /* Implementation depends on specific USB streaming setup */
}

/* Return all buffers to videobuf2 */
static void camera_return_all_buffers(struct camera_device *dev,
                                     enum vb2_buffer_state state)
{
    struct camera_buffer *buf, *tmp;
    unsigned long flags;

    spin_lock_irqsave(&dev->buf_lock, flags);
    list_for_each_entry_safe(buf, tmp, &dev->buf_list, list) {
        list_del(&buf->list);
        vb2_buffer_done(&buf->vb.vb2_buf, state);
    }
    spin_unlock_irqrestore(&dev->buf_lock, flags);
}

/*
 * USB Operations
 */

/* USB probe function */
static int camera_probe(struct usb_interface *intf,
                       const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(intf);
    struct camera_device *dev;
    int ret;

    dev_info(&intf->dev, "Probing camera device\n");

    /* Allocate device structure */
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    /* Initialize device */
    dev->udev = udev;
    dev->intf = intf;
    dev->state = CAMERA_STATE_CONNECTED;

    mutex_init(&dev->lock);
    spin_lock_init(&dev->buf_lock);
    INIT_LIST_HEAD(&dev->buf_list);

    /* Register V4L2 device */
    ret = v4l2_device_register(&intf->dev, &dev->v4l2_dev);
    if (ret) {
        dev_err(&intf->dev, "Failed to register V4L2 device: %d\n", ret);
        goto error_free;
    }

    /* Initialize video buffer queue */
    ret = camera_init_vb2_queue(dev);
    if (ret) {
        dev_err(&intf->dev, "Failed to initialize VB2 queue: %d\n", ret);
        goto error_v4l2;
    }

    /* Create video device */
    ret = camera_create_video_device(dev);
    if (ret) {
        dev_err(&intf->dev, "Failed to create video device: %d\n", ret);
        goto error_v4l2;
    }

    /* Set interface data */
    usb_set_intfdata(intf, dev);

    dev_info(&intf->dev, "Camera device registered as %s\n",
             video_device_node_name(dev->vdev));

    return 0;

error_v4l2:
    v4l2_device_unregister(&dev->v4l2_dev);
error_free:
    kfree(dev);
    return ret;
}

/* USB disconnect function */
static void camera_disconnect(struct usb_interface *intf)
{
    struct camera_device *dev = usb_get_intfdata(intf);
    
    dev_info(&intf->dev, "Disconnecting camera device\n");
    
    if (!dev)
        return;
    
    dev->state = CAMERA_STATE_DISCONNECTED;
    
    /* Unregister video device */
    if (dev->vdev) {
        video_unregister_device(dev->vdev);
        dev->vdev = NULL;
    }
    
    /* Unregister V4L2 device */
    v4l2_device_unregister(&dev->v4l2_dev);
    
    /* Clear interface data */
    usb_set_intfdata(intf, NULL);
    
    /* Free device structure */
    kfree(dev);
}

/*
 * Module initialization and cleanup
 */

static int __init camera_init(void)
{
    int ret;
    
    pr_info("IMX6ULL Camera Driver v%s\n", DRIVER_VERSION);
    
    ret = usb_register(&camera_driver);
    if (ret) {
        pr_err("Failed to register USB driver: %d\n", ret);
        return ret;
    }
    
    pr_info("Camera driver registered successfully\n");
    return 0;
}

static void __exit camera_exit(void)
{
    usb_deregister(&camera_driver);
    pr_info("Camera driver unregistered\n");
}

module_init(camera_init);
module_exit(camera_exit);

MODULE_AUTHOR("Camera Driver Team");
MODULE_DESCRIPTION("USB Camera Driver for IMX6ULL Pro");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");

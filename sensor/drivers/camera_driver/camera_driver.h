/*
 * Camera Driver Header for IMX6ULL Pro
 * 
 * Defines structures, constants and function prototypes
 * for the USB camera driver
 */

#ifndef _CAMERA_DRIVER_H_
#define _CAMERA_DRIVER_H_

#include <linux/usb.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-v4l2.h>

/* Driver information */
#define CAMERA_DRIVER_NAME    "imx6ull_camera"
#define CAMERA_DRIVER_VERSION "1.0.0"

/* Buffer management */
#define CAMERA_MAX_BUFFERS    4
#define CAMERA_MIN_BUFFERS    2
#define CAMERA_MAX_FRAME_SIZE (1280 * 720 * 2)

/* USB streaming parameters */
#define CAMERA_MAX_URBS       8
#define CAMERA_URB_TIMEOUT    1000  /* ms */

/* Device states */
enum camera_state {
    CAMERA_STATE_DISCONNECTED = 0,
    CAMERA_STATE_CONNECTED,
    CAMERA_STATE_STREAMING,
    CAMERA_STATE_ERROR
};

/* Supported pixel formats */
enum camera_format {
    CAMERA_FORMAT_MJPEG = 0,
    CAMERA_FORMAT_YUYV,
    CAMERA_FORMAT_MAX
};

/* Frame size structure */
struct camera_frame_size {
    u32 width;
    u32 height;
    u32 pixelformat;
    u32 fps;
};

/* Buffer structure for video data */
struct camera_buffer {
    struct vb2_v4l2_buffer vb;
    struct list_head list;
    void *vaddr;
    dma_addr_t dma_addr;
    size_t size;
    u64 timestamp;
};

/* USB streaming context */
struct camera_streaming {
    struct usb_host_endpoint *endpoint;
    struct urb *urbs[CAMERA_MAX_URBS];
    u8 *transfer_buffer[CAMERA_MAX_URBS];
    int num_urbs;
    int urb_size;
    atomic_t active_urbs;
    
    /* Frame assembly */
    u8 *frame_buffer;
    size_t frame_size;
    size_t frame_pos;
    bool frame_complete;
    
    /* Statistics */
    atomic_t packets_received;
    atomic_t packets_dropped;
    atomic_t errors;
};

/* Main camera device structure */
struct camera_device {
    /* V4L2 and USB core */
    struct v4l2_device v4l2_dev;
    struct video_device *vdev;
    struct usb_device *udev;
    struct usb_interface *intf;
    
    /* Device state and synchronization */
    enum camera_state state;
    struct mutex lock;          /* Protects device state */
    struct mutex streaming_lock; /* Protects streaming operations */
    
    /* Video buffer management */
    struct vb2_queue queue;
    struct list_head buf_list;
    spinlock_t buf_lock;        /* Protects buffer list */
    
    /* Format and streaming parameters */
    struct v4l2_format format;
    struct v4l2_streamparm parm;
    struct camera_frame_size current_size;
    
    /* USB streaming */
    struct camera_streaming streaming;
    
    /* Device capabilities */
    u32 capabilities;
    struct camera_frame_size *supported_sizes;
    int num_supported_sizes;
    
    /* Statistics and debugging */
    atomic_t frames_received;
    atomic_t frames_dropped;
    atomic_t bytes_received;
    u64 last_frame_time;
    u64 start_time;
    
    /* Error handling */
    int error_count;
    int last_error;
    struct delayed_work error_recovery_work;
};

/* USB control structures */
struct camera_control {
    u8 entity;
    u8 selector;
    u8 size;
    u16 index;
};

/* Function prototypes */

/* Core driver functions */
int camera_probe(struct usb_interface *intf, const struct usb_device_id *id);
void camera_disconnect(struct usb_interface *intf);

/* V4L2 operations */
int camera_querycap(struct file *file, void *priv, struct v4l2_capability *cap);
int camera_enum_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f);
int camera_g_fmt(struct file *file, void *priv, struct v4l2_format *f);
int camera_s_fmt(struct file *file, void *priv, struct v4l2_format *f);
int camera_try_fmt(struct file *file, void *priv, struct v4l2_format *f);

/* Videobuf2 operations */
int camera_queue_setup(struct vb2_queue *q, unsigned int *nbuffers,
                      unsigned int *nplanes, unsigned int sizes[],
                      struct device *alloc_devs[]);
int camera_buf_prepare(struct vb2_buffer *vb);
void camera_buf_queue(struct vb2_buffer *vb);
int camera_start_streaming(struct vb2_queue *q, unsigned int count);
void camera_stop_streaming(struct vb2_queue *q);

/* USB streaming functions */
int camera_init_streaming(struct camera_device *dev);
void camera_cleanup_streaming(struct camera_device *dev);
int camera_start_usb_streaming(struct camera_device *dev);
void camera_stop_usb_streaming(struct camera_device *dev);
void camera_urb_complete(struct urb *urb);

/* Buffer management */
struct camera_buffer *camera_get_next_buffer(struct camera_device *dev);
void camera_buffer_done(struct camera_device *dev, struct camera_buffer *buf,
                       enum vb2_buffer_state state);
void camera_return_all_buffers(struct camera_device *dev,
                              enum vb2_buffer_state state);

/* Device management */
int camera_init_device(struct camera_device *dev);
void camera_cleanup_device(struct camera_device *dev);
int camera_reset_device(struct camera_device *dev);

/* USB control functions */
int camera_get_control(struct camera_device *dev, struct camera_control *ctrl,
                      void *data);
int camera_set_control(struct camera_device *dev, struct camera_control *ctrl,
                      const void *data);

/* Format and capability functions */
int camera_enum_frame_sizes(struct camera_device *dev);
int camera_validate_format(struct camera_device *dev, struct v4l2_format *f);
int camera_set_format(struct camera_device *dev, struct v4l2_format *f);

/* Error handling and recovery */
void camera_handle_error(struct camera_device *dev, int error);
void camera_error_recovery_work(struct work_struct *work);

/* Utility functions */
const char *camera_state_to_string(enum camera_state state);
void camera_print_format(struct camera_device *dev, struct v4l2_format *f);
void camera_print_statistics(struct camera_device *dev);

/* Debug macros */
#ifdef DEBUG
#define camera_dbg(dev, fmt, args...) \
    dev_dbg(&(dev)->udev->dev, fmt, ##args)
#else
#define camera_dbg(dev, fmt, args...) do { } while (0)
#endif

#define camera_info(dev, fmt, args...) \
    dev_info(&(dev)->udev->dev, fmt, ##args)

#define camera_warn(dev, fmt, args...) \
    dev_warn(&(dev)->udev->dev, fmt, ##args)

#define camera_err(dev, fmt, args...) \
    dev_err(&(dev)->udev->dev, fmt, ##args)

/* Constants for supported formats and sizes */
extern const struct v4l2_fmtdesc camera_formats[];
extern const struct camera_frame_size camera_frame_sizes[];
extern const int camera_num_formats;
extern const int camera_num_frame_sizes;

#endif /* _CAMERA_DRIVER_H_ */

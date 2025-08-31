/*
 * Camera Driver Test Program for IMX6ULL Pro
 * 
 * Tests basic camera functionality including:
 * - Device detection and initialization
 * - Format setting and validation
 * - Frame capture and processing
 * - Performance measurement
 * 
 * Author: Test Team
 * License: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define TEST_DEVICE "/dev/video0"
#define TEST_WIDTH 640
#define TEST_HEIGHT 480
#define TEST_FPS 30
#define TEST_BUFFER_COUNT 4
#define TEST_FRAME_COUNT 100

struct buffer {
    void *start;
    size_t length;
};

struct test_context {
    int fd;
    struct buffer *buffers;
    unsigned int n_buffers;
    struct v4l2_format format;
    struct v4l2_capability cap;
    
    // Statistics
    unsigned int frames_captured;
    unsigned int frames_dropped;
    struct timeval start_time;
    struct timeval end_time;
};

// Function prototypes
static int test_device_capabilities(struct test_context *ctx);
static int test_format_setting(struct test_context *ctx);
static int test_buffer_allocation(struct test_context *ctx);
static int test_streaming(struct test_context *ctx);
static int test_performance(struct test_context *ctx);
static void cleanup_buffers(struct test_context *ctx);
static double get_time_diff(struct timeval *start, struct timeval *end);
static void print_test_result(const char *test_name, int result);

int main(int argc, char *argv[])
{
    struct test_context ctx;
    const char *device = TEST_DEVICE;
    int result = 0;
    
    printf("=== IMX6ULL Camera Driver Test ===\n");
    
    // Parse command line arguments
    if (argc > 1) {
        device = argv[1];
    }
    
    printf("Testing device: %s\n\n", device);
    
    // Initialize context
    memset(&ctx, 0, sizeof(ctx));
    ctx.fd = -1;
    
    // Open device
    ctx.fd = open(device, O_RDWR | O_NONBLOCK, 0);
    if (ctx.fd == -1) {
        fprintf(stderr, "Cannot open device %s: %s\n", device, strerror(errno));
        return -1;
    }
    
    printf("Device opened successfully\n");
    
    // Run tests
    printf("\n--- Running Tests ---\n");
    
    result = test_device_capabilities(&ctx);
    print_test_result("Device Capabilities", result);
    if (result != 0) goto cleanup;
    
    result = test_format_setting(&ctx);
    print_test_result("Format Setting", result);
    if (result != 0) goto cleanup;
    
    result = test_buffer_allocation(&ctx);
    print_test_result("Buffer Allocation", result);
    if (result != 0) goto cleanup;
    
    result = test_streaming(&ctx);
    print_test_result("Streaming", result);
    if (result != 0) goto cleanup;
    
    result = test_performance(&ctx);
    print_test_result("Performance", result);
    
cleanup:
    cleanup_buffers(&ctx);
    if (ctx.fd >= 0) {
        close(ctx.fd);
    }
    
    printf("\n=== Test Summary ===\n");
    if (result == 0) {
        printf("All tests PASSED\n");
    } else {
        printf("Some tests FAILED\n");
    }
    
    return result;
}

static int test_device_capabilities(struct test_context *ctx)
{
    printf("Testing device capabilities...\n");
    
    if (ioctl(ctx->fd, VIDIOC_QUERYCAP, &ctx->cap) == -1) {
        fprintf(stderr, "VIDIOC_QUERYCAP failed: %s\n", strerror(errno));
        return -1;
    }
    
    printf("  Driver: %s\n", ctx->cap.driver);
    printf("  Card: %s\n", ctx->cap.card);
    printf("  Bus info: %s\n", ctx->cap.bus_info);
    printf("  Version: %u.%u.%u\n", 
           (ctx->cap.version >> 16) & 0xFF,
           (ctx->cap.version >> 8) & 0xFF,
           ctx->cap.version & 0xFF);
    
    // Check required capabilities
    if (!(ctx->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "Device does not support video capture\n");
        return -1;
    }
    
    if (!(ctx->cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "Device does not support streaming\n");
        return -1;
    }
    
    printf("  Capabilities: Video Capture, Streaming\n");
    return 0;
}

static int test_format_setting(struct test_context *ctx)
{
    struct v4l2_fmtdesc fmt_desc;
    struct v4l2_frmsizeenum frmsize;
    int index = 0;
    
    printf("Testing format setting...\n");
    
    // Enumerate supported formats
    printf("  Supported formats:\n");
    memset(&fmt_desc, 0, sizeof(fmt_desc));
    fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    while (ioctl(ctx->fd, VIDIOC_ENUM_FMT, &fmt_desc) == 0) {
        printf("    %d: %s (%.4s)\n", fmt_desc.index, fmt_desc.description,
               (char*)&fmt_desc.pixelformat);
        
        // Enumerate frame sizes for this format
        memset(&frmsize, 0, sizeof(frmsize));
        frmsize.pixel_format = fmt_desc.pixelformat;
        frmsize.index = 0;
        
        while (ioctl(ctx->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                printf("      %dx%d\n", frmsize.discrete.width, frmsize.discrete.height);
            }
            frmsize.index++;
        }
        
        fmt_desc.index++;
    }
    
    // Set format
    memset(&ctx->format, 0, sizeof(ctx->format));
    ctx->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ctx->format.fmt.pix.width = TEST_WIDTH;
    ctx->format.fmt.pix.height = TEST_HEIGHT;
    ctx->format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    ctx->format.fmt.pix.field = V4L2_FIELD_NONE;
    
    if (ioctl(ctx->fd, VIDIOC_S_FMT, &ctx->format) == -1) {
        fprintf(stderr, "VIDIOC_S_FMT failed: %s\n", strerror(errno));
        return -1;
    }
    
    printf("  Set format: %dx%d, %.4s\n",
           ctx->format.fmt.pix.width,
           ctx->format.fmt.pix.height,
           (char*)&ctx->format.fmt.pix.pixelformat);
    
    // Verify format
    if (ioctl(ctx->fd, VIDIOC_G_FMT, &ctx->format) == -1) {
        fprintf(stderr, "VIDIOC_G_FMT failed: %s\n", strerror(errno));
        return -1;
    }
    
    printf("  Actual format: %dx%d, %.4s, size: %u bytes\n",
           ctx->format.fmt.pix.width,
           ctx->format.fmt.pix.height,
           (char*)&ctx->format.fmt.pix.pixelformat,
           ctx->format.fmt.pix.sizeimage);
    
    return 0;
}

static int test_buffer_allocation(struct test_context *ctx)
{
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    unsigned int i;
    
    printf("Testing buffer allocation...\n");
    
    // Request buffers
    memset(&req, 0, sizeof(req));
    req.count = TEST_BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(ctx->fd, VIDIOC_REQBUFS, &req) == -1) {
        fprintf(stderr, "VIDIOC_REQBUFS failed: %s\n", strerror(errno));
        return -1;
    }
    
    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory\n");
        return -1;
    }
    
    printf("  Requested %d buffers, got %d\n", TEST_BUFFER_COUNT, req.count);
    ctx->n_buffers = req.count;
    
    // Allocate buffer array
    ctx->buffers = calloc(req.count, sizeof(*ctx->buffers));
    if (!ctx->buffers) {
        fprintf(stderr, "Out of memory\n");
        return -1;
    }
    
    // Map buffers
    for (i = 0; i < ctx->n_buffers; ++i) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (ioctl(ctx->fd, VIDIOC_QUERYBUF, &buf) == -1) {
            fprintf(stderr, "VIDIOC_QUERYBUF failed: %s\n", strerror(errno));
            return -1;
        }
        
        ctx->buffers[i].length = buf.length;
        ctx->buffers[i].start = mmap(NULL, buf.length,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    ctx->fd, buf.m.offset);
        
        if (MAP_FAILED == ctx->buffers[i].start) {
            fprintf(stderr, "mmap failed: %s\n", strerror(errno));
            return -1;
        }
        
        printf("  Buffer %d: %zu bytes mapped\n", i, ctx->buffers[i].length);
    }
    
    return 0;
}

static int test_streaming(struct test_context *ctx)
{
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;
    unsigned int i;
    int frames_to_capture = 10;
    
    printf("Testing streaming...\n");
    
    // Queue all buffers
    for (i = 0; i < ctx->n_buffers; ++i) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (ioctl(ctx->fd, VIDIOC_QBUF, &buf) == -1) {
            fprintf(stderr, "VIDIOC_QBUF failed: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Start streaming
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(ctx->fd, VIDIOC_STREAMON, &type) == -1) {
        fprintf(stderr, "VIDIOC_STREAMON failed: %s\n", strerror(errno));
        return -1;
    }
    
    printf("  Streaming started, capturing %d frames...\n", frames_to_capture);
    
    // Capture frames
    for (i = 0; i < frames_to_capture; ++i) {
        fd_set fds;
        struct timeval tv;
        int r;
        
        FD_ZERO(&fds);
        FD_SET(ctx->fd, &fds);
        
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        
        r = select(ctx->fd + 1, &fds, NULL, NULL, &tv);
        
        if (-1 == r) {
            if (EINTR == errno) continue;
            fprintf(stderr, "select failed: %s\n", strerror(errno));
            return -1;
        }
        
        if (0 == r) {
            fprintf(stderr, "select timeout\n");
            return -1;
        }
        
        // Dequeue buffer
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(ctx->fd, VIDIOC_DQBUF, &buf) == -1) {
            fprintf(stderr, "VIDIOC_DQBUF failed: %s\n", strerror(errno));
            return -1;
        }
        
        printf("  Frame %d: %u bytes, sequence %u\n", 
               i + 1, buf.bytesused, buf.sequence);
        
        // Queue buffer back
        if (ioctl(ctx->fd, VIDIOC_QBUF, &buf) == -1) {
            fprintf(stderr, "VIDIOC_QBUF failed: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Stop streaming
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(ctx->fd, VIDIOC_STREAMOFF, &type) == -1) {
        fprintf(stderr, "VIDIOC_STREAMOFF failed: %s\n", strerror(errno));
        return -1;
    }
    
    printf("  Streaming stopped\n");
    return 0;
}

static int test_performance(struct test_context *ctx)
{
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;
    unsigned int i;
    double elapsed_time, fps;
    
    printf("Testing performance...\n");
    
    // Start streaming
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(ctx->fd, VIDIOC_STREAMON, &type) == -1) {
        fprintf(stderr, "VIDIOC_STREAMON failed: %s\n", strerror(errno));
        return -1;
    }
    
    gettimeofday(&ctx->start_time, NULL);
    
    // Capture frames for performance measurement
    for (i = 0; i < TEST_FRAME_COUNT; ++i) {
        fd_set fds;
        struct timeval tv;
        int r;
        
        FD_ZERO(&fds);
        FD_SET(ctx->fd, &fds);
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        r = select(ctx->fd + 1, &fds, NULL, NULL, &tv);
        
        if (r <= 0) {
            ctx->frames_dropped++;
            continue;
        }
        
        // Dequeue buffer
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(ctx->fd, VIDIOC_DQBUF, &buf) == -1) {
            ctx->frames_dropped++;
            continue;
        }
        
        ctx->frames_captured++;
        
        // Queue buffer back
        ioctl(ctx->fd, VIDIOC_QBUF, &buf);
    }
    
    gettimeofday(&ctx->end_time, NULL);
    
    // Stop streaming
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(ctx->fd, VIDIOC_STREAMOFF, &type);
    
    // Calculate performance metrics
    elapsed_time = get_time_diff(&ctx->start_time, &ctx->end_time);
    fps = ctx->frames_captured / elapsed_time;
    
    printf("  Performance Results:\n");
    printf("    Frames captured: %u\n", ctx->frames_captured);
    printf("    Frames dropped: %u\n", ctx->frames_dropped);
    printf("    Elapsed time: %.2f seconds\n", elapsed_time);
    printf("    Average FPS: %.2f\n", fps);
    printf("    Target FPS: %d\n", TEST_FPS);
    
    if (fps < TEST_FPS * 0.8) {
        printf("    WARNING: FPS below 80%% of target\n");
        return -1;
    }
    
    return 0;
}

static void cleanup_buffers(struct test_context *ctx)
{
    unsigned int i;
    
    if (ctx->buffers) {
        for (i = 0; i < ctx->n_buffers; ++i) {
            if (ctx->buffers[i].start != MAP_FAILED) {
                munmap(ctx->buffers[i].start, ctx->buffers[i].length);
            }
        }
        free(ctx->buffers);
        ctx->buffers = NULL;
    }
}

static double get_time_diff(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) + 
           (end->tv_usec - start->tv_usec) / 1000000.0;
}

static void print_test_result(const char *test_name, int result)
{
    printf("  %s: %s\n", test_name, result == 0 ? "PASS" : "FAIL");
}

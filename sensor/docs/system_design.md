# IMX6ULL Pro摄像头驱动系统设计文档

## 1. 系统架构概述

### 1.1 整体架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                    应用层 (Application Layer)                │
├─────────────────────────────────────────────────────────────┤
│  人脸识别应用  │  配置管理工具  │  网络服务  │  调试工具    │
├─────────────────────────────────────────────────────────────┤
│                   API接口层 (API Layer)                     │
├─────────────────────────────────────────────────────────────┤
│  Camera API   │  Face API    │  Network API │  Config API   │
├─────────────────────────────────────────────────────────────┤
│                  中间件层 (Middleware Layer)                 │
├─────────────────────────────────────────────────────────────┤
│ 图像处理模块  │ 人脸识别模块  │  网络模块   │  配置模块    │
├─────────────────────────────────────────────────────────────┤
│                   驱动层 (Driver Layer)                     │
├─────────────────────────────────────────────────────────────┤
│   V4L2驱动    │   UVC驱动    │  USB驱动   │  网络驱动    │
├─────────────────────────────────────────────────────────────┤
│                   硬件层 (Hardware Layer)                   │
└─────────────────────────────────────────────────────────────┘
│  IMX6ULL Pro  │  USB摄像头   │  WiFi模块  │  存储设备    │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 设计原则
- **模块化设计**：各模块职责清晰，接口标准化
- **分层架构**：上层不直接访问底层硬件
- **异步处理**：图像处理和识别采用异步模式
- **资源优化**：针对IMX6ULL性能特点优化
- **可扩展性**：预留接口支持功能扩展

## 2. 驱动层设计

### 2.1 USB摄像头驱动架构

```c
// 驱动模块结构
struct camera_driver {
    struct v4l2_device v4l2_dev;
    struct video_device *vdev;
    struct usb_device *udev;
    struct usb_interface *intf;
    
    // 设备状态
    enum camera_state state;
    struct mutex lock;
    
    // 缓冲区管理
    struct vb2_queue queue;
    struct list_head buf_list;
    spinlock_t buf_lock;
    
    // 格式信息
    struct v4l2_format format;
    struct v4l2_streamparm parm;
};
```

### 2.2 V4L2接口实现

```c
// V4L2操作函数表
static const struct v4l2_file_operations camera_fops = {
    .owner          = THIS_MODULE,
    .open           = camera_open,
    .release        = camera_close,
    .read           = camera_read,
    .poll           = camera_poll,
    .unlocked_ioctl = video_ioctl2,
    .mmap           = vb2_fop_mmap,
};

static const struct v4l2_ioctl_ops camera_ioctl_ops = {
    .vidioc_querycap         = camera_querycap,
    .vidioc_enum_fmt_vid_cap = camera_enum_fmt,
    .vidioc_g_fmt_vid_cap    = camera_g_fmt,
    .vidioc_s_fmt_vid_cap    = camera_s_fmt,
    .vidioc_try_fmt_vid_cap  = camera_try_fmt,
    .vidioc_reqbufs          = vb2_ioctl_reqbufs,
    .vidioc_querybuf         = vb2_ioctl_querybuf,
    .vidioc_qbuf             = vb2_ioctl_qbuf,
    .vidioc_dqbuf            = vb2_ioctl_dqbuf,
    .vidioc_streamon         = vb2_ioctl_streamon,
    .vidioc_streamoff        = vb2_ioctl_streamoff,
};
```

### 2.3 缓冲区管理设计

```c
// 缓冲区结构
struct camera_buffer {
    struct vb2_v4l2_buffer vb;
    struct list_head list;
    void *vaddr;
    dma_addr_t dma_addr;
    size_t size;
};

// 缓冲区队列操作
static struct vb2_ops camera_vb2_ops = {
    .queue_setup    = camera_queue_setup,
    .buf_prepare    = camera_buf_prepare,
    .buf_queue      = camera_buf_queue,
    .start_streaming = camera_start_streaming,
    .stop_streaming = camera_stop_streaming,
};
```

## 3. 中间件层设计

### 3.1 图像处理模块

```cpp
// 图像处理管理器
class ImageProcessor {
public:
    // 初始化和清理
    int Initialize(const ProcessConfig& config);
    void Cleanup();
    
    // 格式转换
    int ConvertFormat(const ImageFrame& input, ImageFrame& output, 
                     ImageFormat target_format);
    
    // 图像预处理
    int Preprocess(const ImageFrame& input, ImageFrame& output);
    
    // 缓冲区管理
    int AllocateBuffer(size_t size, void** buffer);
    void ReleaseBuffer(void* buffer);
    
private:
    ProcessConfig config_;
    MemoryPool memory_pool_;
    std::mutex mutex_;
};

// 图像帧结构
struct ImageFrame {
    void* data;
    size_t size;
    int width;
    int height;
    ImageFormat format;
    uint64_t timestamp;
};
```

### 3.2 人脸识别模块

```cpp
// 人脸识别引擎
class FaceRecognitionEngine {
public:
    // 初始化
    int Initialize(const std::string& model_path);
    void Cleanup();
    
    // 人脸检测
    int DetectFaces(const ImageFrame& frame, 
                   std::vector<FaceInfo>& faces);
    
    // 人脸识别
    int RecognizeFaces(const ImageFrame& frame,
                      const std::vector<FaceInfo>& faces,
                      std::vector<RecognitionResult>& results);
    
    // 人脸库管理
    int AddFace(const std::string& person_id, 
               const ImageFrame& face_image);
    int RemoveFace(const std::string& person_id);
    int UpdateFace(const std::string& person_id,
                  const ImageFrame& face_image);
    
private:
    ncnn::Net detector_net_;
    ncnn::Net recognizer_net_;
    FaceDatabase face_db_;
    std::mutex mutex_;
};

// 人脸信息结构
struct FaceInfo {
    cv::Rect bbox;          // 边界框
    float confidence;       // 置信度
    cv::Point2f landmarks[5]; // 关键点
};

// 识别结果结构
struct RecognitionResult {
    std::string person_id;
    float similarity;
    FaceInfo face_info;
};
```

### 3.3 网络模块

```c
// 网络管理器
struct network_manager {
    // WiFi管理
    struct wifi_config wifi_cfg;
    int wifi_connected;
    
    // TCP服务器
    int server_fd;
    int server_port;
    struct sockaddr_in server_addr;
    
    // 客户端连接
    struct client_connection clients[MAX_CLIENTS];
    int client_count;
    
    // 配置接口
    struct config_server config_srv;
};

// 网络API
int network_init(struct network_manager *mgr);
int network_start_server(struct network_manager *mgr, int port);
int network_send_result(struct network_manager *mgr, 
                       const struct recognition_result *result);
int network_handle_config(struct network_manager *mgr,
                         const char *config_data);
```

## 4. API接口层设计

### 4.1 Camera API

```c
// 摄像头API
typedef struct {
    int device_id;
    int width;
    int height;
    int fps;
    int format;
} camera_config_t;

typedef struct {
    void *data;
    size_t size;
    uint64_t timestamp;
    int width;
    int height;
    int format;
} camera_frame_t;

// API函数
int camera_init(void);
int camera_open(int device_id, const camera_config_t *config);
int camera_start_stream(int device_id);
int camera_get_frame(int device_id, camera_frame_t *frame);
int camera_release_frame(int device_id, camera_frame_t *frame);
int camera_stop_stream(int device_id);
int camera_close(int device_id);
void camera_cleanup(void);
```

### 4.2 Face Recognition API

```c
// 人脸识别API
typedef struct {
    int x, y, width, height;
    float confidence;
} face_bbox_t;

typedef struct {
    char person_id[64];
    float similarity;
    face_bbox_t bbox;
} face_result_t;

// API函数
int face_engine_init(const char *model_path);
int face_detect(const camera_frame_t *frame, 
               face_bbox_t *faces, int *face_count);
int face_recognize(const camera_frame_t *frame,
                  const face_bbox_t *faces, int face_count,
                  face_result_t *results);
int face_add_person(const char *person_id, 
                   const camera_frame_t *face_image);
int face_remove_person(const char *person_id);
void face_engine_cleanup(void);
```

### 4.3 Network API

```c
// 网络API
typedef struct {
    char ssid[32];
    char password[64];
    int security_type;
} wifi_config_t;

typedef void (*result_callback_t)(const face_result_t *result, void *user_data);

// API函数
int network_init(void);
int network_connect_wifi(const wifi_config_t *config);
int network_start_service(int port);
int network_register_callback(result_callback_t callback, void *user_data);
int network_send_notification(const char *message);
void network_cleanup(void);
```

## 5. 数据流设计

### 5.1 图像数据流

```
USB摄像头 → UVC驱动 → V4L2缓冲区 → 图像处理 → 人脸检测 → 人脸识别 → 结果输出
     ↓           ↓          ↓           ↓          ↓          ↓          ↓
   硬件层     驱动层    中间件层    中间件层    中间件层    中间件层    应用层
```

### 5.2 控制流设计

```
应用层请求 → API接口 → 中间件处理 → 驱动控制 → 硬件操作
     ↓          ↓         ↓          ↓          ↓
   用户接口   标准API   业务逻辑   设备控制   硬件响应
```

### 5.3 异步处理机制

```c
// 异步处理框架
struct async_processor {
    pthread_t thread_id;
    struct queue *input_queue;
    struct queue *output_queue;
    int (*process_func)(void *input, void **output);
    void *user_data;
    int running;
};

// 处理流水线
struct processing_pipeline {
    struct async_processor image_proc;
    struct async_processor face_detect;
    struct async_processor face_recog;
    struct async_processor result_output;
};
```

## 6. 内存管理设计

### 6.1 内存池设计

```c
// 内存池结构
struct memory_pool {
    void *base_addr;
    size_t total_size;
    size_t block_size;
    int block_count;
    
    unsigned long *bitmap;
    int free_blocks;
    struct mutex lock;
};

// 内存池操作
int mempool_init(struct memory_pool *pool, size_t total_size, size_t block_size);
void *mempool_alloc(struct memory_pool *pool);
void mempool_free(struct memory_pool *pool, void *ptr);
void mempool_cleanup(struct memory_pool *pool);
```

### 6.2 缓冲区管理

```c
// 缓冲区管理器
struct buffer_manager {
    struct memory_pool image_pool;
    struct memory_pool result_pool;
    
    // 循环缓冲区
    struct circular_buffer *frame_buffer;
    struct circular_buffer *result_buffer;
    
    // 统计信息
    atomic_t alloc_count;
    atomic_t free_count;
    atomic_t peak_usage;
};
```

## 7. 错误处理和恢复

### 7.1 错误码定义

```c
// 错误码定义
typedef enum {
    ERR_SUCCESS = 0,
    ERR_INVALID_PARAM = -1,
    ERR_NO_MEMORY = -2,
    ERR_DEVICE_NOT_FOUND = -3,
    ERR_DEVICE_BUSY = -4,
    ERR_IO_ERROR = -5,
    ERR_TIMEOUT = -6,
    ERR_NOT_SUPPORTED = -7,
    ERR_SYSTEM_ERROR = -8,
} error_code_t;
```

### 7.2 异常恢复机制

```c
// 恢复策略
struct recovery_strategy {
    error_code_t error_type;
    int max_retry_count;
    int retry_interval_ms;
    int (*recovery_func)(void *context);
    void *context;
};

// 错误处理器
struct error_handler {
    struct recovery_strategy strategies[MAX_STRATEGIES];
    int strategy_count;
    
    // 错误统计
    atomic_t error_counts[ERR_MAX];
    time_t last_error_time[ERR_MAX];
};
```

## 8. 性能优化设计

### 8.1 多线程设计

```c
// 线程池
struct thread_pool {
    pthread_t *threads;
    int thread_count;
    
    struct task_queue *queue;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    
    int shutdown;
};

// 任务结构
struct task {
    void (*function)(void *arg);
    void *arg;
    struct task *next;
};
```

### 8.2 缓存优化

```c
// 缓存管理
struct cache_manager {
    // 图像缓存
    struct lru_cache *image_cache;
    
    // 特征缓存
    struct lru_cache *feature_cache;
    
    // 配置缓存
    struct config_cache *config_cache;
    
    // 统计信息
    atomic_t hit_count;
    atomic_t miss_count;
};
```

## 9. 配置管理设计

### 9.1 配置文件格式

```json
{
    "camera": {
        "device_id": 0,
        "width": 640,
        "height": 480,
        "fps": 30,
        "format": "MJPEG"
    },
    "face_recognition": {
        "model_path": "/opt/models/",
        "confidence_threshold": 0.7,
        "max_face_count": 5,
        "detection_interval": 100
    },
    "network": {
        "wifi_ssid": "your_wifi",
        "wifi_password": "your_password",
        "server_port": 8080,
        "enable_remote_config": true
    },
    "system": {
        "log_level": "INFO",
        "max_memory_usage": 200,
        "enable_debug": false
    }
}
```

### 9.2 配置管理器

```c
// 配置管理器
struct config_manager {
    json_object *config_root;
    char config_file_path[256];
    
    // 配置监听
    int inotify_fd;
    int watch_fd;
    
    // 配置回调
    struct config_callback callbacks[MAX_CALLBACKS];
    int callback_count;
    
    struct mutex lock;
};
```

## 10. 调试和监控设计

### 10.1 日志系统

```c
// 日志级别
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

// 日志管理器
struct log_manager {
    FILE *log_file;
    log_level_t current_level;
    int max_file_size;
    int max_file_count;
    
    struct mutex log_mutex;
};
```

### 10.2 性能监控

```c
// 性能计数器
struct performance_counters {
    // 帧率统计
    atomic_t frames_processed;
    atomic_t frames_dropped;
    
    // 延迟统计
    uint64_t total_latency_us;
    uint64_t max_latency_us;
    uint64_t min_latency_us;
    
    // 内存使用
    atomic_t memory_allocated;
    atomic_t memory_peak;
    
    // CPU使用率
    float cpu_usage;
    time_t last_update_time;
};
```

这个系统设计文档提供了完整的架构设计，包括各层的详细设计、接口定义、数据流、错误处理等关键方面。设计充分考虑了IMX6ULL Pro的性能特点和资源限制，采用了模块化、分层的架构，便于开发、测试和维护。

# IMX6ULL Pro摄像头驱动开发项目

## 项目概述

基于IMX6ULL Pro开发板的USB摄像头驱动开发项目，支持人脸识别功能的企业级嵌入式应用。

## 项目特性

- ✅ USB Video Class (UVC) 摄像头驱动支持
- ✅ V4L2 (Video4Linux2) 标准接口
- ✅ 实时人脸检测与识别
- ✅ WiFi网络连接和远程管理
- ✅ 轻量级算法优化，适配ARM Cortex-A7
- ✅ 模块化设计，易于扩展和维护

## 系统要求

### 硬件平台
- **主控芯片**: NXP IMX6ULL Pro (ARM Cortex-A7, 528MHz)
- **内存**: 512MB DDR3
- **摄像头**: USB 2.0接口，支持UVC协议
- **网络**: WiFi 802.11 b/g/n
- **存储**: eMMC/SD卡

### 软件环境
- **操作系统**: Linux 4.9.88
- **根文件系统**: Buildroot
- **交叉编译工具链**: gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf
- **依赖库**: OpenCV 3.4.x, NCNN推理框架

## 项目结构

```
sensor/
├── docs/                    # 项目文档
│   ├── requirements.md      # 需求文档
│   ├── system_design.md     # 系统设计文档
│   └── technical_research.md # 技术调研报告
├── drivers/                 # 驱动程序
│   ├── camera_driver/       # 摄像头驱动
│   └── v4l2_interface/      # V4L2接口实现
├── middleware/              # 中间件层
│   ├── image_process/       # 图像处理模块
│   ├── face_recognition/    # 人脸识别模块
│   └── network/            # 网络通信模块
├── api/                    # API接口层
│   ├── camera_api/         # 摄像头API
│   ├── face_api/           # 人脸识别API
│   └── network_api/        # 网络API
├── applications/           # 应用程序
│   ├── face_recognition_app/ # 人脸识别应用
│   ├── config_tool/        # 配置工具
│   └── test_tools/         # 测试工具
├── configs/                # 配置文件
├── scripts/                # 构建和部署脚本
├── tests/                  # 测试用例
└── README.md              # 项目说明
```

## 快速开始

### 1. 环境准备

```bash
# 设置交叉编译环境
export CROSS_COMPILE=arm-linux-gnueabihf-
export CC=${CROSS_COMPILE}gcc
export CXX=${CROSS_COMPILE}g++

# 设置目标架构
export ARCH=arm
export TARGET_PLATFORM=imx6ull
```

### 2. 编译项目

```bash
# 创建构建目录
mkdir build && cd build

# 配置CMake
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-linux-gnueabihf.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DTARGET_PLATFORM=imx6ull \
      ..

# 编译
make -j4
```

### 3. 部署到目标设备

```bash
# 复制驱动模块
scp drivers/*.ko root@target_ip:/lib/modules/4.9.88/extra/

# 复制应用程序
scp applications/face_recognition_app/face_app root@target_ip:/usr/bin/

# 复制配置文件
scp configs/system_config.json root@target_ip:/etc/
```

### 4. 运行应用

```bash
# 在目标设备上加载驱动
modprobe uvcvideo
modprobe camera_driver

# 启动人脸识别应用
./face_app --config /etc/system_config.json
```

## 配置说明

### 系统配置文件 (configs/system_config.json)

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
        "max_face_count": 5
    },
    "network": {
        "wifi_ssid": "your_wifi_ssid",
        "wifi_password": "your_wifi_password",
        "server_port": 8080
    }
}
```

## API使用示例

### 摄像头API

```c
#include "camera_api.h"

// 初始化摄像头
camera_config_t config = {
    .device_id = 0,
    .width = 640,
    .height = 480,
    .fps = 30,
    .format = CAMERA_FORMAT_MJPEG
};

int ret = camera_open(0, &config);
if (ret != 0) {
    printf("Failed to open camera: %d\n", ret);
    return -1;
}

// 开始视频流
camera_start_stream(0);

// 获取图像帧
camera_frame_t frame;
while (running) {
    ret = camera_get_frame(0, &frame);
    if (ret == 0) {
        // 处理图像帧
        process_frame(&frame);
        
        // 释放帧缓冲
        camera_release_frame(0, &frame);
    }
}

// 停止和清理
camera_stop_stream(0);
camera_close(0);
```

### 人脸识别API

```c
#include "face_api.h"

// 初始化人脸识别引擎
int ret = face_engine_init("/opt/models/");
if (ret != 0) {
    printf("Failed to init face engine: %d\n", ret);
    return -1;
}

// 检测人脸
face_bbox_t faces[MAX_FACES];
int face_count = MAX_FACES;
ret = face_detect(&frame, faces, &face_count);

if (ret == 0 && face_count > 0) {
    // 识别人脸
    face_result_t results[MAX_FACES];
    ret = face_recognize(&frame, faces, face_count, results);
    
    if (ret == 0) {
        for (int i = 0; i < face_count; i++) {
            printf("Person: %s, Similarity: %.2f\n", 
                   results[i].person_id, results[i].similarity);
        }
    }
}
```

## 性能优化

### 针对IMX6ULL的优化策略

1. **算法优化**
   - 使用轻量级人脸检测算法 (LFFD)
   - 模型量化 (INT8) 减少计算量
   - 输入图像尺寸优化 (320x240)

2. **内存优化**
   - 内存池管理减少分配开销
   - 循环缓冲区复用内存
   - 及时释放不需要的资源

3. **多线程优化**
   - 图像采集和处理分离
   - 异步处理流水线
   - 合理的线程优先级设置

## 测试

### 单元测试

```bash
# 运行单元测试
cd build
make test

# 或者直接运行测试程序
./tests/camera_test
./tests/face_recognition_test
./tests/network_test
```

### 性能测试

```bash
# 性能基准测试
./tests/performance_test --duration 60 --log-file perf.log

# 内存泄漏检测
valgrind --leak-check=full ./applications/face_recognition_app/face_app
```

## 故障排除

### 常见问题

1. **摄像头无法识别**
   ```bash
   # 检查USB设备
   lsusb
   
   # 检查内核日志
   dmesg | grep uvc
   
   # 检查V4L2设备
   ls /dev/video*
   ```

2. **人脸识别性能差**
   ```bash
   # 检查CPU使用率
   top
   
   # 检查内存使用
   free -m
   
   # 调整算法参数
   # 降低输入分辨率或检测频率
   ```

3. **网络连接问题**
   ```bash
   # 检查WiFi状态
   iwconfig
   
   # 检查网络配置
   ifconfig
   
   # 测试网络连通性
   ping gateway_ip
   ```

## 贡献指南

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开 Pull Request

## 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 联系方式

- 项目维护者: [Your Name]
- 邮箱: [your.email@example.com]
- 项目链接: [https://github.com/yourname/imx6ull-camera-driver](https://github.com/yourname/imx6ull-camera-driver)

## 致谢

- NXP 提供的 IMX6ULL 技术文档
- OpenCV 开源计算机视觉库
- NCNN 高性能神经网络推理框架
- Linux V4L2 子系统开发者

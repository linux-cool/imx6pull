# 摄像头人脸检测示例工程

这个示例工程演示了如何使用OpenCV读取系统摄像头数据并进行人脸检测。

## 功能特性

- 支持多种摄像头接口（USB、内置摄像头等）
- 实时人脸检测
- 性能统计和监控
- 可配置的检测参数
- 跨平台支持（Linux、Windows、macOS）

## 快速开始

### 简单示例（推荐新手）

如果你想快速测试人脸检测功能，可以使用简化版本：

```bash
# 进入示例目录
cd example

# 构建简单示例
./simple_build.sh

# 运行简单示例
./simple_face_detection_demo
```

简单示例特点：
- 单个源文件，易于理解
- 最小依赖，只需要OpenCV
- 基本的人脸检测功能
- 实时FPS显示
- 键盘控制（ESC退出，s保存图片，f全屏）

## 系统要求

### 软件依赖
- OpenCV 4.x 或更高版本
- CMake 3.10 或更高版本
- C++14 兼容的编译器

### 硬件要求
- 支持的摄像头设备
- 至少 2GB 内存
- 支持 OpenCV 的操作系统

### 完整版本（高级用户）

完整版本提供更多功能和配置选项：

```bash
# 安装依赖
./scripts/install_dependencies.sh

# 构建完整版本
./scripts/build.sh

# 运行完整版本
./build/FaceDetectionDemo
```

## 编译和运行

### 1. 安装依赖

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install libopencv-dev cmake build-essential
```

#### CentOS/RHEL
```bash
sudo yum install opencv-devel cmake gcc-c++
```

#### macOS
```bash
brew install opencv cmake
```

#### Windows
- 下载并安装 OpenCV
- 安装 Visual Studio 或 MinGW
- 安装 CMake

### 2. 编译项目

```bash
# 克隆或进入项目目录
cd example

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make -j4  # Linux/macOS
# 或在 Windows 上使用 Visual Studio 编译
```

### 3. 运行示例

```bash
# 使用默认摄像头（通常是 /dev/video0 或摄像头 ID 0）
./face_detection_demo

# 指定摄像头设备
./face_detection_demo --camera 1

# 指定摄像头设备文件（Linux）
./face_detection_demo --device /dev/video1

# 显示帮助信息
./face_detection_demo --help
```

## 配置选项

### 命令行参数

| 参数 | 描述 | 默认值 |
|------|------|--------|
| `--camera` | 摄像头ID | 0 |
| `--device` | 设备文件路径（Linux） | /dev/video0 |
| `--width` | 图像宽度 | 640 |
| `--height` | 图像高度 | 480 |
| `--fps` | 目标帧率 | 30 |
| `--scale` | 检测缩放因子 | 1.1 |
| `--neighbors` | 最小邻居数 | 3 |
| `--min-size` | 最小人脸尺寸 | 30 |
| `--show-fps` | 显示FPS | true |
| `--save-video` | 保存视频文件 | false |
| `--output` | 输出文件名 | output.avi |

### 配置文件

可以创建 `config.json` 文件来配置参数：

```json
{
    "camera": {
        "device_id": 0,
        "width": 640,
        "height": 480,
        "fps": 30
    },
    "detection": {
        "scale_factor": 1.1,
        "min_neighbors": 3,
        "min_size": 30,
        "max_size": 300
    },
    "display": {
        "show_fps": true,
        "show_detection_info": true,
        "window_title": "Face Detection Demo"
    }
}
```

## 使用示例

### 基本使用

```cpp
#include "face_detection_demo.h"

int main() {
    FaceDetectionDemo demo;
    
    // 初始化摄像头
    if (!demo.initialize(0)) {
        std::cerr << "Failed to initialize camera" << std::endl;
        return -1;
    }
    
    // 开始检测
    demo.run();
    
    return 0;
}
```

### 自定义配置

```cpp
FaceDetectionConfig config;
config.camera_id = 1;
config.width = 1280;
config.height = 720;
config.scale_factor = 1.2;
config.min_neighbors = 5;

FaceDetectionDemo demo(config);
demo.run();
```

## 性能优化

### 1. 降低分辨率
```bash
./face_detection_demo --width 320 --height 240
```

### 2. 调整检测参数
```bash
./face_detection_demo --scale 1.3 --neighbors 5
```

### 3. 使用多线程处理
程序自动使用异步处理来提高性能。

## 故障排除

### 常见问题

#### 1. 摄像头无法打开
```
Error: Cannot open camera 0
```

**解决方案**：
- 检查摄像头是否被其他程序占用
- 尝试不同的摄像头ID（0, 1, 2...）
- 在Linux上检查设备权限：`ls -la /dev/video*`

#### 2. OpenCV未找到
```
Error: OpenCV not found
```

**解决方案**：
- 确保正确安装了OpenCV开发包
- 检查CMake是否能找到OpenCV：`pkg-config --modversion opencv4`

#### 3. 人脸检测效果差
**解决方案**：
- 调整检测参数（scale_factor, min_neighbors）
- 确保光照条件良好
- 调整摄像头位置和角度

#### 4. 性能问题
**解决方案**：
- 降低输入分辨率
- 调整检测参数减少计算量
- 检查系统资源使用情况

### 调试模式

编译调试版本：
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

启用详细日志：
```bash
./face_detection_demo --verbose
```

## 扩展功能

### 添加人脸识别
可以在检测的基础上添加人脸识别功能：

```cpp
// 在检测到人脸后进行识别
for (const auto& face : detected_faces) {
    cv::Mat face_roi = frame(face);
    std::string person_id = face_recognizer.recognize(face_roi);
    // 处理识别结果
}
```

### 添加网络功能
可以添加网络接口来远程监控：

```cpp
// 启动HTTP服务器
HttpServer server(8080);
server.add_endpoint("/stream", [&](){ return get_video_stream(); });
server.start();
```

## 许可证

本示例工程采用 MIT 许可证。详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个示例工程。

## 联系方式

如有问题，请通过以下方式联系：
- 提交 GitHub Issue
- 发送邮件至：example@domain.com

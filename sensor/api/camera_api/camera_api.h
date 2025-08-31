/*
 * Camera API Header for IMX6ULL Pro
 * 
 * Provides high-level C++ interface for camera operations
 * Abstracts V4L2 and driver details from applications
 * 
 * Author: Camera API Team
 * License: MIT
 */

#ifndef _CAMERA_API_H_
#define _CAMERA_API_H_

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Camera formats
enum CameraFormat {
    CAMERA_FORMAT_MJPEG = 0,
    CAMERA_FORMAT_YUYV,
    CAMERA_FORMAT_RGB24,
    CAMERA_FORMAT_MAX
};

// Camera configuration
struct CameraConfig {
    int device_id;          // Camera device ID (0, 1, 2, ...)
    int width;              // Frame width
    int height;             // Frame height
    int fps;                // Frames per second
    CameraFormat format;    // Pixel format
    int buffer_count;       // Number of buffers (default: 4)
    
    CameraConfig() 
        : device_id(0), width(640), height(480), fps(30)
        , format(CAMERA_FORMAT_MJPEG), buffer_count(4) {}
};

// Camera frame data
struct CameraFrame {
    void* data;             // Frame data pointer
    size_t size;            // Frame data size in bytes
    int width;              // Frame width
    int height;             // Frame height
    CameraFormat format;    // Pixel format
    uint64_t timestamp;     // Frame timestamp (microseconds)
    int sequence;           // Frame sequence number
    
    CameraFrame() 
        : data(nullptr), size(0), width(0), height(0)
        , format(CAMERA_FORMAT_MJPEG), timestamp(0), sequence(0) {}
};

// Camera capabilities
struct CameraCapabilities {
    std::string driver_name;
    std::string card_name;
    std::string bus_info;
    uint32_t capabilities;
    
    std::vector<CameraFormat> supported_formats;
    std::vector<std::pair<int, int>> supported_resolutions;
    std::vector<int> supported_fps;
};

// Frame callback function type
using FrameCallback = std::function<void(const CameraFrame& frame)>;

// Error codes
enum CameraError {
    CAMERA_SUCCESS = 0,
    CAMERA_ERROR_INVALID_PARAM = -1,
    CAMERA_ERROR_DEVICE_NOT_FOUND = -2,
    CAMERA_ERROR_DEVICE_BUSY = -3,
    CAMERA_ERROR_IO_ERROR = -4,
    CAMERA_ERROR_TIMEOUT = -5,
    CAMERA_ERROR_NO_MEMORY = -6,
    CAMERA_ERROR_NOT_SUPPORTED = -7,
    CAMERA_ERROR_SYSTEM_ERROR = -8
};

// Camera API class
class CameraAPI {
public:
    CameraAPI();
    virtual ~CameraAPI();
    
    // Device management
    int Initialize(const CameraConfig& config);
    void Cleanup();
    
    // Camera control
    int Start();
    int Stop();
    bool IsStreaming() const;
    
    // Frame operations
    int GetFrame(CameraFrame& frame);
    int ReleaseFrame(const CameraFrame& frame);
    
    // Asynchronous frame capture
    int SetFrameCallback(FrameCallback callback);
    int StartAsyncCapture();
    int StopAsyncCapture();
    
    // Configuration
    int SetConfig(const CameraConfig& config);
    int GetConfig(CameraConfig& config) const;
    
    // Capabilities
    int GetCapabilities(CameraCapabilities& caps) const;
    
    // Format operations
    int SetFormat(int width, int height, CameraFormat format);
    int GetFormat(int& width, int& height, CameraFormat& format) const;
    
    // Frame rate control
    int SetFrameRate(int fps);
    int GetFrameRate(int& fps) const;
    
    // Camera controls (brightness, contrast, etc.)
    int SetControl(int control_id, int value);
    int GetControl(int control_id, int& value) const;
    
    // Statistics
    struct Statistics {
        uint64_t frames_captured;
        uint64_t frames_dropped;
        uint64_t bytes_received;
        double average_fps;
        uint64_t total_errors;
    };
    
    int GetStatistics(Statistics& stats) const;
    void ResetStatistics();
    
    // Error handling
    std::string GetLastError() const;
    static std::string ErrorToString(CameraError error);
    
    // Device enumeration (static methods)
    static std::vector<std::string> EnumerateDevices();
    static int GetDeviceInfo(const std::string& device_path, CameraCapabilities& caps);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Non-copyable
    CameraAPI(const CameraAPI&) = delete;
    CameraAPI& operator=(const CameraAPI&) = delete;
};

// Utility functions
namespace CameraUtils {
    // Format conversion
    std::string FormatToString(CameraFormat format);
    CameraFormat StringToFormat(const std::string& format_str);
    
    // Frame size calculation
    size_t CalculateFrameSize(int width, int height, CameraFormat format);
    
    // Validation
    bool IsValidResolution(int width, int height);
    bool IsValidFrameRate(int fps);
    bool IsValidFormat(CameraFormat format);
    
    // Performance helpers
    double CalculateFPS(uint64_t frame_count, uint64_t duration_us);
    uint64_t GetTimestampUs();
}

// Camera control IDs (V4L2 compatible)
namespace CameraControls {
    constexpr int BRIGHTNESS = 0x00980900;
    constexpr int CONTRAST = 0x00980901;
    constexpr int SATURATION = 0x00980902;
    constexpr int HUE = 0x00980903;
    constexpr int AUTO_WHITE_BALANCE = 0x0098090c;
    constexpr int GAMMA = 0x00980910;
    constexpr int GAIN = 0x00980913;
    constexpr int POWER_LINE_FREQUENCY = 0x00980918;
    constexpr int WHITE_BALANCE_TEMPERATURE = 0x0098091a;
    constexpr int SHARPNESS = 0x0098091b;
    constexpr int BACKLIGHT_COMPENSATION = 0x0098091c;
    constexpr int AUTO_EXPOSURE = 0x009a0901;
    constexpr int EXPOSURE_TIME_ABSOLUTE = 0x009a0902;
    constexpr int FOCUS_ABSOLUTE = 0x009a090a;
    constexpr int FOCUS_AUTO = 0x009a090c;
    constexpr int ZOOM_ABSOLUTE = 0x009a090d;
}

// Convenience macros
#define CAMERA_CHECK_ERROR(expr) \
    do { \
        int _ret = (expr); \
        if (_ret != CAMERA_SUCCESS) { \
            return _ret; \
        } \
    } while(0)

#define CAMERA_LOG_ERROR(api, msg) \
    do { \
        std::cerr << "Camera API Error: " << msg \
                  << " (" << api.GetLastError() << ")" << std::endl; \
    } while(0)

#endif /* _CAMERA_API_H_ */

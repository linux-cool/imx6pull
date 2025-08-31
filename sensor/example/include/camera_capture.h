/*
 * Camera Capture Header
 * 
 * This header defines the camera capture interface for cross-platform
 * camera access using OpenCV VideoCapture.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include <opencv2/opencv.hpp>
#include <string>
#include <atomic>
#include <mutex>
#include <memory>

// Camera configuration
struct CameraConfig {
    int camera_id = 0;
    std::string device_path;  // For Linux V4L2 devices
    int width = 640;
    int height = 480;
    int fps = 30;
    
    // Advanced settings
    double brightness = -1;    // -1 means auto/default
    double contrast = -1;
    double saturation = -1;
    double gain = -1;
    double exposure = -1;
    bool auto_focus = true;
    
    // Buffer settings
    int buffer_size = 3;
    
    CameraConfig() = default;
    CameraConfig(int id) : camera_id(id) {}
    CameraConfig(const std::string& path) : device_path(path) {}
};

// Camera capabilities
struct CameraCapabilities {
    std::vector<cv::Size> supported_resolutions;
    std::vector<double> supported_fps;
    std::vector<int> supported_formats;
    
    bool supports_brightness_control = false;
    bool supports_contrast_control = false;
    bool supports_saturation_control = false;
    bool supports_gain_control = false;
    bool supports_exposure_control = false;
    bool supports_focus_control = false;
    
    double min_brightness = 0;
    double max_brightness = 255;
    double min_contrast = 0;
    double max_contrast = 255;
    double min_saturation = 0;
    double max_saturation = 255;
};

// Camera frame information
struct CameraFrame {
    cv::Mat image;
    double timestamp;
    int frame_number;
    cv::Size resolution;
    
    CameraFrame() : timestamp(0), frame_number(0) {}
    CameraFrame(const cv::Mat& img, double ts, int fn) 
        : image(img), timestamp(ts), frame_number(fn), resolution(img.size()) {}
    
    bool empty() const { return image.empty(); }
    cv::Size size() const { return image.size(); }
};

// Camera capture class
class CameraCapture {
public:
    CameraCapture();
    ~CameraCapture();
    
    // Initialization and cleanup
    bool initialize(const CameraConfig& config);
    bool initialize(int camera_id);
    bool initialize(const std::string& device_path);
    void cleanup();
    
    // Camera control
    bool start();
    bool stop();
    bool isRunning() const { return running_; }
    bool isInitialized() const { return initialized_; }
    
    // Frame capture
    bool captureFrame(CameraFrame& frame);
    bool captureFrame(cv::Mat& image);
    
    // Configuration
    bool setConfig(const CameraConfig& config);
    const CameraConfig& getConfig() const { return config_; }
    
    // Camera properties
    bool setProperty(int property_id, double value);
    double getProperty(int property_id) const;
    
    // Convenience methods for common properties
    bool setBrightness(double value);
    bool setContrast(double value);
    bool setSaturation(double value);
    bool setGain(double value);
    bool setExposure(double value);
    bool setAutoFocus(bool enable);
    
    double getBrightness() const;
    double getContrast() const;
    double getSaturation() const;
    double getGain() const;
    double getExposure() const;
    bool getAutoFocus() const;
    
    // Resolution and FPS
    bool setResolution(int width, int height);
    bool setFPS(double fps);
    cv::Size getResolution() const;
    double getFPS() const;
    
    // Capabilities
    CameraCapabilities getCapabilities() const;
    bool isPropertySupported(int property_id) const;
    
    // Statistics
    struct Statistics {
        std::atomic<int> frames_captured{0};
        std::atomic<int> frames_dropped{0};
        std::atomic<double> actual_fps{0.0};
        std::atomic<double> average_capture_time{0.0};
    };
    
    const Statistics& getStatistics() const { return stats_; }
    void resetStatistics();
    
    // Error handling
    std::string getLastError() const { return last_error_; }
    
    // Static utility methods
    static std::vector<int> getAvailableCameras();
    static bool isCameraAvailable(int camera_id);
    static std::string getCameraInfo(int camera_id);
    
private:
    // OpenCV VideoCapture
    std::unique_ptr<cv::VideoCapture> cap_;
    
    // Configuration
    CameraConfig config_;
    
    // State
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    
    // Statistics
    mutable Statistics stats_;
    
    // Error handling
    mutable std::string last_error_;
    mutable std::mutex error_mutex_;
    
    // Frame tracking
    std::atomic<int> frame_counter_{0};
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_fps_update_;
    
    // Private methods
    bool openCamera();
    bool configureCamera();
    void updateStatistics();
    void setError(const std::string& error) const;
    
    // Platform-specific methods
    bool openCameraById(int camera_id);
    bool openCameraByPath(const std::string& device_path);
    
    // Property validation
    bool validateProperty(int property_id, double value) const;
    double clampProperty(int property_id, double value) const;
    
    // Non-copyable
    CameraCapture(const CameraCapture&) = delete;
    CameraCapture& operator=(const CameraCapture&) = delete;
};

// Utility functions
namespace CameraUtils {
    // Camera enumeration
    std::vector<CameraConfig> enumerateAvailableCameras();
    
    // Resolution utilities
    std::vector<cv::Size> getCommonResolutions();
    cv::Size findBestResolution(const std::vector<cv::Size>& available, 
                               const cv::Size& desired);
    
    // FPS utilities
    std::vector<double> getCommonFPS();
    double findBestFPS(const std::vector<double>& available, double desired);
    
    // Format conversion
    std::string propertyIdToString(int property_id);
    int stringToPropertyId(const std::string& property_name);
    
    // Validation
    bool isValidResolution(const cv::Size& resolution);
    bool isValidFPS(double fps);
    
    // Platform detection
    bool isLinux();
    bool isWindows();
    bool isMacOS();
    
    // V4L2 specific utilities (Linux only)
#ifdef __linux__
    std::vector<std::string> getV4L2Devices();
    bool isV4L2Device(const std::string& device_path);
    std::string getV4L2DeviceInfo(const std::string& device_path);
#endif
}

// Constants
namespace CameraConstants {
    // Common resolutions
    const cv::Size RESOLUTION_QVGA(320, 240);
    const cv::Size RESOLUTION_VGA(640, 480);
    const cv::Size RESOLUTION_HD(1280, 720);
    const cv::Size RESOLUTION_FHD(1920, 1080);
    
    // Common FPS values
    constexpr double FPS_15 = 15.0;
    constexpr double FPS_30 = 30.0;
    constexpr double FPS_60 = 60.0;
    
    // Property ranges
    constexpr double BRIGHTNESS_MIN = 0.0;
    constexpr double BRIGHTNESS_MAX = 255.0;
    constexpr double CONTRAST_MIN = 0.0;
    constexpr double CONTRAST_MAX = 255.0;
    constexpr double SATURATION_MIN = 0.0;
    constexpr double SATURATION_MAX = 255.0;
    
    // Timeouts
    constexpr int CAPTURE_TIMEOUT_MS = 5000;
    constexpr int INIT_TIMEOUT_MS = 10000;
}

#endif // CAMERA_CAPTURE_H

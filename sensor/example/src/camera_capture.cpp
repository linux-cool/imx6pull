/*
 * Camera Capture Implementation
 * 
 * This file implements the camera capture functionality using OpenCV VideoCapture.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#include "camera_capture.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

CameraCapture::CameraCapture() {
    cap_ = std::make_unique<cv::VideoCapture>();
}

CameraCapture::~CameraCapture() {
    cleanup();
}

bool CameraCapture::initialize(const CameraConfig& config) {
    config_ = config;
    return openCamera() && configureCamera();
}

bool CameraCapture::initialize(int camera_id) {
    CameraConfig config(camera_id);
    return initialize(config);
}

bool CameraCapture::initialize(const std::string& device_path) {
    CameraConfig config(device_path);
    return initialize(config);
}

void CameraCapture::cleanup() {
    stop();
    if (cap_ && cap_->isOpened()) {
        cap_->release();
    }
    initialized_ = false;
}

bool CameraCapture::start() {
    if (!initialized_) {
        setError("Camera not initialized");
        return false;
    }
    
    if (running_) {
        return true;
    }
    
    running_ = true;
    start_time_ = std::chrono::steady_clock::now();
    last_fps_update_ = start_time_;
    frame_counter_ = 0;
    
    return true;
}

bool CameraCapture::stop() {
    running_ = false;
    return true;
}

bool CameraCapture::captureFrame(CameraFrame& frame) {
    if (!running_ || !cap_ || !cap_->isOpened()) {
        setError("Camera not running or not opened");
        return false;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    cv::Mat image;
    if (!cap_->read(image)) {
        setError("Failed to read frame from camera");
        stats_.frames_dropped++;
        return false;
    }
    
    if (image.empty()) {
        setError("Captured frame is empty");
        stats_.frames_dropped++;
        return false;
    }
    
    // Create frame info
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    frame.image = image;
    frame.timestamp = duration.count() / 1000.0; // Convert to milliseconds
    frame.frame_number = frame_counter_++;
    frame.resolution = image.size();
    
    stats_.frames_captured++;
    updateStatistics();
    
    return true;
}

bool CameraCapture::captureFrame(cv::Mat& image) {
    CameraFrame frame;
    if (captureFrame(frame)) {
        image = frame.image;
        return true;
    }
    return false;
}

bool CameraCapture::setConfig(const CameraConfig& config) {
    config_ = config;
    if (initialized_) {
        return configureCamera();
    }
    return true;
}

bool CameraCapture::setProperty(int property_id, double value) {
    if (!cap_ || !cap_->isOpened()) {
        setError("Camera not opened");
        return false;
    }
    
    if (!validateProperty(property_id, value)) {
        setError("Invalid property value");
        return false;
    }
    
    value = clampProperty(property_id, value);
    return cap_->set(property_id, value);
}

double CameraCapture::getProperty(int property_id) const {
    if (!cap_ || !cap_->isOpened()) {
        return -1.0;
    }
    
    return cap_->get(property_id);
}

bool CameraCapture::setBrightness(double value) {
    return setProperty(cv::CAP_PROP_BRIGHTNESS, value);
}

bool CameraCapture::setContrast(double value) {
    return setProperty(cv::CAP_PROP_CONTRAST, value);
}

bool CameraCapture::setSaturation(double value) {
    return setProperty(cv::CAP_PROP_SATURATION, value);
}

bool CameraCapture::setGain(double value) {
    return setProperty(cv::CAP_PROP_GAIN, value);
}

bool CameraCapture::setExposure(double value) {
    return setProperty(cv::CAP_PROP_EXPOSURE, value);
}

bool CameraCapture::setAutoFocus(bool enable) {
    return setProperty(cv::CAP_PROP_AUTOFOCUS, enable ? 1.0 : 0.0);
}

double CameraCapture::getBrightness() const {
    return getProperty(cv::CAP_PROP_BRIGHTNESS);
}

double CameraCapture::getContrast() const {
    return getProperty(cv::CAP_PROP_CONTRAST);
}

double CameraCapture::getSaturation() const {
    return getProperty(cv::CAP_PROP_SATURATION);
}

double CameraCapture::getGain() const {
    return getProperty(cv::CAP_PROP_GAIN);
}

double CameraCapture::getExposure() const {
    return getProperty(cv::CAP_PROP_EXPOSURE);
}

bool CameraCapture::getAutoFocus() const {
    return getProperty(cv::CAP_PROP_AUTOFOCUS) > 0.5;
}

bool CameraCapture::setResolution(int width, int height) {
    if (!cap_ || !cap_->isOpened()) {
        setError("Camera not opened");
        return false;
    }
    
    bool success = cap_->set(cv::CAP_PROP_FRAME_WIDTH, width) &&
                   cap_->set(cv::CAP_PROP_FRAME_HEIGHT, height);
    
    if (success) {
        config_.width = width;
        config_.height = height;
    }
    
    return success;
}

bool CameraCapture::setFPS(double fps) {
    if (!cap_ || !cap_->isOpened()) {
        setError("Camera not opened");
        return false;
    }
    
    bool success = cap_->set(cv::CAP_PROP_FPS, fps);
    if (success) {
        config_.fps = fps;
    }
    
    return success;
}

cv::Size CameraCapture::getResolution() const {
    if (!cap_ || !cap_->isOpened()) {
        return cv::Size(0, 0);
    }
    
    int width = static_cast<int>(cap_->get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap_->get(cv::CAP_PROP_FRAME_HEIGHT));
    
    return cv::Size(width, height);
}

double CameraCapture::getFPS() const {
    if (!cap_ || !cap_->isOpened()) {
        return 0.0;
    }
    
    return cap_->get(cv::CAP_PROP_FPS);
}

CameraCapabilities CameraCapture::getCapabilities() const {
    CameraCapabilities caps;
    
    if (!cap_ || !cap_->isOpened()) {
        return caps;
    }
    
    // Get common resolutions that might be supported
    std::vector<cv::Size> test_resolutions = CameraUtils::getCommonResolutions();
    
    for (const auto& res : test_resolutions) {
        // Test if resolution is supported (this is a simplified check)
        caps.supported_resolutions.push_back(res);
    }
    
    // Get common FPS values
    caps.supported_fps = CameraUtils::getCommonFPS();
    
    // Check property support
    caps.supports_brightness_control = isPropertySupported(cv::CAP_PROP_BRIGHTNESS);
    caps.supports_contrast_control = isPropertySupported(cv::CAP_PROP_CONTRAST);
    caps.supports_saturation_control = isPropertySupported(cv::CAP_PROP_SATURATION);
    caps.supports_gain_control = isPropertySupported(cv::CAP_PROP_GAIN);
    caps.supports_exposure_control = isPropertySupported(cv::CAP_PROP_EXPOSURE);
    caps.supports_focus_control = isPropertySupported(cv::CAP_PROP_AUTOFOCUS);
    
    return caps;
}

bool CameraCapture::isPropertySupported(int property_id) const {
    if (!cap_ || !cap_->isOpened()) {
        return false;
    }
    
    // Try to get the property value
    double value = cap_->get(property_id);
    return value >= 0; // Negative values usually indicate unsupported properties
}

void CameraCapture::resetStatistics() {
    stats_.frames_captured = 0;
    stats_.frames_dropped = 0;
    stats_.actual_fps = 0.0;
    stats_.average_capture_time = 0.0;
}

std::vector<int> CameraCapture::getAvailableCameras() {
    std::vector<int> cameras;
    
    for (int i = 0; i < 10; ++i) { // Test first 10 camera indices
        cv::VideoCapture test_cap(i);
        if (test_cap.isOpened()) {
            cameras.push_back(i);
            test_cap.release();
        }
    }
    
    return cameras;
}

bool CameraCapture::isCameraAvailable(int camera_id) {
    cv::VideoCapture test_cap(camera_id);
    bool available = test_cap.isOpened();
    if (available) {
        test_cap.release();
    }
    return available;
}

std::string CameraCapture::getCameraInfo(int camera_id) {
    cv::VideoCapture test_cap(camera_id);
    if (!test_cap.isOpened()) {
        return "Camera not available";
    }
    
    int width = static_cast<int>(test_cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(test_cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = test_cap.get(cv::CAP_PROP_FPS);
    
    test_cap.release();
    
    return "Resolution: " + std::to_string(width) + "x" + std::to_string(height) +
           ", FPS: " + std::to_string(fps);
}

bool CameraCapture::openCamera() {
    if (!config_.device_path.empty()) {
        return openCameraByPath(config_.device_path);
    } else {
        return openCameraById(config_.camera_id);
    }
}

bool CameraCapture::configureCamera() {
    if (!cap_ || !cap_->isOpened()) {
        setError("Camera not opened");
        return false;
    }

    // Set resolution
    if (!setResolution(config_.width, config_.height)) {
        setError("Failed to set resolution");
        return false;
    }

    // Set FPS
    if (!setFPS(config_.fps)) {
        setError("Failed to set FPS");
        // Don't return false here as FPS setting often fails but camera still works
    }

    // Set buffer size
    cap_->set(cv::CAP_PROP_BUFFERSIZE, config_.buffer_size);

    // Set optional properties if specified
    if (config_.brightness >= 0) {
        setBrightness(config_.brightness);
    }
    if (config_.contrast >= 0) {
        setContrast(config_.contrast);
    }
    if (config_.saturation >= 0) {
        setSaturation(config_.saturation);
    }
    if (config_.gain >= 0) {
        setGain(config_.gain);
    }
    if (config_.exposure >= 0) {
        setExposure(config_.exposure);
    }

    setAutoFocus(config_.auto_focus);

    initialized_ = true;
    return true;
}

void CameraCapture::updateStatistics() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_update_);

    if (duration.count() >= 1000) { // Update every second
        int frames_in_period = stats_.frames_captured.load() -
                              (stats_.frames_captured.load() - frame_counter_.load());
        stats_.actual_fps = frames_in_period * 1000.0 / duration.count();
        last_fps_update_ = now;
    }
}

void CameraCapture::setError(const std::string& error) const {
    std::lock_guard<std::mutex> lock(error_mutex_);
    last_error_ = error;
}

bool CameraCapture::openCameraById(int camera_id) {
    if (!cap_->open(camera_id)) {
        setError("Failed to open camera " + std::to_string(camera_id));
        return false;
    }

    return true;
}

bool CameraCapture::openCameraByPath(const std::string& device_path) {
    if (!cap_->open(device_path)) {
        setError("Failed to open camera device: " + device_path);
        return false;
    }

    return true;
}

bool CameraCapture::validateProperty(int property_id, double value) const {
    // Basic validation - this could be expanded
    switch (property_id) {
    case cv::CAP_PROP_BRIGHTNESS:
    case cv::CAP_PROP_CONTRAST:
    case cv::CAP_PROP_SATURATION:
        return value >= 0.0 && value <= 255.0;
    case cv::CAP_PROP_GAIN:
        return value >= 0.0;
    case cv::CAP_PROP_EXPOSURE:
        return true; // Exposure can have various ranges
    case cv::CAP_PROP_AUTOFOCUS:
        return value == 0.0 || value == 1.0;
    default:
        return true;
    }
}

double CameraCapture::clampProperty(int property_id, double value) const {
    switch (property_id) {
    case cv::CAP_PROP_BRIGHTNESS:
    case cv::CAP_PROP_CONTRAST:
    case cv::CAP_PROP_SATURATION:
        return std::max(0.0, std::min(255.0, value));
    case cv::CAP_PROP_GAIN:
        return std::max(0.0, value);
    case cv::CAP_PROP_AUTOFOCUS:
        return value > 0.5 ? 1.0 : 0.0;
    default:
        return value;
    }
}

// CameraUtils namespace implementation
namespace CameraUtils {

std::vector<CameraConfig> enumerateAvailableCameras() {
    std::vector<CameraConfig> cameras;

    for (int i = 0; i < 10; ++i) {
        if (CameraCapture::isCameraAvailable(i)) {
            CameraConfig config(i);
            cameras.push_back(config);
        }
    }

    return cameras;
}

std::vector<cv::Size> getCommonResolutions() {
    return {
        CameraConstants::RESOLUTION_QVGA,
        CameraConstants::RESOLUTION_VGA,
        CameraConstants::RESOLUTION_HD,
        CameraConstants::RESOLUTION_FHD
    };
}

cv::Size findBestResolution(const std::vector<cv::Size>& available, const cv::Size& desired) {
    if (available.empty()) {
        return desired;
    }

    cv::Size best = available[0];
    int best_diff = abs(desired.width - best.width) + abs(desired.height - best.height);

    for (const auto& res : available) {
        int diff = abs(desired.width - res.width) + abs(desired.height - res.height);
        if (diff < best_diff) {
            best = res;
            best_diff = diff;
        }
    }

    return best;
}

std::vector<double> getCommonFPS() {
    return {CameraConstants::FPS_15, CameraConstants::FPS_30, CameraConstants::FPS_60};
}

double findBestFPS(const std::vector<double>& available, double desired) {
    if (available.empty()) {
        return desired;
    }

    double best = available[0];
    double best_diff = abs(desired - best);

    for (double fps : available) {
        double diff = abs(desired - fps);
        if (diff < best_diff) {
            best = fps;
            best_diff = diff;
        }
    }

    return best;
}

bool isValidResolution(const cv::Size& resolution) {
    return resolution.width > 0 && resolution.height > 0 &&
           resolution.width <= 4096 && resolution.height <= 4096;
}

bool isValidFPS(double fps) {
    return fps > 0.0 && fps <= 120.0;
}

bool isLinux() {
#ifdef __linux__
    return true;
#else
    return false;
#endif
}

bool isWindows() {
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

bool isMacOS() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

} // namespace CameraUtils

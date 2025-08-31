/*
 * Face Detection Demo Implementation
 * 
 * This file implements the main application class for face detection demo.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#include "face_detection_demo.h"
#include "camera_capture.h"
#include "face_detector.h"
#include "performance_monitor.h"
#include "config_manager.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>

FaceDetectionDemo::FaceDetectionDemo() {
    // Initialize with default configuration
}

FaceDetectionDemo::FaceDetectionDemo(const FaceDetectionConfig& config) 
    : config_(config) {
}

FaceDetectionDemo::~FaceDetectionDemo() {
    cleanup();
}

bool FaceDetectionDemo::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        // Initialize camera
        if (!initializeCamera()) {
            handleError("Failed to initialize camera");
            return false;
        }
        
        // Initialize face detector
        if (!initializeFaceDetector()) {
            handleError("Failed to initialize face detector");
            return false;
        }
        
        // Initialize video writer if needed
        if (config_.save_video && !initializeVideoWriter()) {
            handleError("Failed to initialize video writer");
            return false;
        }
        
        // Initialize performance monitor
        if (config_.enable_performance_monitor) {
            performance_monitor_ = std::make_unique<PerformanceMonitor>();
        }
        
        // Initialize config manager
        config_manager_ = std::make_unique<ConfigManager>();
        
        initialized_ = true;
        
        if (config_.verbose) {
            std::cout << "Face detection demo initialized successfully" << std::endl;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        handleError(std::string("Initialization failed: ") + e.what());
        return false;
    }
}

bool FaceDetectionDemo::initialize(int camera_id) {
    config_.camera_id = camera_id;
    config_.device_path.clear();
    return initialize();
}

bool FaceDetectionDemo::initialize(const std::string& device_path) {
    config_.device_path = device_path;
    return initialize();
}

void FaceDetectionDemo::cleanup() {
    if (!initialized_) {
        return;
    }
    
    // Stop processing
    stop();
    
    // Wait for threads to finish
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    if (process_thread_.joinable()) {
        process_thread_.join();
    }
    
    // Cleanup components
    camera_.reset();
    detector_.reset();
    performance_monitor_.reset();
    config_manager_.reset();
    video_writer_.reset();
    
    initialized_ = false;
    
    if (config_.verbose) {
        std::cout << "Face detection demo cleaned up" << std::endl;
    }
}

int FaceDetectionDemo::run() {
    if (!initialized_) {
        std::cerr << "Demo not initialized" << std::endl;
        return -1;
    }
    
    running_ = true;

    // Initialize display window with config size
    cv::namedWindow(config_.window_title, cv::WINDOW_NORMAL);
    cv::resizeWindow(config_.window_title, config_.width, config_.height);

    try {
        // Start camera
        if (!camera_->start()) {
            handleError("Failed to start camera");
            return -1;
        }
        
        // Start threads
        if (config_.enable_multithreading) {
            capture_thread_ = std::thread(&FaceDetectionDemo::captureLoop, this);
            process_thread_ = std::thread(&FaceDetectionDemo::processLoop, this);

            // Main thread handles display
            while (running_) {
                std::pair<cv::Mat, std::vector<FaceDetectionResult>> result;

                // Get result from queue
                {
                    std::unique_lock<std::mutex> lock(result_mutex_);
                    if (result_cv_.wait_for(lock, std::chrono::milliseconds(10),
                                          [this] { return !result_queue_.empty() || !running_; })) {
                        if (!running_) break;

                        if (!result_queue_.empty()) {
                            result = result_queue_.front();
                            result_queue_.pop();
                        }
                    }
                }

                if (!result.first.empty()) {
                    static int display_count = 0;
                    display_count++;
                    if (display_count % 30 == 0) {
                        std::cout << "Displaying frame " << display_count << std::endl;
                    }

                    cv::Mat display_frame = result.first.clone();
                    drawDetectionResults(display_frame, result.second);

                    // Show frame
                    cv::imshow(config_.window_title, display_frame);

                    // Handle key press
                    int key = cv::waitKey(1) & 0xFF;
                    if (key == 27 || key == 'q') { // ESC or 'q'
                        stop();
                        break;
                    }
                }
            }

            // Wait for threads to finish
            if (capture_thread_.joinable()) capture_thread_.join();
            if (process_thread_.joinable()) process_thread_.join();

            // Clean up OpenCV windows
            cv::destroyAllWindows();
        } else {
            // Single-threaded processing
            while (running_) {
                CameraFrame frame;
                if (camera_->captureFrame(frame)) {
                    processFrame(frame.image);
                    
                    // Simple display
                    cv::imshow(config_.window_title, frame.image);
                    
                    int key = cv::waitKey(1) & 0xFF;
                    if (key == 27 || key == 'q') { // ESC or 'q'
                        break;
                    }
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        handleError(std::string("Runtime error: ") + e.what());
        return -1;
    }
}

void FaceDetectionDemo::stop() {
    running_ = false;
    
    // Notify all waiting threads
    frame_cv_.notify_all();
    result_cv_.notify_all();
    
    if (camera_) {
        camera_->stop();
    }
}

void FaceDetectionDemo::setConfig(const FaceDetectionConfig& config) {
    config_ = config;
}

const FaceDetectionConfig& FaceDetectionDemo::getConfig() const {
    return config_;
}

bool FaceDetectionDemo::loadConfigFromFile(const std::string& filename) {
    if (!config_manager_) {
        config_manager_ = std::make_unique<ConfigManager>();
    }
    
    return config_manager_->loadConfig(filename, config_);
}

bool FaceDetectionDemo::saveConfigToFile(const std::string& filename) const {
    if (!config_manager_) {
        return false;
    }
    
    return config_manager_->saveConfig(filename, config_);
}

void FaceDetectionDemo::resetStatistics() {
    stats_.frames_processed = 0;
    stats_.faces_detected = 0;
    stats_.frames_dropped = 0;
    stats_.average_fps = 0.0;
    stats_.average_detection_time = 0.0;
}

void FaceDetectionDemo::printStatistics() const {
    std::cout << "=== Face Detection Statistics ===" << std::endl;
    std::cout << "Frames processed: " << stats_.frames_processed.load() << std::endl;
    std::cout << "Faces detected: " << stats_.faces_detected.load() << std::endl;
    std::cout << "Frames dropped: " << stats_.frames_dropped.load() << std::endl;
    std::cout << "Average FPS: " << formatFPS(stats_.average_fps.load()) << std::endl;
    std::cout << "Average detection time: " << formatTime(stats_.average_detection_time.load()) << std::endl;
    
    if (camera_) {
        const auto& cam_stats = camera_->getStatistics();
        std::cout << "Camera frames captured: " << cam_stats.frames_captured.load() << std::endl;
        std::cout << "Camera frames dropped: " << cam_stats.frames_dropped.load() << std::endl;
        std::cout << "Camera actual FPS: " << formatFPS(cam_stats.actual_fps.load()) << std::endl;
    }
    
    if (detector_) {
        const auto& det_stats = detector_->getStatistics();
        std::cout << "Detector frames processed: " << det_stats.frames_processed.load() << std::endl;
        std::cout << "Total detections: " << det_stats.total_detections.load() << std::endl;
        std::cout << "Average faces per frame: " << det_stats.average_faces_per_frame.load() << std::endl;
    }
    
    std::cout << "=================================" << std::endl;
}

void FaceDetectionDemo::setFaceDetectionCallback(FaceDetectionCallback callback) {
    face_callback_ = callback;
}

bool FaceDetectionDemo::initializeCamera() {
    bool camera_initialized = false;

    std::cout << "Attempting to initialize camera..." << std::endl;
    std::cout << "Using resolution: " << config_.width << "x" << config_.height << std::endl;

    // Method 1: Try with original config (camera ID or device path)
    camera_ = std::make_unique<CameraCapture>();
    CameraConfig cam_config;
    cam_config.camera_id = config_.camera_id;
    cam_config.device_path = config_.device_path;
    cam_config.width = config_.width;
    cam_config.height = config_.height;
    cam_config.fps = config_.fps;

    if (camera_->initialize(cam_config)) {
        camera_initialized = true;
        if (config_.verbose) {
            std::cout << "Successfully initialized camera using original config" << std::endl;
        }
    } else {
        std::cout << "Original config failed, trying fallback methods..." << std::endl;

        // Method 2: Try camera ID 0
        std::cout << "Trying camera ID 0..." << std::endl;
        camera_ = std::make_unique<CameraCapture>();
        if (camera_->initialize(0)) {
            if (camera_->setResolution(config_.width, config_.height)) {
                camera_initialized = true;
                std::cout << "Successfully initialized camera using ID 0 with custom resolution" << std::endl;
            } else {
                std::cout << "Camera 0 initialized but failed to set resolution, using default" << std::endl;
                camera_initialized = true;
            }
        } else {
            std::cout << "Camera ID 0 failed, trying device path..." << std::endl;

            // Method 3: Try /dev/video0 explicitly
            std::cout << "Trying /dev/video0..." << std::endl;
            camera_ = std::make_unique<CameraCapture>();
            if (camera_->initialize("/dev/video0")) {
                if (camera_->setResolution(config_.width, config_.height)) {
                    camera_initialized = true;
                    std::cout << "Successfully initialized camera using /dev/video0 with custom resolution" << std::endl;
                } else {
                    std::cout << "Camera /dev/video0 initialized but failed to set resolution, using default" << std::endl;
                    camera_initialized = true;
                }
            } else {
                std::cout << "Failed to initialize /dev/video0, trying other camera IDs..." << std::endl;

                // Method 4: Try different camera IDs
                for (int i = 1; i <= 3; ++i) {
                    std::cout << "Trying camera ID " << i << "..." << std::endl;
                    camera_ = std::make_unique<CameraCapture>();
                    if (camera_->initialize(i)) {
                        if (camera_->setResolution(config_.width, config_.height)) {
                            camera_initialized = true;
                            std::cout << "Successfully initialized camera using ID " << i << " with custom resolution" << std::endl;
                        } else {
                            std::cout << "Camera " << i << " initialized but failed to set resolution, using default" << std::endl;
                            camera_initialized = true;
                        }
                        break;
                    }
                }
            }
        }
    }

    if (!camera_initialized) {
        std::cerr << "Camera initialization failed: Failed to initialize camera with all methods" << std::endl;
        return false;
    }

    if (config_.verbose) {
        std::cout << "Camera initialized: " << config_.width << "x" << config_.height
                  << "@" << config_.fps << "fps" << std::endl;
    }

    return true;
}

bool FaceDetectionDemo::initializeFaceDetector() {
    detector_ = std::make_unique<FaceDetector>();
    
    FaceDetectorConfig det_config;
    det_config.method = FaceDetectorConfig::HAAR_CASCADE;
    det_config.scale_factor = config_.scale_factor;
    det_config.min_neighbors = config_.min_neighbors;
    det_config.min_size = config_.min_size;
    det_config.max_size = config_.max_size;
    
    if (!detector_->initialize(det_config)) {
        std::cerr << "Face detector initialization failed: " << detector_->getLastError() << std::endl;
        return false;
    }
    
    if (config_.verbose) {
        std::cout << "Face detector initialized with Haar cascade" << std::endl;
    }
    
    return true;
}

bool FaceDetectionDemo::initializeVideoWriter() {
    if (!config_.save_video) {
        return true;
    }
    
    video_writer_ = std::make_unique<cv::VideoWriter>();
    
    bool success = video_writer_->open(
        config_.output_filename,
        config_.output_fourcc,
        config_.fps,
        cv::Size(config_.width, config_.height)
    );
    
    if (!success) {
        std::cerr << "Failed to open video writer for: " << config_.output_filename << std::endl;
        return false;
    }
    
    if (config_.verbose) {
        std::cout << "Video writer initialized: " << config_.output_filename << std::endl;
    }
    
    return true;
}

void FaceDetectionDemo::captureLoop() {
    if (config_.verbose) {
        std::cout << "Capture thread started" << std::endl;
    }

    int frame_count = 0;
    int failed_count = 0;

    while (running_) {
        CameraFrame frame;
        if (camera_->captureFrame(frame)) {
            frame_count++;
            if (frame_count % 30 == 0) { // Log every 30 frames
                std::cout << "Successfully captured " << frame_count << " frames" << std::endl;
            }

            // Add frame to processing queue
            {
                std::unique_lock<std::mutex> lock(frame_mutex_);

                // Limit queue size to prevent memory issues
                while (frame_queue_.size() >= config_.max_queue_size && running_) {
                    frame_queue_.pop(); // Drop oldest frame
                    stats_.frames_dropped++;
                }

                if (running_) {
                    frame_queue_.push(frame.image.clone());
                }
            }

            frame_cv_.notify_one();
        } else {
            failed_count++;
            if (failed_count % 100 == 0) { // Log every 100 failures
                std::cout << "Camera capture failed " << failed_count << " times" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    if (config_.verbose) {
        std::cout << "Capture thread stopped" << std::endl;
    }
}

void FaceDetectionDemo::processLoop() {
    if (config_.verbose) {
        std::cout << "Process thread started" << std::endl;
    }
    
    while (running_) {
        cv::Mat frame;
        
        // Get frame from queue
        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            frame_cv_.wait(lock, [this] { return !frame_queue_.empty() || !running_; });
            
            if (!running_) break;
            
            if (!frame_queue_.empty()) {
                frame = frame_queue_.front();
                frame_queue_.pop();
            }
        }
        
        if (!frame.empty()) {
            static int process_count = 0;
            process_count++;
            if (process_count % 30 == 0) {
                std::cout << "Processing frame " << process_count << std::endl;
            }
            processFrame(frame);
        }
    }
    
    if (config_.verbose) {
        std::cout << "Process thread stopped" << std::endl;
    }
}



void FaceDetectionDemo::processFrame(const cv::Mat& frame) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Detect faces
    std::vector<FaceDetection> detections = detector_->detectFaces(frame);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Convert to result format
    std::vector<FaceDetectionResult> results;
    for (const auto& detection : detections) {
        FaceDetectionResult result(detection.bbox);
        result.confidence = detection.confidence;
        results.push_back(result);
    }
    
    // Update statistics
    updateStatistics(results.size(), duration.count());
    
    // Call callback if set
    if (face_callback_) {
        face_callback_(results);
    }
    
    // Add to display queue
    if (config_.enable_multithreading) {
        std::unique_lock<std::mutex> lock(result_mutex_);
        result_queue_.push(std::make_pair(frame.clone(), results));
        static int queue_count = 0;
        queue_count++;
        if (queue_count % 30 == 0) {
            std::cout << "Added " << queue_count << " results to display queue" << std::endl;
        }
        result_cv_.notify_one();
    }
}

void FaceDetectionDemo::drawDetectionResults(cv::Mat& frame, const std::vector<FaceDetectionResult>& results) {
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        cv::Scalar color = getDetectionColor(i);
        
        // Draw bounding box
        cv::rectangle(frame, result.bbox, color, 2);
        
        // Draw face center
        cv::circle(frame, result.center, 3, color, -1);
        
        // Draw confidence if enabled
        if (config_.show_confidence && result.confidence < 1.0) {
            std::string conf_text = cv::format("%.2f", result.confidence);
            cv::putText(frame, conf_text, 
                       cv::Point(result.bbox.x, result.bbox.y - 5),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
        }
    }
    
    // Draw FPS if enabled
    if (config_.show_fps) {
        std::string fps_text = "FPS: " + formatFPS(stats_.average_fps.load());
        cv::putText(frame, fps_text, cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
    }
    
    // Draw detection info if enabled
    if (config_.show_detection_info) {
        std::string info_text = cv::format("Faces: %zu", results.size());
        cv::putText(frame, info_text, cv::Point(10, 60),
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
    }
}

void FaceDetectionDemo::updateStatistics(int face_count, double detection_time) {
    stats_.frames_processed++;
    stats_.faces_detected += face_count;
    
    // Update average detection time
    static double total_detection_time = 0.0;
    total_detection_time += detection_time;
    stats_.average_detection_time = total_detection_time / stats_.frames_processed.load();
    
    // Update FPS (calculate every second)
    static auto last_fps_update = std::chrono::steady_clock::now();
    static int frames_since_last_update = 0;
    
    frames_since_last_update++;
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_update);
    
    if (duration.count() >= 1000) { // Update every second
        stats_.average_fps = frames_since_last_update * 1000.0 / duration.count();
        frames_since_last_update = 0;
        last_fps_update = now;
    }
}

std::string FaceDetectionDemo::formatFPS(double fps) const {
    return cv::format("%.1f", fps);
}

std::string FaceDetectionDemo::formatTime(double time_ms) const {
    return cv::format("%.1f ms", time_ms);
}

cv::Scalar FaceDetectionDemo::getDetectionColor(size_t index) const {
    const std::vector<cv::Scalar> colors = {
        cv::Scalar(0, 255, 0),    // Green
        cv::Scalar(0, 0, 255),    // Red
        cv::Scalar(255, 0, 0),    // Blue
        cv::Scalar(0, 255, 255),  // Yellow
        cv::Scalar(255, 0, 255),  // Magenta
        cv::Scalar(255, 255, 0),  // Cyan
    };
    
    return colors[index % colors.size()];
}

void FaceDetectionDemo::handleError(const std::string& error_msg) {
    std::cerr << "Error: " << error_msg << std::endl;
    
    if (config_.verbose) {
        // Additional error context could be added here
    }
}

bool FaceDetectionDemo::checkSystemResources() {
    // Basic system resource check
    // This could be expanded to check memory, CPU, etc.
    return true;
}

// FaceDetectionUtils namespace implementation
namespace FaceDetectionUtils {

FaceDetectionConfig parseCommandLineArgs(int argc, char* argv[]) {
    // This is implemented in main.cpp
    return FaceDetectionConfig();
}

void printUsage(const std::string& program_name) {
    // This is implemented in main.cpp
}

std::vector<int> getAvailableCameras() {
    return CameraCapture::getAvailableCameras();
}

bool isCameraAvailable(int camera_id) {
    return CameraCapture::isCameraAvailable(camera_id);
}

std::string getSystemInfo() {
    std::ostringstream info;

    info << "System Information:\n";

    // Platform information
#ifdef __linux__
    info << "  Platform: Linux\n";
#elif defined(_WIN32)
    info << "  Platform: Windows\n";
#elif defined(__APPLE__)
    info << "  Platform: macOS\n";
#else
    info << "  Platform: Unknown\n";
#endif

    // Hardware information
    info << "  CPU Cores: " << std::thread::hardware_concurrency() << "\n";

    // OpenCV information
    info << "  OpenCV Version: " << CV_VERSION << "\n";

    // Available cameras
    auto cameras = getAvailableCameras();
    info << "  Available Cameras: ";
    if (cameras.empty()) {
        info << "None\n";
    } else {
        for (size_t i = 0; i < cameras.size(); ++i) {
            if (i > 0) info << ", ";
            info << cameras[i];
        }
        info << "\n";
    }

    return info.str();
}

cv::Mat resizeKeepAspectRatio(const cv::Mat& image, const cv::Size& target_size) {
    if (image.empty()) {
        return cv::Mat();
    }

    double scale = std::min(
        static_cast<double>(target_size.width) / image.cols,
        static_cast<double>(target_size.height) / image.rows
    );

    cv::Size new_size(
        static_cast<int>(image.cols * scale),
        static_cast<int>(image.rows * scale)
    );

    cv::Mat resized;
    cv::resize(image, resized, new_size);

    // Create output image with target size and center the resized image
    cv::Mat result = cv::Mat::zeros(target_size, image.type());

    int x_offset = (target_size.width - new_size.width) / 2;
    int y_offset = (target_size.height - new_size.height) / 2;

    cv::Rect roi(x_offset, y_offset, new_size.width, new_size.height);
    resized.copyTo(result(roi));

    return result;
}

cv::Rect expandRect(const cv::Rect& rect, double factor, const cv::Size& image_size) {
    if (factor <= 1.0) {
        return rect;
    }

    int new_width = static_cast<int>(rect.width * factor);
    int new_height = static_cast<int>(rect.height * factor);

    int x_offset = (new_width - rect.width) / 2;
    int y_offset = (new_height - rect.height) / 2;

    cv::Rect expanded(
        rect.x - x_offset,
        rect.y - y_offset,
        new_width,
        new_height
    );

    // Clamp to image boundaries
    expanded.x = std::max(0, expanded.x);
    expanded.y = std::max(0, expanded.y);
    expanded.width = std::min(expanded.width, image_size.width - expanded.x);
    expanded.height = std::min(expanded.height, image_size.height - expanded.y);

    return expanded;
}

double calculateIoU(const cv::Rect& rect1, const cv::Rect& rect2) {
    int x1 = std::max(rect1.x, rect2.x);
    int y1 = std::max(rect1.y, rect2.y);
    int x2 = std::min(rect1.x + rect1.width, rect2.x + rect2.width);
    int y2 = std::min(rect1.y + rect1.height, rect2.y + rect2.height);

    if (x2 <= x1 || y2 <= y1) {
        return 0.0;
    }

    int intersection = (x2 - x1) * (y2 - y1);
    int union_area = rect1.area() + rect2.area() - intersection;

    return static_cast<double>(intersection) / union_area;
}

void filterOverlappingDetections(std::vector<FaceDetectionResult>& detections, double iou_threshold) {
    if (detections.size() <= 1) {
        return;
    }

    // Sort by confidence (assuming higher confidence is better)
    std::sort(detections.begin(), detections.end(),
        [](const FaceDetectionResult& a, const FaceDetectionResult& b) {
            return a.confidence > b.confidence;
        });

    std::vector<bool> keep(detections.size(), true);

    for (size_t i = 0; i < detections.size(); ++i) {
        if (!keep[i]) continue;

        for (size_t j = i + 1; j < detections.size(); ++j) {
            if (!keep[j]) continue;

            double iou = calculateIoU(detections[i].bbox, detections[j].bbox);
            if (iou > iou_threshold) {
                keep[j] = false; // Remove the one with lower confidence
            }
        }
    }

    // Remove filtered detections
    auto new_end = std::remove_if(detections.begin(), detections.end(),
        [&keep, &detections](const FaceDetectionResult& detection) {
            size_t index = &detection - &detections[0];
            return !keep[index];
        });

    detections.erase(new_end, detections.end());
}

} // namespace FaceDetectionUtils

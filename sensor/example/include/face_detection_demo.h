/*
 * Face Detection Demo Header
 * 
 * This header defines the main application class for face detection demo
 * using OpenCV and system camera.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#ifndef FACE_DETECTION_DEMO_H
#define FACE_DETECTION_DEMO_H

#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

// Forward declarations
class CameraCapture;
class FaceDetector;
class PerformanceMonitor;
class ConfigManager;

// Configuration structure
struct FaceDetectionConfig {
    // Camera settings
    int camera_id = 0;
    std::string device_path = "/dev/video0";  // Linux specific
    int width = 640;
    int height = 480;
    int fps = 30;
    
    // Detection settings
    double scale_factor = 1.1;
    int min_neighbors = 3;
    int min_size = 30;
    int max_size = 300;
    
    // Display settings
    bool show_fps = true;
    bool show_detection_info = true;
    bool show_confidence = false;
    std::string window_title = "Face Detection Demo";
    
    // Performance settings
    bool enable_multithreading = true;
    int max_queue_size = 5;
    bool enable_performance_monitor = true;
    
    // Output settings
    bool save_video = false;
    std::string output_filename = "output.avi";
    int output_fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
    
    // Debug settings
    bool verbose = false;
    bool enable_debug_display = false;
};

// Face detection result
struct FaceDetectionResult {
    cv::Rect bbox;              // Bounding box
    double confidence = 1.0;    // Detection confidence
    std::string label;          // Optional label
    cv::Point2f center;         // Face center
    
    FaceDetectionResult() = default;
    FaceDetectionResult(const cv::Rect& rect) 
        : bbox(rect), center(rect.x + rect.width/2.0f, rect.y + rect.height/2.0f) {}
};

// Main application class
class FaceDetectionDemo {
public:
    FaceDetectionDemo();
    explicit FaceDetectionDemo(const FaceDetectionConfig& config);
    ~FaceDetectionDemo();
    
    // Initialization and cleanup
    bool initialize();
    bool initialize(int camera_id);
    bool initialize(const std::string& device_path);
    void cleanup();
    
    // Main execution
    int run();
    void stop();
    
    // Configuration
    void setConfig(const FaceDetectionConfig& config);
    const FaceDetectionConfig& getConfig() const;
    bool loadConfigFromFile(const std::string& filename);
    bool saveConfigToFile(const std::string& filename) const;
    
    // Statistics
    struct Statistics {
        std::atomic<int> frames_processed{0};
        std::atomic<int> faces_detected{0};
        std::atomic<int> frames_dropped{0};
        std::atomic<double> average_fps{0.0};
        std::atomic<double> average_detection_time{0.0};
    };
    
    const Statistics& getStatistics() const { return stats_; }
    void resetStatistics();
    void printStatistics() const;
    
    // Event callbacks
    using FaceDetectionCallback = std::function<void(const std::vector<FaceDetectionResult>&)>;
    void setFaceDetectionCallback(FaceDetectionCallback callback);
    
private:
    // Core components
    std::unique_ptr<CameraCapture> camera_;
    std::unique_ptr<FaceDetector> detector_;
    std::unique_ptr<PerformanceMonitor> performance_monitor_;
    std::unique_ptr<ConfigManager> config_manager_;
    
    // Configuration
    FaceDetectionConfig config_;
    
    // Threading
    std::thread capture_thread_;
    std::thread process_thread_;
    
    std::atomic<bool> running_{false};
    std::atomic<bool> initialized_{false};
    
    // Frame processing queue
    std::queue<cv::Mat> frame_queue_;
    std::mutex frame_mutex_;
    std::condition_variable frame_cv_;
    
    // Result queue
    std::queue<std::pair<cv::Mat, std::vector<FaceDetectionResult>>> result_queue_;
    std::mutex result_mutex_;
    std::condition_variable result_cv_;
    
    // Statistics
    mutable Statistics stats_;
    
    // Video writer
    std::unique_ptr<cv::VideoWriter> video_writer_;
    
    // Callback
    FaceDetectionCallback face_callback_;
    
    // Private methods
    void captureLoop();
    void processLoop();
    
    bool initializeCamera();
    bool initializeFaceDetector();
    bool initializeVideoWriter();
    
    void processFrame(const cv::Mat& frame);
    void drawDetectionResults(cv::Mat& frame, const std::vector<FaceDetectionResult>& results);
    void updateStatistics(int face_count, double detection_time);
    
    // Utility methods
    std::string formatFPS(double fps) const;
    std::string formatTime(double time_ms) const;
    cv::Scalar getDetectionColor(size_t index) const;
    
    // Error handling
    void handleError(const std::string& error_msg);
    bool checkSystemResources();
    
    // Non-copyable
    FaceDetectionDemo(const FaceDetectionDemo&) = delete;
    FaceDetectionDemo& operator=(const FaceDetectionDemo&) = delete;
};

// Utility functions
namespace FaceDetectionUtils {
    // Command line argument parsing
    FaceDetectionConfig parseCommandLineArgs(int argc, char* argv[]);
    void printUsage(const std::string& program_name);
    
    // System information
    std::vector<int> getAvailableCameras();
    bool isCameraAvailable(int camera_id);
    std::string getSystemInfo();
    
    // Image utilities
    cv::Mat resizeKeepAspectRatio(const cv::Mat& image, const cv::Size& target_size);
    cv::Rect expandRect(const cv::Rect& rect, double factor, const cv::Size& image_size);
    
    // Performance utilities
    double calculateIoU(const cv::Rect& rect1, const cv::Rect& rect2);
    void filterOverlappingDetections(std::vector<FaceDetectionResult>& detections, double iou_threshold = 0.3);
}

// Global constants
namespace FaceDetectionConstants {
    constexpr int DEFAULT_CAMERA_ID = 0;
    constexpr int DEFAULT_WIDTH = 640;
    constexpr int DEFAULT_HEIGHT = 480;
    constexpr int DEFAULT_FPS = 30;
    constexpr double DEFAULT_SCALE_FACTOR = 1.1;
    constexpr int DEFAULT_MIN_NEIGHBORS = 3;
    constexpr int DEFAULT_MIN_SIZE = 30;
    constexpr int MAX_QUEUE_SIZE = 10;
    constexpr double FPS_UPDATE_INTERVAL = 1.0;  // seconds
}

#endif // FACE_DETECTION_DEMO_H

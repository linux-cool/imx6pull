/*
 * Face Detector Header
 * 
 * This header defines the face detection interface using OpenCV's
 * Haar cascade classifiers and DNN-based face detection.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#ifndef FACE_DETECTOR_H
#define FACE_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

// Face detection result
struct FaceDetection {
    cv::Rect bbox;              // Bounding box
    float confidence = 1.0f;    // Detection confidence [0.0, 1.0]
    cv::Point2f center;         // Face center point
    std::string method;         // Detection method used
    
    FaceDetection() = default;
    FaceDetection(const cv::Rect& rect, float conf = 1.0f) 
        : bbox(rect), confidence(conf), center(rect.x + rect.width/2.0f, rect.y + rect.height/2.0f) {}
};

// Face detector configuration
struct FaceDetectorConfig {
    // Detection method
    enum Method {
        HAAR_CASCADE,
        DNN_CAFFE,
        DNN_TENSORFLOW,
        DNN_ONNX
    } method = HAAR_CASCADE;
    
    // Haar cascade parameters
    double scale_factor = 1.1;
    int min_neighbors = 3;
    int min_size = 30;
    int max_size = 300;
    
    // DNN parameters
    float confidence_threshold = 0.7f;
    float nms_threshold = 0.4f;
    cv::Size input_size = cv::Size(300, 300);
    cv::Scalar mean = cv::Scalar(104.0, 177.0, 123.0);
    double scale = 1.0;
    bool swap_rb = false;
    
    // Model paths
    std::string haar_cascade_path = "haarcascade_frontalface_alt.xml";
    std::string dnn_model_path = "opencv_face_detector_uint8.pb";
    std::string dnn_config_path = "opencv_face_detector.pbtxt";
    
    // Performance settings
    bool enable_gpu = false;
    int num_threads = 1;
    bool enable_optimization = true;
    
    // Post-processing
    bool enable_nms = true;
    bool enable_tracking = false;
    int max_faces = 10;
    
    FaceDetectorConfig() = default;
};

// Face detector statistics
struct FaceDetectorStats {
    std::atomic<int> frames_processed{0};
    std::atomic<int> faces_detected{0};
    std::atomic<double> average_detection_time{0.0};
    std::atomic<double> average_faces_per_frame{0.0};
    std::atomic<int> total_detections{0};
    
    void reset() {
        frames_processed = 0;
        faces_detected = 0;
        average_detection_time = 0.0;
        average_faces_per_frame = 0.0;
        total_detections = 0;
    }
};

// Main face detector class
class FaceDetector {
public:
    FaceDetector();
    explicit FaceDetector(const FaceDetectorConfig& config);
    ~FaceDetector();
    
    // Initialization
    bool initialize();
    bool initialize(const FaceDetectorConfig& config);
    bool isInitialized() const { return initialized_; }
    
    // Configuration
    void setConfig(const FaceDetectorConfig& config);
    const FaceDetectorConfig& getConfig() const;
    
    // Detection methods
    std::vector<FaceDetection> detectFaces(const cv::Mat& image);
    bool detectFaces(const cv::Mat& image, std::vector<FaceDetection>& faces);
    
    // Batch processing
    std::vector<std::vector<FaceDetection>> detectFacesBatch(const std::vector<cv::Mat>& images);
    
    // Statistics
    const FaceDetectorStats& getStatistics() const { return stats_; }
    void resetStatistics() { stats_.reset(); }
    
    // Model management
    bool loadHaarCascade(const std::string& cascade_path);
    bool loadDNNModel(const std::string& model_path, const std::string& config_path = "");
    
    // Utility methods
    cv::Mat preprocessImage(const cv::Mat& image) const;
    void drawDetections(cv::Mat& image, const std::vector<FaceDetection>& faces) const;
    
    // Performance optimization
    void enableGPU(bool enable = true);
    void setNumThreads(int num_threads);
    
    // Error handling
    std::string getLastError() const { return last_error_; }
    
    // Static utility methods
    static std::vector<std::string> getAvailableHaarCascades();
    static bool isModelFileValid(const std::string& model_path);
    static FaceDetectorConfig getDefaultConfig(FaceDetectorConfig::Method method);
    
private:
    // Configuration
    FaceDetectorConfig config_;
    
    // Detection engines
    std::unique_ptr<cv::CascadeClassifier> haar_cascade_;
    std::unique_ptr<cv::dnn::Net> dnn_net_;
    
    // State
    std::atomic<bool> initialized_{false};
    
    // Statistics
    mutable FaceDetectorStats stats_;
    
    // Performance tracking
    mutable std::chrono::steady_clock::time_point last_detection_time_;
    mutable double total_detection_time_ = 0.0;
    
    // Thread safety
    mutable std::mutex detection_mutex_;
    
    // Error handling
    mutable std::string last_error_;
    
    // Private detection methods
    std::vector<FaceDetection> detectWithHaarCascade(const cv::Mat& image);
    std::vector<FaceDetection> detectWithDNN(const cv::Mat& image);
    
    // Post-processing
    void applyNonMaximumSuppression(std::vector<FaceDetection>& faces) const;
    void filterDetectionsBySize(std::vector<FaceDetection>& faces) const;
    void limitMaxDetections(std::vector<FaceDetection>& faces) const;
    
    // Utility methods
    void updateStatistics(int face_count, double detection_time) const;
    void setError(const std::string& error) const;
    
    // Model loading helpers
    bool loadHaarCascadeInternal(const std::string& cascade_path);
    bool loadDNNModelInternal(const std::string& model_path, const std::string& config_path);
    
    // Validation
    bool validateConfig(const FaceDetectorConfig& config) const;
    bool validateImage(const cv::Mat& image) const;
    
    // Non-copyable
    FaceDetector(const FaceDetector&) = delete;
    FaceDetector& operator=(const FaceDetector&) = delete;
};

// Utility functions
namespace FaceDetectorUtils {
    // Model file utilities
    std::string findHaarCascadeFile(const std::string& cascade_name);
    std::vector<std::string> findAvailableModels(const std::string& models_dir);
    
    // Detection utilities
    double calculateIoU(const cv::Rect& rect1, const cv::Rect& rect2);
    std::vector<FaceDetection> mergeDetections(const std::vector<FaceDetection>& detections1,
                                              const std::vector<FaceDetection>& detections2,
                                              double iou_threshold = 0.3);
    
    // Visualization utilities
    cv::Scalar getDetectionColor(size_t index);
    void drawBoundingBox(cv::Mat& image, const cv::Rect& bbox, 
                        const cv::Scalar& color = cv::Scalar(0, 255, 0),
                        int thickness = 2);
    void drawConfidence(cv::Mat& image, const cv::Rect& bbox, float confidence,
                       const cv::Scalar& color = cv::Scalar(0, 255, 0));
    
    // Performance utilities
    std::string formatDetectionTime(double time_ms);
    std::string formatDetectionStats(const FaceDetectorStats& stats);
    
    // Validation utilities
    bool isValidBoundingBox(const cv::Rect& bbox, const cv::Size& image_size);
    bool isValidConfidence(float confidence);
    
    // Configuration utilities
    FaceDetectorConfig loadConfigFromFile(const std::string& config_file);
    bool saveConfigToFile(const FaceDetectorConfig& config, const std::string& config_file);
}

// Constants
namespace FaceDetectorConstants {
    // Default model files
    const std::string DEFAULT_HAAR_CASCADE = "haarcascade_frontalface_alt.xml";
    const std::string DEFAULT_DNN_MODEL = "opencv_face_detector_uint8.pb";
    const std::string DEFAULT_DNN_CONFIG = "opencv_face_detector.pbtxt";
    
    // Default parameters
    constexpr double DEFAULT_SCALE_FACTOR = 1.1;
    constexpr int DEFAULT_MIN_NEIGHBORS = 3;
    constexpr int DEFAULT_MIN_SIZE = 30;
    constexpr int DEFAULT_MAX_SIZE = 300;
    constexpr float DEFAULT_CONFIDENCE_THRESHOLD = 0.7f;
    constexpr float DEFAULT_NMS_THRESHOLD = 0.4f;
    
    // Performance limits
    constexpr int MAX_FACES_PER_FRAME = 50;
    constexpr double MAX_DETECTION_TIME_MS = 1000.0;
    
    // Colors for visualization
    const cv::Scalar COLOR_GREEN = cv::Scalar(0, 255, 0);
    const cv::Scalar COLOR_RED = cv::Scalar(0, 0, 255);
    const cv::Scalar COLOR_BLUE = cv::Scalar(255, 0, 0);
    const cv::Scalar COLOR_YELLOW = cv::Scalar(0, 255, 255);
    const cv::Scalar COLOR_CYAN = cv::Scalar(255, 255, 0);
    const cv::Scalar COLOR_MAGENTA = cv::Scalar(255, 0, 255);
}

#endif // FACE_DETECTOR_H

/*
 * Advanced Face Detector Header
 * 
 * This header defines advanced face detection algorithms including YOLO, SSD,
 * RetinaNet, MTCNN, and LFFD for various performance and accuracy requirements.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#ifndef ADVANCED_FACE_DETECTOR_H
#define ADVANCED_FACE_DETECTOR_H

#include "face_detector.h"
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <memory>
#include <vector>
#include <string>
#include <map>

// Detection algorithm types
enum class DetectionAlgorithm {
    HAAR_CASCADE,       // Traditional Haar cascade
    DNN_CAFFE,         // DNN with Caffe model
    DNN_TENSORFLOW,    // DNN with TensorFlow model
    DNN_ONNX,          // DNN with ONNX model
    YOLO_V3,           // YOLO v3 for face detection
    YOLO_V4,           // YOLO v4 for face detection
    YOLO_V5,           // YOLO v5 for face detection
    SSD_MOBILENET,     // SSD with MobileNet backbone
    SSD_RESNET,        // SSD with ResNet backbone
    RETINANET,         // RetinaNet for small face detection
    MTCNN,             // Multi-task CNN for face detection
    LFFD,              // Light and Fast Face Detector
    SCRFD,             // Sample and Computation Redistributed Face Detection
    YOLO_FACE          // Specialized YOLO for faces
};

// Algorithm performance characteristics
struct AlgorithmProfile {
    DetectionAlgorithm algorithm;
    std::string name;
    std::string description;
    
    // Performance metrics (1-5 scale, 5 being best)
    int speed_rating;
    int accuracy_rating;
    int memory_efficiency;
    
    // Resource requirements
    size_t min_memory_mb;
    bool requires_gpu;
    bool supports_batch;
    
    // Optimal use cases
    std::vector<std::string> use_cases;
    
    // Model file requirements
    std::string model_file;
    std::string config_file;
    std::string weights_file;
    
    AlgorithmProfile() = default;
    AlgorithmProfile(DetectionAlgorithm algo, const std::string& n, const std::string& desc,
                    int speed, int accuracy, int memory, size_t min_mem, bool gpu, bool batch)
        : algorithm(algo), name(n), description(desc), speed_rating(speed), 
          accuracy_rating(accuracy), memory_efficiency(memory), min_memory_mb(min_mem),
          requires_gpu(gpu), supports_batch(batch) {}
};

// Advanced detector configuration
struct AdvancedDetectorConfig {
    DetectionAlgorithm algorithm = DetectionAlgorithm::HAAR_CASCADE;
    
    // Common parameters
    float confidence_threshold = 0.7f;
    float nms_threshold = 0.4f;
    cv::Size input_size = cv::Size(416, 416);
    cv::Scalar mean = cv::Scalar(104, 177, 123);
    double scale = 1.0;
    bool swap_rb = false;
    
    // YOLO specific
    float yolo_confidence = 0.5f;
    float yolo_nms = 0.4f;
    std::vector<std::string> yolo_classes;
    
    // SSD specific
    float ssd_confidence = 0.7f;
    cv::Size ssd_input_size = cv::Size(300, 300);
    
    // RetinaNet specific
    float retinanet_confidence = 0.7f;
    cv::Size retinanet_input_size = cv::Size(640, 640);
    
    // MTCNN specific
    float mtcnn_min_face_size = 20.0f;
    std::vector<float> mtcnn_thresholds = {0.6f, 0.7f, 0.7f};
    std::vector<float> mtcnn_scale_factors = {0.709f, 0.709f, 0.709f};
    
    // LFFD specific
    float lffd_confidence = 0.7f;
    cv::Size lffd_input_size = cv::Size(480, 640);
    
    // Performance settings
    bool enable_gpu = false;
    int num_threads = 1;
    bool enable_optimization = true;
    bool enable_fp16 = false;
    
    // Model paths
    std::string model_dir = "models/";
    std::map<DetectionAlgorithm, std::string> model_paths;
    
    AdvancedDetectorConfig() {
        setupDefaultModelPaths();
    }
    
private:
    void setupDefaultModelPaths();
};

// Advanced face detection result with additional information
struct AdvancedFaceDetection : public FaceDetection {
    DetectionAlgorithm algorithm_used;
    float detection_time_ms;
    
    // Additional face attributes (if supported by algorithm)
    std::vector<cv::Point2f> landmarks;  // Facial landmarks
    float pose_yaw = 0.0f;               // Head pose angles
    float pose_pitch = 0.0f;
    float pose_roll = 0.0f;
    
    // Face quality metrics
    float blur_score = 0.0f;
    float brightness_score = 0.0f;
    float face_quality = 0.0f;
    
    // Age and gender (if supported)
    int estimated_age = -1;
    std::string estimated_gender = "unknown";
    float gender_confidence = 0.0f;
    
    AdvancedFaceDetection() = default;
    AdvancedFaceDetection(const FaceDetection& base) : FaceDetection(base) {}
};

// Main advanced face detector class
class AdvancedFaceDetector {
public:
    AdvancedFaceDetector();
    explicit AdvancedFaceDetector(const AdvancedDetectorConfig& config);
    ~AdvancedFaceDetector();
    
    // Initialization
    bool initialize();
    bool initialize(DetectionAlgorithm algorithm);
    bool initialize(const AdvancedDetectorConfig& config);
    
    // Configuration
    void setConfig(const AdvancedDetectorConfig& config);
    const AdvancedDetectorConfig& getConfig() const;
    void setAlgorithm(DetectionAlgorithm algorithm);
    DetectionAlgorithm getCurrentAlgorithm() const;
    
    // Detection methods
    std::vector<AdvancedFaceDetection> detectFaces(const cv::Mat& image);
    bool detectFaces(const cv::Mat& image, std::vector<AdvancedFaceDetection>& faces);
    
    // Batch processing
    std::vector<std::vector<AdvancedFaceDetection>> detectFacesBatch(
        const std::vector<cv::Mat>& images);
    
    // Algorithm management
    std::vector<DetectionAlgorithm> getAvailableAlgorithms() const;
    AlgorithmProfile getAlgorithmProfile(DetectionAlgorithm algorithm) const;
    std::vector<AlgorithmProfile> getAllProfiles() const;
    
    // Model management
    bool loadModel(DetectionAlgorithm algorithm, const std::string& model_path,
                   const std::string& config_path = "", const std::string& weights_path = "");
    bool isModelLoaded(DetectionAlgorithm algorithm) const;
    void unloadModel(DetectionAlgorithm algorithm);
    void unloadAllModels();
    
    // Performance analysis
    void enableProfiling(bool enable);
    std::map<std::string, double> getProfilingResults() const;
    void resetProfilingResults();
    
    // Utility methods
    cv::Mat preprocessImage(const cv::Mat& image, DetectionAlgorithm algorithm) const;
    void drawAdvancedDetections(cv::Mat& image, 
                               const std::vector<AdvancedFaceDetection>& faces) const;
    
    // Algorithm recommendation
    DetectionAlgorithm recommendAlgorithm(const cv::Size& image_size, 
                                         bool real_time_required = true,
                                         bool high_accuracy_required = false) const;
    
    // Error handling
    std::string getLastError() const;
    bool hasError() const;
    
    // Static utility methods
    static std::vector<AlgorithmProfile> getBuiltinProfiles();
    static bool isAlgorithmSupported(DetectionAlgorithm algorithm);
    static std::string algorithmToString(DetectionAlgorithm algorithm);
    static DetectionAlgorithm stringToAlgorithm(const std::string& name);
    
private:
    // Configuration
    AdvancedDetectorConfig config_;
    DetectionAlgorithm current_algorithm_;
    
    // Model storage
    std::map<DetectionAlgorithm, cv::dnn::Net> loaded_models_;
    std::map<DetectionAlgorithm, bool> model_status_;
    
    // Performance monitoring
    bool profiling_enabled_;
    std::map<std::string, double> profiling_results_;
    
    // State
    bool initialized_;
    mutable std::string last_error_;
    
    // Private methods
    bool initializeAlgorithm(DetectionAlgorithm algorithm);
    std::vector<AdvancedFaceDetection> detectWithYOLO(const cv::Mat& image);
    std::vector<AdvancedFaceDetection> detectWithSSD(const cv::Mat& image);
    std::vector<AdvancedFaceDetection> detectWithRetinaNet(const cv::Mat& image);
    std::vector<AdvancedFaceDetection> detectWithMTCNN(const cv::Mat& image);
    std::vector<AdvancedFaceDetection> detectWithLFFD(const cv::Mat& image);
    
    void setError(const std::string& error) const;
    void updateProfilingResults(const std::string& operation, double time_ms);
    
    // Non-copyable
    AdvancedFaceDetector(const AdvancedFaceDetector&) = delete;
    AdvancedFaceDetector& operator=(const AdvancedFaceDetector&) = delete;
};

// Utility functions
namespace AdvancedDetectorUtils {
    // Model downloading and management
    bool downloadModel(DetectionAlgorithm algorithm, const std::string& destination_dir);
    std::vector<std::string> getRequiredFiles(DetectionAlgorithm algorithm);
    bool verifyModelFiles(DetectionAlgorithm algorithm, const std::string& model_dir);
    
    // Performance benchmarking
    struct BenchmarkResult {
        DetectionAlgorithm algorithm;
        double avg_inference_time_ms;
        double avg_fps;
        double memory_usage_mb;
        int total_detections;
        double accuracy_score;
    };
    
    std::vector<BenchmarkResult> benchmarkAlgorithms(
        const std::vector<cv::Mat>& test_images,
        const std::vector<DetectionAlgorithm>& algorithms);
    
    // Algorithm comparison
    void printAlgorithmComparison(const std::vector<AlgorithmProfile>& profiles);
    AlgorithmProfile findBestAlgorithm(const std::vector<AlgorithmProfile>& profiles,
                                      bool prioritize_speed = true);
    
    // Model format conversion
    bool convertModel(const std::string& source_path, const std::string& target_path,
                     const std::string& source_format, const std::string& target_format);
}

// Constants
namespace AdvancedDetectorConstants {
    // Default model URLs
    extern const std::map<DetectionAlgorithm, std::string> MODEL_URLS;
    
    // Performance thresholds
    constexpr double REAL_TIME_FPS_THRESHOLD = 25.0;
    constexpr double HIGH_ACCURACY_THRESHOLD = 0.9;
    constexpr size_t MOBILE_MEMORY_LIMIT_MB = 100;
    
    // Input size recommendations
    extern const std::map<DetectionAlgorithm, cv::Size> RECOMMENDED_INPUT_SIZES;
}

#endif // ADVANCED_FACE_DETECTOR_H

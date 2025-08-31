/*
 * Face Recognition Engine Header for IMX6ULL Pro
 * 
 * Provides face detection and recognition capabilities
 * Optimized for ARM Cortex-A7 with NCNN inference framework
 * 
 * Author: Face Recognition Team
 * License: MIT
 */

#ifndef _FACE_ENGINE_H_
#define _FACE_ENGINE_H_

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>

// Face detection result
struct FaceDetection {
    cv::Rect bbox;              // Bounding box
    float confidence;           // Detection confidence [0.0, 1.0]
    cv::Point2f landmarks[5];   // Facial landmarks (eyes, nose, mouth corners)
    
    FaceDetection() : confidence(0.0f) {
        for (int i = 0; i < 5; i++) {
            landmarks[i] = cv::Point2f(0, 0);
        }
    }
};

// Face recognition result
struct FaceResult {
    std::string person_id;      // Person identifier
    float confidence;           // Recognition confidence [0.0, 1.0]
    FaceDetection detection;    // Detection information
    std::vector<float> feature; // Face feature vector (optional)
    
    FaceResult() : confidence(0.0f) {}
};

// Face database entry
struct FaceDatabaseEntry {
    std::string person_id;
    std::string name;
    std::vector<float> feature;
    std::string image_path;
    uint64_t timestamp;
    
    FaceDatabaseEntry() : timestamp(0) {}
};

// Face engine configuration
struct FaceEngineConfig {
    std::string model_path;         // Path to model files
    float detection_threshold;      // Detection confidence threshold
    float recognition_threshold;    // Recognition confidence threshold
    int max_faces;                  // Maximum faces to detect per frame
    int input_width;                // Input image width for processing
    int input_height;               // Input image height for processing
    bool use_landmarks;             // Enable landmark detection
    bool enable_tracking;           // Enable face tracking
    int num_threads;                // Number of inference threads
    
    FaceEngineConfig() 
        : detection_threshold(0.7f), recognition_threshold(0.8f)
        , max_faces(5), input_width(320), input_height(240)
        , use_landmarks(true), enable_tracking(false), num_threads(1) {}
};

// Face engine statistics
struct FaceEngineStats {
    uint64_t frames_processed;
    uint64_t faces_detected;
    uint64_t faces_recognized;
    double avg_detection_time_ms;
    double avg_recognition_time_ms;
    uint64_t total_errors;
    
    FaceEngineStats() 
        : frames_processed(0), faces_detected(0), faces_recognized(0)
        , avg_detection_time_ms(0.0), avg_recognition_time_ms(0.0)
        , total_errors(0) {}
};

// Error codes
enum FaceEngineError {
    FACE_ENGINE_SUCCESS = 0,
    FACE_ENGINE_ERROR_INVALID_PARAM = -1,
    FACE_ENGINE_ERROR_MODEL_NOT_FOUND = -2,
    FACE_ENGINE_ERROR_MODEL_LOAD_FAILED = -3,
    FACE_ENGINE_ERROR_INFERENCE_FAILED = -4,
    FACE_ENGINE_ERROR_NO_MEMORY = -5,
    FACE_ENGINE_ERROR_DATABASE_ERROR = -6,
    FACE_ENGINE_ERROR_FEATURE_EXTRACTION_FAILED = -7,
    FACE_ENGINE_ERROR_SYSTEM_ERROR = -8
};

// Face tracking information
struct FaceTrack {
    int track_id;
    FaceDetection detection;
    std::vector<cv::Point2f> trajectory;
    int age;                    // Number of frames tracked
    int lost_count;             // Number of consecutive frames lost
    
    FaceTrack() : track_id(-1), age(0), lost_count(0) {}
};

// Face engine main class
class FaceEngine {
public:
    FaceEngine();
    virtual ~FaceEngine();
    
    // Initialization and cleanup
    int Initialize(const FaceEngineConfig& config);
    void Cleanup();
    bool IsInitialized() const;
    
    // Face detection
    int DetectFaces(const cv::Mat& image, std::vector<FaceDetection>& detections);
    int DetectFacesWithLandmarks(const cv::Mat& image, std::vector<FaceDetection>& detections);
    
    // Face recognition
    int ExtractFeature(const cv::Mat& image, const FaceDetection& detection, 
                      std::vector<float>& feature);
    int RecognizeFaces(const cv::Mat& image, const std::vector<FaceDetection>& detections,
                      std::vector<FaceResult>& results);
    int RecognizeFace(const cv::Mat& image, const FaceDetection& detection,
                     FaceResult& result);
    
    // Face tracking
    int TrackFaces(const cv::Mat& image, std::vector<FaceTrack>& tracks);
    int UpdateTracks(const std::vector<FaceDetection>& detections,
                    std::vector<FaceTrack>& tracks);
    
    // Face database management
    int LoadDatabase(const std::string& database_path);
    int SaveDatabase(const std::string& database_path);
    int AddPerson(const std::string& person_id, const std::string& name,
                 const cv::Mat& face_image);
    int RemovePerson(const std::string& person_id);
    int UpdatePerson(const std::string& person_id, const cv::Mat& face_image);
    int GetPersonList(std::vector<std::string>& person_ids);
    int GetPersonInfo(const std::string& person_id, FaceDatabaseEntry& entry);
    
    // Configuration
    int SetConfig(const FaceEngineConfig& config);
    int GetConfig(FaceEngineConfig& config) const;
    
    // Statistics and monitoring
    int GetStatistics(FaceEngineStats& stats) const;
    void ResetStatistics();
    
    // Utility functions
    cv::Mat AlignFace(const cv::Mat& image, const FaceDetection& detection);
    cv::Mat CropFace(const cv::Mat& image, const FaceDetection& detection, 
                    int target_size = 112);
    float ComputeSimilarity(const std::vector<float>& feature1,
                           const std::vector<float>& feature2);
    
    // Model information
    std::string GetModelInfo() const;
    std::vector<std::string> GetSupportedFormats() const;
    
    // Error handling
    std::string GetLastError() const;
    static std::string ErrorToString(FaceEngineError error);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Non-copyable
    FaceEngine(const FaceEngine&) = delete;
    FaceEngine& operator=(const FaceEngine&) = delete;
};

// Utility functions
namespace FaceUtils {
    // Image preprocessing
    cv::Mat PreprocessImage(const cv::Mat& image, int target_width, int target_height);
    cv::Mat NormalizeImage(const cv::Mat& image);
    
    // Geometric operations
    cv::Mat GetAffineTransform(const std::vector<cv::Point2f>& src_points,
                              const std::vector<cv::Point2f>& dst_points);
    cv::Rect ExpandBBox(const cv::Rect& bbox, float expand_ratio, 
                       const cv::Size& image_size);
    
    // Feature operations
    void NormalizeFeature(std::vector<float>& feature);
    float CosineSimilarity(const std::vector<float>& feature1,
                          const std::vector<float>& feature2);
    float EuclideanDistance(const std::vector<float>& feature1,
                           const std::vector<float>& feature2);
    
    // Validation
    bool IsValidBBox(const cv::Rect& bbox, const cv::Size& image_size);
    bool IsValidLandmarks(const cv::Point2f landmarks[5], const cv::Size& image_size);
    
    // Performance helpers
    double GetCurrentTimeMs();
    void PrintBenchmark(const std::string& operation, double time_ms);
}

// Face quality assessment
namespace FaceQuality {
    // Quality metrics
    struct QualityMetrics {
        float blur_score;       // Blur assessment [0.0, 1.0]
        float brightness_score; // Brightness assessment [0.0, 1.0]
        float pose_score;       // Pose assessment [0.0, 1.0]
        float overall_score;    // Overall quality [0.0, 1.0]
    };
    
    // Quality assessment functions
    int AssessQuality(const cv::Mat& face_image, QualityMetrics& metrics);
    float AssessBlur(const cv::Mat& face_image);
    float AssessBrightness(const cv::Mat& face_image);
    float AssessPose(const cv::Point2f landmarks[5]);
    bool IsGoodQuality(const QualityMetrics& metrics, float threshold = 0.6f);
}

// Constants
namespace FaceConstants {
    // Model file names
    constexpr const char* DETECTION_MODEL = "face_detection.ncnn.param";
    constexpr const char* DETECTION_WEIGHTS = "face_detection.ncnn.bin";
    constexpr const char* RECOGNITION_MODEL = "face_recognition.ncnn.param";
    constexpr const char* RECOGNITION_WEIGHTS = "face_recognition.ncnn.bin";
    
    // Default parameters
    constexpr int DEFAULT_FACE_SIZE = 112;
    constexpr int DEFAULT_FEATURE_SIZE = 512;
    constexpr float DEFAULT_NMS_THRESHOLD = 0.4f;
    constexpr int MAX_DETECTION_COUNT = 100;
    
    // Landmark indices
    constexpr int LEFT_EYE = 0;
    constexpr int RIGHT_EYE = 1;
    constexpr int NOSE = 2;
    constexpr int LEFT_MOUTH = 3;
    constexpr int RIGHT_MOUTH = 4;
}

// Convenience macros
#define FACE_ENGINE_CHECK_ERROR(expr) \
    do { \
        int _ret = (expr); \
        if (_ret != FACE_ENGINE_SUCCESS) { \
            return _ret; \
        } \
    } while(0)

#define FACE_ENGINE_LOG_ERROR(engine, msg) \
    do { \
        std::cerr << "Face Engine Error: " << msg \
                  << " (" << engine.GetLastError() << ")" << std::endl; \
    } while(0)

#endif /* _FACE_ENGINE_H_ */

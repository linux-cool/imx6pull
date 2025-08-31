/*
 * Face Detector Implementation
 * 
 * This file implements face detection using OpenCV's Haar cascade classifiers.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#include "face_detector.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <fstream>

FaceDetector::FaceDetector() {
    haar_cascade_ = std::make_unique<cv::CascadeClassifier>();
}

FaceDetector::FaceDetector(const FaceDetectorConfig& config) : config_(config) {
    haar_cascade_ = std::make_unique<cv::CascadeClassifier>();
}

FaceDetector::~FaceDetector() = default;

bool FaceDetector::initialize() {
    return initialize(config_);
}

bool FaceDetector::initialize(const FaceDetectorConfig& config) {
    config_ = config;
    
    if (!validateConfig(config_)) {
        setError("Invalid configuration");
        return false;
    }
    
    switch (config_.method) {
    case FaceDetectorConfig::HAAR_CASCADE:
        if (!loadHaarCascadeInternal(config_.haar_cascade_path)) {
            return false;
        }
        break;
        
    case FaceDetectorConfig::DNN_CAFFE:
    case FaceDetectorConfig::DNN_TENSORFLOW:
    case FaceDetectorConfig::DNN_ONNX:
        if (!loadDNNModelInternal(config_.dnn_model_path, config_.dnn_config_path)) {
            return false;
        }
        break;
        
    default:
        setError("Unsupported detection method");
        return false;
    }
    
    initialized_ = true;
    return true;
}

void FaceDetector::setConfig(const FaceDetectorConfig& config) {
    config_ = config;
}

const FaceDetectorConfig& FaceDetector::getConfig() const {
    return config_;
}

std::vector<FaceDetection> FaceDetector::detectFaces(const cv::Mat& image) {
    std::vector<FaceDetection> faces;
    detectFaces(image, faces);
    return faces;
}

bool FaceDetector::detectFaces(const cv::Mat& image, std::vector<FaceDetection>& faces) {
    if (!initialized_) {
        setError("Detector not initialized");
        return false;
    }
    
    if (!validateImage(image)) {
        setError("Invalid input image");
        return false;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(detection_mutex_);
    
    switch (config_.method) {
    case FaceDetectorConfig::HAAR_CASCADE:
        faces = detectWithHaarCascade(image);
        break;
        
    case FaceDetectorConfig::DNN_CAFFE:
    case FaceDetectorConfig::DNN_TENSORFLOW:
    case FaceDetectorConfig::DNN_ONNX:
        faces = detectWithDNN(image);
        break;
        
    default:
        setError("Unsupported detection method");
        return false;
    }
    
    // Post-processing
    if (config_.enable_nms) {
        applyNonMaximumSuppression(faces);
    }
    
    filterDetectionsBySize(faces);
    limitMaxDetections(faces);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    updateStatistics(faces.size(), duration.count());
    
    return true;
}

std::vector<std::vector<FaceDetection>> FaceDetector::detectFacesBatch(const std::vector<cv::Mat>& images) {
    std::vector<std::vector<FaceDetection>> results;
    results.reserve(images.size());
    
    for (const auto& image : images) {
        results.push_back(detectFaces(image));
    }
    
    return results;
}

bool FaceDetector::loadHaarCascade(const std::string& cascade_path) {
    return loadHaarCascadeInternal(cascade_path);
}

bool FaceDetector::loadDNNModel(const std::string& model_path, const std::string& config_path) {
    return loadDNNModelInternal(model_path, config_path);
}

cv::Mat FaceDetector::preprocessImage(const cv::Mat& image) const {
    cv::Mat processed;
    
    // Convert to grayscale for Haar cascade
    if (config_.method == FaceDetectorConfig::HAAR_CASCADE) {
        if (image.channels() == 3) {
            cv::cvtColor(image, processed, cv::COLOR_BGR2GRAY);
        } else {
            processed = image.clone();
        }
        
        // Histogram equalization for better detection
        cv::equalizeHist(processed, processed);
    } else {
        // For DNN methods, keep original format
        processed = image.clone();
    }
    
    return processed;
}

void FaceDetector::drawDetections(cv::Mat& image, const std::vector<FaceDetection>& faces) const {
    for (size_t i = 0; i < faces.size(); ++i) {
        const auto& face = faces[i];
        cv::Scalar color = FaceDetectorUtils::getDetectionColor(i);
        
        FaceDetectorUtils::drawBoundingBox(image, face.bbox, color);
        
        if (face.confidence < 1.0f) {
            FaceDetectorUtils::drawConfidence(image, face.bbox, face.confidence, color);
        }
    }
}

void FaceDetector::enableGPU(bool enable) {
    config_.enable_gpu = enable;
    
    if (dnn_net_ && initialized_) {
        if (enable) {
            dnn_net_->setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            dnn_net_->setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        } else {
            dnn_net_->setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            dnn_net_->setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }
    }
}

void FaceDetector::setNumThreads(int num_threads) {
    config_.num_threads = num_threads;
    cv::setNumThreads(num_threads);
}

std::vector<std::string> FaceDetector::getAvailableHaarCascades() {
    // This would typically scan a directory for cascade files
    return {
        "haarcascade_frontalface_alt.xml",
        "haarcascade_frontalface_alt2.xml",
        "haarcascade_frontalface_default.xml",
        "haarcascade_profileface.xml"
    };
}

bool FaceDetector::isModelFileValid(const std::string& model_path) {
    std::ifstream file(model_path);
    return file.good();
}

FaceDetectorConfig FaceDetector::getDefaultConfig(FaceDetectorConfig::Method method) {
    FaceDetectorConfig config;
    config.method = method;
    
    switch (method) {
    case FaceDetectorConfig::HAAR_CASCADE:
        config.scale_factor = 1.1;
        config.min_neighbors = 3;
        config.min_size = 30;
        config.max_size = 300;
        break;
        
    case FaceDetectorConfig::DNN_CAFFE:
    case FaceDetectorConfig::DNN_TENSORFLOW:
    case FaceDetectorConfig::DNN_ONNX:
        config.confidence_threshold = 0.7f;
        config.nms_threshold = 0.4f;
        config.input_size = cv::Size(300, 300);
        break;
    }
    
    return config;
}

std::vector<FaceDetection> FaceDetector::detectWithHaarCascade(const cv::Mat& image) {
    std::vector<FaceDetection> faces;
    
    if (!haar_cascade_ || haar_cascade_->empty()) {
        setError("Haar cascade not loaded");
        return faces;
    }
    
    cv::Mat gray = preprocessImage(image);
    std::vector<cv::Rect> face_rects;
    
    haar_cascade_->detectMultiScale(
        gray,
        face_rects,
        config_.scale_factor,
        config_.min_neighbors,
        0,
        cv::Size(config_.min_size, config_.min_size),
        cv::Size(config_.max_size, config_.max_size)
    );
    
    // Convert to FaceDetection format
    for (const auto& rect : face_rects) {
        FaceDetection detection;
        detection.bbox = rect;
        detection.confidence = 1.0f; // Haar cascade doesn't provide confidence
        detection.center = cv::Point2f(rect.x + rect.width/2.0f, rect.y + rect.height/2.0f);
        detection.method = "Haar Cascade";
        faces.push_back(detection);
    }
    
    return faces;
}

std::vector<FaceDetection> FaceDetector::detectWithDNN(const cv::Mat& image) {
    std::vector<FaceDetection> faces;
    
    if (!dnn_net_) {
        setError("DNN model not loaded");
        return faces;
    }
    
    // Create blob from image
    cv::Mat blob;
    cv::dnn::blobFromImage(image, blob, config_.scale, config_.input_size, 
                          config_.mean, config_.swap_rb, false);
    
    // Set input to the network
    dnn_net_->setInput(blob);
    
    // Run forward pass
    cv::Mat detection = dnn_net_->forward();
    
    // Parse detections
    cv::Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());
    
    for (int i = 0; i < detectionMat.rows; i++) {
        float confidence = detectionMat.at<float>(i, 2);
        
        if (confidence > config_.confidence_threshold) {
            int x1 = static_cast<int>(detectionMat.at<float>(i, 3) * image.cols);
            int y1 = static_cast<int>(detectionMat.at<float>(i, 4) * image.rows);
            int x2 = static_cast<int>(detectionMat.at<float>(i, 5) * image.cols);
            int y2 = static_cast<int>(detectionMat.at<float>(i, 6) * image.rows);
            
            cv::Rect bbox(x1, y1, x2 - x1, y2 - y1);
            
            // Validate bounding box
            if (FaceDetectorUtils::isValidBoundingBox(bbox, image.size())) {
                FaceDetection detection;
                detection.bbox = bbox;
                detection.confidence = confidence;
                detection.center = cv::Point2f(bbox.x + bbox.width/2.0f, bbox.y + bbox.height/2.0f);
                detection.method = "DNN";
                faces.push_back(detection);
            }
        }
    }
    
    return faces;
}

void FaceDetector::applyNonMaximumSuppression(std::vector<FaceDetection>& faces) const {
    if (faces.size() <= 1) {
        return;
    }
    
    // Convert to OpenCV format for NMS
    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    
    for (const auto& face : faces) {
        boxes.push_back(face.bbox);
        scores.push_back(face.confidence);
    }
    
    // Apply NMS
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, scores, config_.confidence_threshold, 
                     config_.nms_threshold, indices);
    
    // Keep only selected detections
    std::vector<FaceDetection> filtered_faces;
    for (int idx : indices) {
        filtered_faces.push_back(faces[idx]);
    }
    
    faces = filtered_faces;
}

void FaceDetector::filterDetectionsBySize(std::vector<FaceDetection>& faces) const {
    faces.erase(std::remove_if(faces.begin(), faces.end(),
        [this](const FaceDetection& face) {
            int size = std::min(face.bbox.width, face.bbox.height);
            return size < config_.min_size || size > config_.max_size;
        }), faces.end());
}

void FaceDetector::limitMaxDetections(std::vector<FaceDetection>& faces) const {
    if (faces.size() > static_cast<size_t>(config_.max_faces)) {
        // Sort by confidence and keep top detections
        std::sort(faces.begin(), faces.end(),
            [](const FaceDetection& a, const FaceDetection& b) {
                return a.confidence > b.confidence;
            });
        
        faces.resize(config_.max_faces);
    }
}

void FaceDetector::updateStatistics(int face_count, double detection_time) const {
    stats_.frames_processed++;
    stats_.total_detections += face_count;
    
    // Update average detection time
    total_detection_time_ += detection_time;
    stats_.average_detection_time = total_detection_time_ / stats_.frames_processed.load();
    
    // Update average faces per frame
    stats_.average_faces_per_frame = static_cast<double>(stats_.total_detections.load()) / 
                                    stats_.frames_processed.load();
}

void FaceDetector::setError(const std::string& error) const {
    last_error_ = error;
}

bool FaceDetector::loadHaarCascadeInternal(const std::string& cascade_path) {
    if (!haar_cascade_->load(cascade_path)) {
        setError("Failed to load Haar cascade: " + cascade_path);
        return false;
    }
    
    if (haar_cascade_->empty()) {
        setError("Loaded Haar cascade is empty");
        return false;
    }
    
    return true;
}

bool FaceDetector::loadDNNModelInternal(const std::string& model_path, const std::string& config_path) {
    try {
        if (config_path.empty()) {
            dnn_net_ = std::make_unique<cv::dnn::Net>(cv::dnn::readNet(model_path));
        } else {
            dnn_net_ = std::make_unique<cv::dnn::Net>(cv::dnn::readNet(model_path, config_path));
        }
        
        if (dnn_net_->empty()) {
            setError("Failed to load DNN model");
            return false;
        }
        
        // Set backend and target
        if (config_.enable_gpu) {
            dnn_net_->setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            dnn_net_->setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        } else {
            dnn_net_->setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            dnn_net_->setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }
        
        return true;
        
    } catch (const cv::Exception& e) {
        setError("DNN model loading failed: " + std::string(e.what()));
        return false;
    }
}

bool FaceDetector::validateConfig(const FaceDetectorConfig& config) const {
    if (config.scale_factor <= 1.0 || config.scale_factor > 2.0) {
        return false;
    }
    
    if (config.min_neighbors < 1 || config.min_neighbors > 10) {
        return false;
    }
    
    if (config.min_size < 10 || config.min_size > 500) {
        return false;
    }
    
    if (config.max_size < config.min_size || config.max_size > 1000) {
        return false;
    }
    
    if (config.confidence_threshold < 0.0f || config.confidence_threshold > 1.0f) {
        return false;
    }
    
    if (config.nms_threshold < 0.0f || config.nms_threshold > 1.0f) {
        return false;
    }
    
    return true;
}

bool FaceDetector::validateImage(const cv::Mat& image) const {
    return !image.empty() && image.cols > 0 && image.rows > 0;
}

// FaceDetectorUtils namespace implementation
namespace FaceDetectorUtils {

std::string findHaarCascadeFile(const std::string& cascade_name) {
    // Common paths where OpenCV cascade files might be located
    std::vector<std::string> search_paths = {
        "/usr/share/opencv4/haarcascades/",
        "/usr/share/opencv/haarcascades/",
        "/usr/local/share/opencv4/haarcascades/",
        "/usr/local/share/opencv/haarcascades/",
        "./data/haarcascades/",
        "./haarcascades/",
        "./"
    };

    for (const auto& path : search_paths) {
        std::string full_path = path + cascade_name;
        std::ifstream file(full_path);
        if (file.good()) {
            return full_path;
        }
    }

    return cascade_name; // Return original name if not found
}

std::vector<std::string> findAvailableModels(const std::string& models_dir) {
    std::vector<std::string> models;

    // This is a simplified implementation
    // In a real application, you would scan the directory
    models.push_back("opencv_face_detector_uint8.pb");
    models.push_back("opencv_face_detector_fp16.pb");

    return models;
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

std::vector<FaceDetection> mergeDetections(const std::vector<FaceDetection>& detections1,
                                          const std::vector<FaceDetection>& detections2,
                                          double iou_threshold) {
    std::vector<FaceDetection> merged = detections1;

    for (const auto& det2 : detections2) {
        bool should_add = true;

        for (const auto& det1 : detections1) {
            if (calculateIoU(det1.bbox, det2.bbox) > iou_threshold) {
                should_add = false;
                break;
            }
        }

        if (should_add) {
            merged.push_back(det2);
        }
    }

    return merged;
}

cv::Scalar getDetectionColor(size_t index) {
    const std::vector<cv::Scalar> colors = {
        FaceDetectorConstants::COLOR_GREEN,
        FaceDetectorConstants::COLOR_RED,
        FaceDetectorConstants::COLOR_BLUE,
        FaceDetectorConstants::COLOR_YELLOW,
        FaceDetectorConstants::COLOR_CYAN,
        FaceDetectorConstants::COLOR_MAGENTA
    };

    return colors[index % colors.size()];
}

void drawBoundingBox(cv::Mat& image, const cv::Rect& bbox,
                    const cv::Scalar& color, int thickness) {
    cv::rectangle(image, bbox, color, thickness);
}

void drawConfidence(cv::Mat& image, const cv::Rect& bbox, float confidence,
                   const cv::Scalar& color) {
    std::string conf_text = cv::format("%.2f", confidence);
    cv::Point text_pos(bbox.x, bbox.y - 5);

    cv::putText(image, conf_text, text_pos, cv::FONT_HERSHEY_SIMPLEX,
               0.5, color, 1);
}

std::string formatDetectionTime(double time_ms) {
    return cv::format("%.2f ms", time_ms);
}

std::string formatDetectionStats(const FaceDetectorStats& stats) {
    std::ostringstream oss;
    oss << "Frames: " << stats.frames_processed.load()
        << ", Faces: " << stats.total_detections.load()
        << ", Avg Time: " << formatDetectionTime(stats.average_detection_time.load())
        << ", Avg Faces/Frame: " << cv::format("%.1f", stats.average_faces_per_frame.load());
    return oss.str();
}

bool isValidBoundingBox(const cv::Rect& bbox, const cv::Size& image_size) {
    return bbox.x >= 0 && bbox.y >= 0 &&
           bbox.x + bbox.width <= image_size.width &&
           bbox.y + bbox.height <= image_size.height &&
           bbox.width > 0 && bbox.height > 0;
}

bool isValidConfidence(float confidence) {
    return confidence >= 0.0f && confidence <= 1.0f;
}

FaceDetectorConfig loadConfigFromFile(const std::string& config_file) {
    // Simplified implementation - would use JSON or INI parser
    return FaceDetectorConfig();
}

bool saveConfigToFile(const FaceDetectorConfig& config, const std::string& config_file) {
    // Simplified implementation - would use JSON or INI writer
    return true;
}

} // namespace FaceDetectorUtils

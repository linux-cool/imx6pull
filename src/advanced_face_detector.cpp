/*
 * Advanced Face Detector Implementation
 * 
 * This file implements advanced face detection algorithms including YOLO, SSD,
 * RetinaNet, MTCNN, and LFFD.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#include "advanced_face_detector.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

// Algorithm profiles initialization
const std::vector<AlgorithmProfile> builtin_profiles = {
    {DetectionAlgorithm::HAAR_CASCADE, "Haar Cascade", "Traditional cascade classifier",
     3, 2, 5, 50, false, false},
    
    {DetectionAlgorithm::YOLO_V3, "YOLO v3", "Fast real-time object detection",
     5, 3, 3, 200, false, true},
    
    {DetectionAlgorithm::YOLO_V4, "YOLO v4", "Improved YOLO with better accuracy",
     4, 4, 3, 250, false, true},
    
    {DetectionAlgorithm::YOLO_V5, "YOLO v5", "Latest YOLO with optimizations",
     5, 4, 4, 180, false, true},
    
    {DetectionAlgorithm::SSD_MOBILENET, "SSD MobileNet", "Balanced speed and accuracy",
     4, 3, 4, 100, false, true},
    
    {DetectionAlgorithm::SSD_RESNET, "SSD ResNet", "Higher accuracy SSD variant",
     3, 4, 2, 300, false, true},
    
    {DetectionAlgorithm::RETINANET, "RetinaNet", "Excellent for small face detection",
     2, 5, 2, 400, true, true},
    
    {DetectionAlgorithm::MTCNN, "MTCNN", "Multi-task CNN specialized for faces",
     3, 5, 4, 150, false, false},
    
    {DetectionAlgorithm::LFFD, "LFFD", "Light and fast face detector for mobile",
     5, 3, 5, 50, false, true},
    
    {DetectionAlgorithm::YOLO_FACE, "YOLO-Face", "YOLO specialized for face detection",
     4, 4, 3, 200, false, true}
};

// AdvancedDetectorConfig implementation
void AdvancedDetectorConfig::setupDefaultModelPaths() {
    model_paths[DetectionAlgorithm::YOLO_V3] = "yolov3-face.weights";
    model_paths[DetectionAlgorithm::YOLO_V4] = "yolov4-face.weights";
    model_paths[DetectionAlgorithm::YOLO_V5] = "yolov5s-face.onnx";
    model_paths[DetectionAlgorithm::SSD_MOBILENET] = "ssd_mobilenet_face.pb";
    model_paths[DetectionAlgorithm::SSD_RESNET] = "ssd_resnet_face.pb";
    model_paths[DetectionAlgorithm::RETINANET] = "retinanet_face.onnx";
    model_paths[DetectionAlgorithm::MTCNN] = "mtcnn_face.onnx";
    model_paths[DetectionAlgorithm::LFFD] = "lffd_face.onnx";
    model_paths[DetectionAlgorithm::YOLO_FACE] = "yolo_face.onnx";
}

// AdvancedFaceDetector implementation
AdvancedFaceDetector::AdvancedFaceDetector() 
    : current_algorithm_(DetectionAlgorithm::HAAR_CASCADE),
      profiling_enabled_(false),
      initialized_(false) {
}

AdvancedFaceDetector::AdvancedFaceDetector(const AdvancedDetectorConfig& config)
    : config_(config),
      current_algorithm_(config.algorithm),
      profiling_enabled_(false),
      initialized_(false) {
}

AdvancedFaceDetector::~AdvancedFaceDetector() {
    unloadAllModels();
}

bool AdvancedFaceDetector::initialize() {
    return initialize(config_.algorithm);
}

bool AdvancedFaceDetector::initialize(DetectionAlgorithm algorithm) {
    current_algorithm_ = algorithm;
    config_.algorithm = algorithm;
    
    if (!initializeAlgorithm(algorithm)) {
        setError("Failed to initialize algorithm: " + algorithmToString(algorithm));
        return false;
    }
    
    initialized_ = true;
    return true;
}

bool AdvancedFaceDetector::initialize(const AdvancedDetectorConfig& config) {
    config_ = config;
    return initialize(config.algorithm);
}

void AdvancedFaceDetector::setConfig(const AdvancedDetectorConfig& config) {
    config_ = config;
    if (config.algorithm != current_algorithm_) {
        initialize(config.algorithm);
    }
}

const AdvancedDetectorConfig& AdvancedFaceDetector::getConfig() const {
    return config_;
}

void AdvancedFaceDetector::setAlgorithm(DetectionAlgorithm algorithm) {
    if (algorithm != current_algorithm_) {
        initialize(algorithm);
    }
}

DetectionAlgorithm AdvancedFaceDetector::getCurrentAlgorithm() const {
    return current_algorithm_;
}

std::vector<AdvancedFaceDetection> AdvancedFaceDetector::detectFaces(const cv::Mat& image) {
    if (!initialized_) {
        setError("Detector not initialized");
        return {};
    }
    
    if (image.empty()) {
        setError("Input image is empty");
        return {};
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<AdvancedFaceDetection> detections;
    
    switch (current_algorithm_) {
    case DetectionAlgorithm::YOLO_V3:
    case DetectionAlgorithm::YOLO_V4:
    case DetectionAlgorithm::YOLO_V5:
    case DetectionAlgorithm::YOLO_FACE:
        detections = detectWithYOLO(image);
        break;
        
    case DetectionAlgorithm::SSD_MOBILENET:
    case DetectionAlgorithm::SSD_RESNET:
        detections = detectWithSSD(image);
        break;
        
    case DetectionAlgorithm::RETINANET:
        detections = detectWithRetinaNet(image);
        break;
        
    case DetectionAlgorithm::MTCNN:
        detections = detectWithMTCNN(image);
        break;
        
    case DetectionAlgorithm::LFFD:
        detections = detectWithLFFD(image);
        break;
        
    default:
        setError("Unsupported algorithm");
        return {};
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Update detection time for all results
    for (auto& detection : detections) {
        detection.algorithm_used = current_algorithm_;
        detection.detection_time_ms = duration.count();
    }
    
    if (profiling_enabled_) {
        updateProfilingResults("detection", duration.count());
    }
    
    return detections;
}

bool AdvancedFaceDetector::detectFaces(const cv::Mat& image, 
                                      std::vector<AdvancedFaceDetection>& faces) {
    faces = detectFaces(image);
    return !faces.empty() || !hasError();
}

std::vector<std::vector<AdvancedFaceDetection>> AdvancedFaceDetector::detectFacesBatch(
    const std::vector<cv::Mat>& images) {
    
    std::vector<std::vector<AdvancedFaceDetection>> results;
    results.reserve(images.size());
    
    for (const auto& image : images) {
        results.push_back(detectFaces(image));
    }
    
    return results;
}

std::vector<DetectionAlgorithm> AdvancedFaceDetector::getAvailableAlgorithms() const {
    std::vector<DetectionAlgorithm> algorithms;
    
    for (const auto& profile : builtin_profiles) {
        if (isAlgorithmSupported(profile.algorithm)) {
            algorithms.push_back(profile.algorithm);
        }
    }
    
    return algorithms;
}

AlgorithmProfile AdvancedFaceDetector::getAlgorithmProfile(DetectionAlgorithm algorithm) const {
    for (const auto& profile : builtin_profiles) {
        if (profile.algorithm == algorithm) {
            return profile;
        }
    }
    
    return AlgorithmProfile(); // Return empty profile if not found
}

std::vector<AlgorithmProfile> AdvancedFaceDetector::getAllProfiles() const {
    return builtin_profiles;
}

bool AdvancedFaceDetector::loadModel(DetectionAlgorithm algorithm, 
                                    const std::string& model_path,
                                    const std::string& config_path,
                                    const std::string& weights_path) {
    try {
        cv::dnn::Net net;
        
        // Load model based on file extension
        std::string ext = model_path.substr(model_path.find_last_of('.'));
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".onnx") {
            net = cv::dnn::readNetFromONNX(model_path);
        } else if (ext == ".pb") {
            if (!config_path.empty()) {
                net = cv::dnn::readNetFromTensorflow(model_path, config_path);
            } else {
                net = cv::dnn::readNetFromTensorflow(model_path);
            }
        } else if (ext == ".weights") {
            if (!config_path.empty()) {
                net = cv::dnn::readNetFromDarknet(config_path, model_path);
            } else {
                setError("Config file required for .weights format");
                return false;
            }
        } else if (ext == ".caffemodel") {
            if (!config_path.empty()) {
                net = cv::dnn::readNetFromCaffe(config_path, model_path);
            } else {
                setError("Config file required for .caffemodel format");
                return false;
            }
        } else {
            setError("Unsupported model format: " + ext);
            return false;
        }
        
        if (net.empty()) {
            setError("Failed to load model: " + model_path);
            return false;
        }
        
        // Set backend and target
        if (config_.enable_gpu) {
            net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        } else {
            net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }
        
        loaded_models_[algorithm] = net;
        model_status_[algorithm] = true;
        
        return true;
        
    } catch (const cv::Exception& e) {
        setError("OpenCV error loading model: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        setError("Error loading model: " + std::string(e.what()));
        return false;
    }
}

bool AdvancedFaceDetector::isModelLoaded(DetectionAlgorithm algorithm) const {
    auto it = model_status_.find(algorithm);
    return it != model_status_.end() && it->second;
}

void AdvancedFaceDetector::unloadModel(DetectionAlgorithm algorithm) {
    loaded_models_.erase(algorithm);
    model_status_[algorithm] = false;
}

void AdvancedFaceDetector::unloadAllModels() {
    loaded_models_.clear();
    model_status_.clear();
}

void AdvancedFaceDetector::enableProfiling(bool enable) {
    profiling_enabled_ = enable;
    if (!enable) {
        profiling_results_.clear();
    }
}

std::map<std::string, double> AdvancedFaceDetector::getProfilingResults() const {
    return profiling_results_;
}

void AdvancedFaceDetector::resetProfilingResults() {
    profiling_results_.clear();
}

cv::Mat AdvancedFaceDetector::preprocessImage(const cv::Mat& image, 
                                             DetectionAlgorithm algorithm) const {
    cv::Mat processed;
    
    switch (algorithm) {
    case DetectionAlgorithm::YOLO_V3:
    case DetectionAlgorithm::YOLO_V4:
    case DetectionAlgorithm::YOLO_V5:
    case DetectionAlgorithm::YOLO_FACE:
        // YOLO preprocessing
        cv::resize(image, processed, config_.input_size);
        processed.convertTo(processed, CV_32F, 1.0/255.0);
        break;
        
    case DetectionAlgorithm::SSD_MOBILENET:
    case DetectionAlgorithm::SSD_RESNET:
        // SSD preprocessing
        cv::resize(image, processed, config_.ssd_input_size);
        break;
        
    case DetectionAlgorithm::RETINANET:
        // RetinaNet preprocessing
        cv::resize(image, processed, config_.retinanet_input_size);
        processed.convertTo(processed, CV_32F);
        break;
        
    case DetectionAlgorithm::MTCNN:
        // MTCNN preprocessing
        processed = image.clone();
        processed.convertTo(processed, CV_32F, 1.0/255.0);
        break;
        
    case DetectionAlgorithm::LFFD:
        // LFFD preprocessing
        cv::resize(image, processed, config_.lffd_input_size);
        processed.convertTo(processed, CV_32F, 1.0/255.0);
        break;
        
    default:
        processed = image.clone();
        break;
    }
    
    return processed;
}

DetectionAlgorithm AdvancedFaceDetector::recommendAlgorithm(const cv::Size& image_size,
                                                           bool real_time_required,
                                                           bool high_accuracy_required) const {
    // Simple recommendation logic
    if (real_time_required && !high_accuracy_required) {
        if (image_size.width * image_size.height < 640 * 480) {
            return DetectionAlgorithm::LFFD;  // Best for small images
        } else {
            return DetectionAlgorithm::YOLO_V5;  // Fast and efficient
        }
    } else if (high_accuracy_required) {
        return DetectionAlgorithm::RETINANET;  // Best accuracy
    } else {
        return DetectionAlgorithm::SSD_MOBILENET;  // Balanced
    }
}

std::string AdvancedFaceDetector::getLastError() const {
    return last_error_;
}

bool AdvancedFaceDetector::hasError() const {
    return !last_error_.empty();
}

// Static methods
std::vector<AlgorithmProfile> AdvancedFaceDetector::getBuiltinProfiles() {
    return builtin_profiles;
}

bool AdvancedFaceDetector::isAlgorithmSupported(DetectionAlgorithm algorithm) {
    // For now, assume all algorithms are supported if OpenCV DNN is available
    (void)algorithm; // Suppress unused parameter warning
    return true;
}

std::string AdvancedFaceDetector::algorithmToString(DetectionAlgorithm algorithm) {
    switch (algorithm) {
    case DetectionAlgorithm::HAAR_CASCADE: return "Haar Cascade";
    case DetectionAlgorithm::DNN_CAFFE: return "DNN Caffe";
    case DetectionAlgorithm::DNN_TENSORFLOW: return "DNN TensorFlow";
    case DetectionAlgorithm::DNN_ONNX: return "DNN ONNX";
    case DetectionAlgorithm::YOLO_V3: return "YOLO v3";
    case DetectionAlgorithm::YOLO_V4: return "YOLO v4";
    case DetectionAlgorithm::YOLO_V5: return "YOLO v5";
    case DetectionAlgorithm::SSD_MOBILENET: return "SSD MobileNet";
    case DetectionAlgorithm::SSD_RESNET: return "SSD ResNet";
    case DetectionAlgorithm::RETINANET: return "RetinaNet";
    case DetectionAlgorithm::MTCNN: return "MTCNN";
    case DetectionAlgorithm::LFFD: return "LFFD";
    case DetectionAlgorithm::SCRFD: return "SCRFD";
    case DetectionAlgorithm::YOLO_FACE: return "YOLO-Face";
    default: return "Unknown";
    }
}

DetectionAlgorithm AdvancedFaceDetector::stringToAlgorithm(const std::string& name) {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    if (lower_name.find("yolo") != std::string::npos) {
        if (lower_name.find("v3") != std::string::npos) return DetectionAlgorithm::YOLO_V3;
        if (lower_name.find("v4") != std::string::npos) return DetectionAlgorithm::YOLO_V4;
        if (lower_name.find("v5") != std::string::npos) return DetectionAlgorithm::YOLO_V5;
        if (lower_name.find("face") != std::string::npos) return DetectionAlgorithm::YOLO_FACE;
        return DetectionAlgorithm::YOLO_V5; // Default YOLO
    }
    
    if (lower_name.find("ssd") != std::string::npos) {
        if (lower_name.find("resnet") != std::string::npos) return DetectionAlgorithm::SSD_RESNET;
        return DetectionAlgorithm::SSD_MOBILENET; // Default SSD
    }
    
    if (lower_name.find("retinanet") != std::string::npos) return DetectionAlgorithm::RETINANET;
    if (lower_name.find("mtcnn") != std::string::npos) return DetectionAlgorithm::MTCNN;
    if (lower_name.find("lffd") != std::string::npos) return DetectionAlgorithm::LFFD;
    if (lower_name.find("haar") != std::string::npos) return DetectionAlgorithm::HAAR_CASCADE;
    
    return DetectionAlgorithm::HAAR_CASCADE; // Default fallback
}

// Private method implementations
bool AdvancedFaceDetector::initializeAlgorithm(DetectionAlgorithm algorithm) {
    // Check if model is already loaded
    if (isModelLoaded(algorithm)) {
        return true;
    }

    // Try to load default model
    std::string model_path = config_.model_dir + config_.model_paths[algorithm];

    // For algorithms that don't require external models
    if (algorithm == DetectionAlgorithm::HAAR_CASCADE) {
        return true; // Handled by base FaceDetector
    }

    // Check if model file exists
    std::ifstream file(model_path);
    if (!file.good()) {
        setError("Model file not found: " + model_path);
        return false;
    }

    return loadModel(algorithm, model_path);
}

std::vector<AdvancedFaceDetection> AdvancedFaceDetector::detectWithYOLO(const cv::Mat& image) {
    std::vector<AdvancedFaceDetection> detections;

    auto it = loaded_models_.find(current_algorithm_);
    if (it == loaded_models_.end()) {
        setError("YOLO model not loaded");
        return detections;
    }

    cv::dnn::Net& net = it->second;

    try {
        // Preprocess image
        cv::Mat blob;
        cv::dnn::blobFromImage(image, blob, 1.0/255.0, config_.input_size,
                              cv::Scalar(0, 0, 0), true, false);

        // Set input
        net.setInput(blob);

        // Run forward pass
        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());

        // Parse YOLO outputs
        float conf_threshold = config_.yolo_confidence;
        float nms_threshold = config_.yolo_nms;

        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> class_ids;

        for (size_t i = 0; i < outputs.size(); ++i) {
            float* data = (float*)outputs[i].data;
            for (int j = 0; j < outputs[i].rows; ++j, data += outputs[i].cols) {
                cv::Mat scores = outputs[i].row(j).colRange(5, outputs[i].cols);
                cv::Point class_id_point;
                double confidence;
                minMaxLoc(scores, 0, &confidence, 0, &class_id_point);

                if (confidence > conf_threshold) {
                    int center_x = (int)(data[0] * image.cols);
                    int center_y = (int)(data[1] * image.rows);
                    int width = (int)(data[2] * image.cols);
                    int height = (int)(data[3] * image.rows);
                    int left = center_x - width / 2;
                    int top = center_y - height / 2;

                    boxes.push_back(cv::Rect(left, top, width, height));
                    confidences.push_back((float)confidence);
                    class_ids.push_back(class_id_point.x);
                }
            }
        }

        // Apply NMS
        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, confidences, conf_threshold, nms_threshold, indices);

        // Convert to AdvancedFaceDetection
        for (size_t i = 0; i < indices.size(); ++i) {
            int idx = indices[i];
            AdvancedFaceDetection detection;
            detection.bbox = boxes[idx];
            detection.confidence = confidences[idx];
            detection.center = cv::Point2f(detection.bbox.x + detection.bbox.width/2.0f,
                                         detection.bbox.y + detection.bbox.height/2.0f);
            detection.method = algorithmToString(current_algorithm_);
            detection.algorithm_used = current_algorithm_;

            detections.push_back(detection);
        }

    } catch (const cv::Exception& e) {
        setError("YOLO detection error: " + std::string(e.what()));
    }

    return detections;
}

std::vector<AdvancedFaceDetection> AdvancedFaceDetector::detectWithSSD(const cv::Mat& image) {
    std::vector<AdvancedFaceDetection> detections;

    auto it = loaded_models_.find(current_algorithm_);
    if (it == loaded_models_.end()) {
        setError("SSD model not loaded");
        return detections;
    }

    cv::dnn::Net& net = it->second;

    try {
        // Preprocess image
        cv::Mat blob;
        cv::dnn::blobFromImage(image, blob, 1.0, config_.ssd_input_size,
                              config_.mean, config_.swap_rb, false);

        // Set input and run inference
        net.setInput(blob);
        cv::Mat detection = net.forward();

        // Parse SSD outputs
        cv::Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

        for (int i = 0; i < detectionMat.rows; i++) {
            float confidence = detectionMat.at<float>(i, 2);

            if (confidence > config_.ssd_confidence) {
                int x1 = static_cast<int>(detectionMat.at<float>(i, 3) * image.cols);
                int y1 = static_cast<int>(detectionMat.at<float>(i, 4) * image.rows);
                int x2 = static_cast<int>(detectionMat.at<float>(i, 5) * image.cols);
                int y2 = static_cast<int>(detectionMat.at<float>(i, 6) * image.rows);

                cv::Rect bbox(x1, y1, x2 - x1, y2 - y1);

                // Validate bounding box
                if (bbox.x >= 0 && bbox.y >= 0 &&
                    bbox.x + bbox.width <= image.cols &&
                    bbox.y + bbox.height <= image.rows &&
                    bbox.width > 0 && bbox.height > 0) {

                    AdvancedFaceDetection face_detection;
                    face_detection.bbox = bbox;
                    face_detection.confidence = confidence;
                    face_detection.center = cv::Point2f(bbox.x + bbox.width/2.0f,
                                                       bbox.y + bbox.height/2.0f);
                    face_detection.method = algorithmToString(current_algorithm_);
                    face_detection.algorithm_used = current_algorithm_;

                    detections.push_back(face_detection);
                }
            }
        }

    } catch (const cv::Exception& e) {
        setError("SSD detection error: " + std::string(e.what()));
    }

    return detections;
}

std::vector<AdvancedFaceDetection> AdvancedFaceDetector::detectWithRetinaNet(const cv::Mat& image) {
    std::vector<AdvancedFaceDetection> detections;

    auto it = loaded_models_.find(current_algorithm_);
    if (it == loaded_models_.end()) {
        setError("RetinaNet model not loaded");
        return detections;
    }

    cv::dnn::Net& net = it->second;

    try {
        // Preprocess image
        cv::Mat blob;
        cv::dnn::blobFromImage(image, blob, 1.0, config_.retinanet_input_size,
                              cv::Scalar(103.94, 116.78, 123.68), false, false);

        // Set input and run inference
        net.setInput(blob);
        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());

        // Parse RetinaNet outputs (simplified)
        for (const auto& output : outputs) {
            if (output.dims == 2) {
                for (int i = 0; i < output.rows; i++) {
                    const float* data = output.ptr<float>(i);
                    float confidence = data[4]; // Assuming confidence is at index 4

                    if (confidence > config_.retinanet_confidence) {
                        int x1 = static_cast<int>(data[0]);
                        int y1 = static_cast<int>(data[1]);
                        int x2 = static_cast<int>(data[2]);
                        int y2 = static_cast<int>(data[3]);

                        cv::Rect bbox(x1, y1, x2 - x1, y2 - y1);

                        if (bbox.x >= 0 && bbox.y >= 0 &&
                            bbox.x + bbox.width <= image.cols &&
                            bbox.y + bbox.height <= image.rows &&
                            bbox.width > 0 && bbox.height > 0) {

                            AdvancedFaceDetection detection;
                            detection.bbox = bbox;
                            detection.confidence = confidence;
                            detection.center = cv::Point2f(bbox.x + bbox.width/2.0f,
                                                          bbox.y + bbox.height/2.0f);
                            detection.method = algorithmToString(current_algorithm_);
                            detection.algorithm_used = current_algorithm_;

                            detections.push_back(detection);
                        }
                    }
                }
            }
        }

    } catch (const cv::Exception& e) {
        setError("RetinaNet detection error: " + std::string(e.what()));
    }

    return detections;
}

std::vector<AdvancedFaceDetection> AdvancedFaceDetector::detectWithMTCNN(const cv::Mat& image) {
    std::vector<AdvancedFaceDetection> detections;

    auto it = loaded_models_.find(current_algorithm_);
    if (it == loaded_models_.end()) {
        setError("MTCNN model not loaded");
        return detections;
    }

    // MTCNN typically requires three networks (P-Net, R-Net, O-Net)
    // This is a simplified implementation
    cv::dnn::Net& net = it->second;

    try {
        // Preprocess image
        cv::Mat processed = preprocessImage(image, DetectionAlgorithm::MTCNN);

        cv::Mat blob;
        cv::dnn::blobFromImage(processed, blob, 1.0, processed.size(),
                              cv::Scalar(0, 0, 0), false, false);

        // Set input and run inference
        net.setInput(blob);
        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());

        // Parse MTCNN outputs (simplified)
        if (!outputs.empty()) {
            cv::Mat detection_output = outputs[0];

            for (int i = 0; i < detection_output.rows; i++) {
                const float* data = detection_output.ptr<float>(i);
                float confidence = data[4];

                if (confidence > config_.mtcnn_thresholds[0]) {
                    int x1 = static_cast<int>(data[0] * image.cols);
                    int y1 = static_cast<int>(data[1] * image.rows);
                    int x2 = static_cast<int>(data[2] * image.cols);
                    int y2 = static_cast<int>(data[3] * image.rows);

                    cv::Rect bbox(x1, y1, x2 - x1, y2 - y1);

                    if (bbox.width >= config_.mtcnn_min_face_size &&
                        bbox.height >= config_.mtcnn_min_face_size &&
                        bbox.x >= 0 && bbox.y >= 0 &&
                        bbox.x + bbox.width <= image.cols &&
                        bbox.y + bbox.height <= image.rows) {

                        AdvancedFaceDetection detection;
                        detection.bbox = bbox;
                        detection.confidence = confidence;
                        detection.center = cv::Point2f(bbox.x + bbox.width/2.0f,
                                                      bbox.y + bbox.height/2.0f);
                        detection.method = algorithmToString(current_algorithm_);
                        detection.algorithm_used = current_algorithm_;

                        // MTCNN can provide facial landmarks
                        if (outputs.size() > 1) {
                            cv::Mat landmarks_output = outputs[1];
                            if (i < landmarks_output.rows) {
                                const float* landmark_data = landmarks_output.ptr<float>(i);
                                for (int j = 0; j < 5; j++) { // 5 landmarks typically
                                    float x = landmark_data[j * 2] * image.cols;
                                    float y = landmark_data[j * 2 + 1] * image.rows;
                                    detection.landmarks.push_back(cv::Point2f(x, y));
                                }
                            }
                        }

                        detections.push_back(detection);
                    }
                }
            }
        }

    } catch (const cv::Exception& e) {
        setError("MTCNN detection error: " + std::string(e.what()));
    }

    return detections;
}

std::vector<AdvancedFaceDetection> AdvancedFaceDetector::detectWithLFFD(const cv::Mat& image) {
    std::vector<AdvancedFaceDetection> detections;

    auto it = loaded_models_.find(current_algorithm_);
    if (it == loaded_models_.end()) {
        setError("LFFD model not loaded");
        return detections;
    }

    cv::dnn::Net& net = it->second;

    try {
        // Preprocess image
        cv::Mat blob;
        cv::dnn::blobFromImage(image, blob, 1.0/255.0, config_.lffd_input_size,
                              cv::Scalar(0, 0, 0), true, false);

        // Set input and run inference
        net.setInput(blob);
        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());

        // Parse LFFD outputs
        for (const auto& output : outputs) {
            if (output.dims >= 2) {
                for (int i = 0; i < output.rows; i++) {
                    const float* data = output.ptr<float>(i);
                    float confidence = data[4]; // Assuming confidence is at index 4

                    if (confidence > config_.lffd_confidence) {
                        // Scale coordinates back to original image size
                        float scale_x = static_cast<float>(image.cols) / config_.lffd_input_size.width;
                        float scale_y = static_cast<float>(image.rows) / config_.lffd_input_size.height;

                        int x1 = static_cast<int>(data[0] * scale_x);
                        int y1 = static_cast<int>(data[1] * scale_y);
                        int x2 = static_cast<int>(data[2] * scale_x);
                        int y2 = static_cast<int>(data[3] * scale_y);

                        cv::Rect bbox(x1, y1, x2 - x1, y2 - y1);

                        if (bbox.x >= 0 && bbox.y >= 0 &&
                            bbox.x + bbox.width <= image.cols &&
                            bbox.y + bbox.height <= image.rows &&
                            bbox.width > 0 && bbox.height > 0) {

                            AdvancedFaceDetection detection;
                            detection.bbox = bbox;
                            detection.confidence = confidence;
                            detection.center = cv::Point2f(bbox.x + bbox.width/2.0f,
                                                          bbox.y + bbox.height/2.0f);
                            detection.method = algorithmToString(current_algorithm_);
                            detection.algorithm_used = current_algorithm_;

                            detections.push_back(detection);
                        }
                    }
                }
            }
        }

    } catch (const cv::Exception& e) {
        setError("LFFD detection error: " + std::string(e.what()));
    }

    return detections;
}

void AdvancedFaceDetector::drawAdvancedDetections(cv::Mat& image,
                                                  const std::vector<AdvancedFaceDetection>& faces) const {
    for (size_t i = 0; i < faces.size(); ++i) {
        const auto& face = faces[i];

        // Choose color based on algorithm
        cv::Scalar color;
        switch (face.algorithm_used) {
        case DetectionAlgorithm::YOLO_V3:
        case DetectionAlgorithm::YOLO_V4:
        case DetectionAlgorithm::YOLO_V5:
        case DetectionAlgorithm::YOLO_FACE:
            color = cv::Scalar(0, 255, 0); // Green for YOLO
            break;
        case DetectionAlgorithm::SSD_MOBILENET:
        case DetectionAlgorithm::SSD_RESNET:
            color = cv::Scalar(255, 0, 0); // Blue for SSD
            break;
        case DetectionAlgorithm::RETINANET:
            color = cv::Scalar(0, 0, 255); // Red for RetinaNet
            break;
        case DetectionAlgorithm::MTCNN:
            color = cv::Scalar(255, 255, 0); // Cyan for MTCNN
            break;
        case DetectionAlgorithm::LFFD:
            color = cv::Scalar(255, 0, 255); // Magenta for LFFD
            break;
        default:
            color = cv::Scalar(128, 128, 128); // Gray for others
            break;
        }

        // Draw bounding box
        cv::rectangle(image, face.bbox, color, 2);

        // Draw confidence
        std::string conf_text = cv::format("%.2f", face.confidence);
        cv::putText(image, conf_text,
                   cv::Point(face.bbox.x, face.bbox.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);

        // Draw algorithm name
        std::string algo_text = algorithmToString(face.algorithm_used);
        cv::putText(image, algo_text,
                   cv::Point(face.bbox.x, face.bbox.y + face.bbox.height + 15),
                   cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);

        // Draw landmarks if available
        if (!face.landmarks.empty()) {
            for (const auto& landmark : face.landmarks) {
                cv::circle(image, landmark, 2, color, -1);
            }
        }

        // Draw detection time if available
        if (face.detection_time_ms > 0) {
            std::string time_text = cv::format("%.1fms", face.detection_time_ms);
            cv::putText(image, time_text,
                       cv::Point(face.bbox.x + face.bbox.width - 50, face.bbox.y - 5),
                       cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
        }
    }
}

void AdvancedFaceDetector::setError(const std::string& error) const {
    last_error_ = error;
}

void AdvancedFaceDetector::updateProfilingResults(const std::string& operation, double time_ms) {
    if (profiling_enabled_) {
        profiling_results_[operation + "_time"] = time_ms;
        profiling_results_[operation + "_count"] = profiling_results_[operation + "_count"] + 1;
    }
}

// AdvancedDetectorUtils namespace implementation
namespace AdvancedDetectorUtils {

bool downloadModel(DetectionAlgorithm algorithm, const std::string& destination_dir) {
    // This would implement model downloading from URLs
    // For now, return false as models need to be manually provided
    (void)algorithm;
    (void)destination_dir;
    return false;
}

std::vector<std::string> getRequiredFiles(DetectionAlgorithm algorithm) {
    std::vector<std::string> files;

    switch (algorithm) {
    case DetectionAlgorithm::YOLO_V3:
        files = {"yolov3-face.cfg", "yolov3-face.weights"};
        break;
    case DetectionAlgorithm::YOLO_V4:
        files = {"yolov4-face.cfg", "yolov4-face.weights"};
        break;
    case DetectionAlgorithm::YOLO_V5:
        files = {"yolov5s-face.onnx"};
        break;
    case DetectionAlgorithm::SSD_MOBILENET:
        files = {"ssd_mobilenet_face.pb", "ssd_mobilenet_face.pbtxt"};
        break;
    case DetectionAlgorithm::SSD_RESNET:
        files = {"ssd_resnet_face.pb", "ssd_resnet_face.pbtxt"};
        break;
    case DetectionAlgorithm::RETINANET:
        files = {"retinanet_face.onnx"};
        break;
    case DetectionAlgorithm::MTCNN:
        files = {"mtcnn_pnet.onnx", "mtcnn_rnet.onnx", "mtcnn_onet.onnx"};
        break;
    case DetectionAlgorithm::LFFD:
        files = {"lffd_face.onnx"};
        break;
    case DetectionAlgorithm::YOLO_FACE:
        files = {"yolo_face.onnx"};
        break;
    default:
        break;
    }

    return files;
}

bool verifyModelFiles(DetectionAlgorithm algorithm, const std::string& model_dir) {
    auto required_files = getRequiredFiles(algorithm);

    for (const auto& file : required_files) {
        std::string full_path = model_dir + "/" + file;
        std::ifstream f(full_path);
        if (!f.good()) {
            return false;
        }
    }

    return true;
}

std::vector<BenchmarkResult> benchmarkAlgorithms(
    const std::vector<cv::Mat>& test_images,
    const std::vector<DetectionAlgorithm>& algorithms) {

    std::vector<BenchmarkResult> results;

    for (auto algorithm : algorithms) {
        BenchmarkResult result;
        result.algorithm = algorithm;
        result.total_detections = 0;

        AdvancedFaceDetector detector;
        if (!detector.initialize(algorithm)) {
            continue; // Skip if initialization fails
        }

        detector.enableProfiling(true);

        auto start_time = std::chrono::high_resolution_clock::now();

        for (const auto& image : test_images) {
            auto detections = detector.detectFaces(image);
            result.total_detections += detections.size();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();

        result.avg_inference_time_ms = static_cast<double>(total_time) / test_images.size();
        result.avg_fps = 1000.0 / result.avg_inference_time_ms;
        result.memory_usage_mb = 0; // Would need platform-specific implementation
        result.accuracy_score = 0; // Would need ground truth data

        results.push_back(result);
    }

    return results;
}

void printAlgorithmComparison(const std::vector<AlgorithmProfile>& profiles) {
    std::cout << "\n=== Algorithm Comparison ===" << std::endl;
    std::cout << std::left << std::setw(15) << "Algorithm"
              << std::setw(8) << "Speed"
              << std::setw(10) << "Accuracy"
              << std::setw(8) << "Memory"
              << std::setw(10) << "Min RAM"
              << std::setw(6) << "GPU" << std::endl;
    std::cout << std::string(65, '-') << std::endl;

    for (const auto& profile : profiles) {
        std::cout << std::left << std::setw(15) << profile.name
                  << std::setw(8) << (std::string(profile.speed_rating, '*'))
                  << std::setw(10) << (std::string(profile.accuracy_rating, '*'))
                  << std::setw(8) << (std::string(profile.memory_efficiency, '*'))
                  << std::setw(10) << (std::to_string(profile.min_memory_mb) + "MB")
                  << std::setw(6) << (profile.requires_gpu ? "Yes" : "No") << std::endl;
    }
    std::cout << std::string(65, '-') << std::endl;
    std::cout << "Rating: * = Poor, ***** = Excellent" << std::endl;
}

AlgorithmProfile findBestAlgorithm(const std::vector<AlgorithmProfile>& profiles,
                                  bool prioritize_speed) {
    if (profiles.empty()) {
        return AlgorithmProfile();
    }

    AlgorithmProfile best = profiles[0];

    for (const auto& profile : profiles) {
        if (prioritize_speed) {
            if (profile.speed_rating > best.speed_rating ||
                (profile.speed_rating == best.speed_rating &&
                 profile.accuracy_rating > best.accuracy_rating)) {
                best = profile;
            }
        } else {
            if (profile.accuracy_rating > best.accuracy_rating ||
                (profile.accuracy_rating == best.accuracy_rating &&
                 profile.speed_rating > best.speed_rating)) {
                best = profile;
            }
        }
    }

    return best;
}

bool convertModel(const std::string& source_path, const std::string& target_path,
                 const std::string& source_format, const std::string& target_format) {
    // Model conversion would require specific libraries like ONNX, TensorRT, etc.
    // This is a placeholder implementation
    (void)source_path;
    (void)target_path;
    (void)source_format;
    (void)target_format;
    return false;
}

} // namespace AdvancedDetectorUtils

// Constants implementation
namespace AdvancedDetectorConstants {

const std::map<DetectionAlgorithm, std::string> MODEL_URLS = {
    {DetectionAlgorithm::YOLO_V3, "https://github.com/spmallick/learnopencv/raw/master/FaceDetectionComparison/models/yolov3-face.weights"},
    {DetectionAlgorithm::YOLO_V4, "https://github.com/AlexeyAB/darknet/releases/download/darknet_yolo_v4_pre/yolov4-face.weights"},
    {DetectionAlgorithm::YOLO_V5, "https://github.com/deepcam-cn/yolov5-face/releases/download/v6.0/yolov5s-face.onnx"},
    {DetectionAlgorithm::SSD_MOBILENET, "https://github.com/opencv/opencv_extra/raw/master/testdata/dnn/opencv_face_detector_uint8.pb"},
    {DetectionAlgorithm::RETINANET, "https://github.com/onnx/models/raw/master/vision/object_detection_segmentation/retinanet/model/retinanet-9.onnx"},
    {DetectionAlgorithm::MTCNN, "https://github.com/kpzhang93/MTCNN_face_detection_alignment/raw/master/code/codes/MTCNNv1/model/det1.caffemodel"},
    {DetectionAlgorithm::LFFD, "https://github.com/YonghaoHe/A-Light-and-Fast-Face-Detector-for-Edge-Devices/raw/master/LFFD_original/model/LFFD_25M_8.onnx"},
    {DetectionAlgorithm::YOLO_FACE, "https://github.com/deepcam-cn/yolov5-face/releases/download/v6.0/yolov5n-face.onnx"}
};

const std::map<DetectionAlgorithm, cv::Size> RECOMMENDED_INPUT_SIZES = {
    {DetectionAlgorithm::YOLO_V3, cv::Size(416, 416)},
    {DetectionAlgorithm::YOLO_V4, cv::Size(608, 608)},
    {DetectionAlgorithm::YOLO_V5, cv::Size(640, 640)},
    {DetectionAlgorithm::SSD_MOBILENET, cv::Size(300, 300)},
    {DetectionAlgorithm::SSD_RESNET, cv::Size(300, 300)},
    {DetectionAlgorithm::RETINANET, cv::Size(640, 640)},
    {DetectionAlgorithm::MTCNN, cv::Size(48, 48)},
    {DetectionAlgorithm::LFFD, cv::Size(480, 640)},
    {DetectionAlgorithm::YOLO_FACE, cv::Size(640, 640)}
};

} // namespace AdvancedDetectorConstants

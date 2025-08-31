/*
 * Advanced Face Detection Demo
 * 
 * This demo showcases all available detection algorithms and allows
 * users to compare their performance and accuracy.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#include "advanced_face_detector.h"
#include "camera_capture.h"
#include "config_manager.h"
#include "face_detection_demo.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>

class AdvancedFaceDetectionDemo {
private:
    AdvancedFaceDetector detector_;
    std::unique_ptr<CameraCapture> camera_;
    DetectionAlgorithm current_algorithm_;
    bool running_;
    bool show_comparison_;
    FaceDetectionConfig face_config_;  // Add config as member variable

    // Performance tracking
    std::map<DetectionAlgorithm, double> algorithm_fps_;
    
public:
    AdvancedFaceDetectionDemo() 
        : current_algorithm_(DetectionAlgorithm::HAAR_CASCADE),
          running_(false),
          show_comparison_(false) {
        camera_ = std::make_unique<CameraCapture>();
    }
    
    bool initialize() {
        // Load configuration
        ConfigManager config_manager;

        if (config_manager.loadConfig("config/default_config.json", face_config_)) {
            std::cout << "Loaded camera config: " << face_config_.width << "x" << face_config_.height << std::endl;
        } else {
            std::cout << "Failed to load config, using defaults" << std::endl;
            // Set smaller default resolution
            face_config_.width = 320;
            face_config_.height = 240;
            face_config_.fps = 30;
        }

        // Initialize camera with fallback options
        bool camera_initialized = false;

        std::cout << "Attempting to initialize camera..." << std::endl;
        std::cout << "Using resolution: " << face_config_.width << "x" << face_config_.height << std::endl;

        // Try camera 0 first
        std::cout << "Trying camera ID 0..." << std::endl;
        if (camera_->initialize(0)) {
            // Set resolution after initialization
            if (camera_->setResolution(face_config_.width, face_config_.height)) {
                camera_initialized = true;
                std::cout << "Successfully initialized camera using ID 0 with custom resolution" << std::endl;
            } else {
                std::cout << "Camera 0 initialized but failed to set resolution, using default" << std::endl;
                camera_initialized = true;
            }
        } else {
            std::cerr << "Camera 0 failed, trying alternative methods..." << std::endl;

            // Try with explicit device path (we know this works from our tests)
            std::cout << "Trying /dev/video0..." << std::endl;
            camera_ = std::make_unique<CameraCapture>();
            if (camera_->initialize("/dev/video0")) {
                // Set resolution after initialization
                if (camera_->setResolution(face_config_.width, face_config_.height)) {
                    camera_initialized = true;
                    std::cout << "Successfully initialized camera using /dev/video0 with custom resolution" << std::endl;
                } else {
                    std::cout << "Camera /dev/video0 initialized but failed to set resolution, using default" << std::endl;
                    camera_initialized = true;
                }
            } else {
                std::cerr << "Failed to initialize /dev/video0" << std::endl;

                // Try different camera IDs
                for (int i = 1; i <= 3; ++i) {
                    std::cout << "Trying camera ID " << i << "..." << std::endl;
                    camera_ = std::make_unique<CameraCapture>();
                    if (camera_->initialize(i)) {
                        // Set resolution after initialization
                        if (camera_->setResolution(face_config_.width, face_config_.height)) {
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

        if (!camera_initialized) {
            std::cerr << "Failed to initialize camera with all methods" << std::endl;
            return false;
        }
        
        // Initialize detector with default algorithm
        if (!detector_.initialize(current_algorithm_)) {
            std::cerr << "Failed to initialize detector" << std::endl;
            return false;
        }
        
        detector_.enableProfiling(true);
        
        std::cout << "Advanced Face Detection Demo initialized" << std::endl;
        printAvailableAlgorithms();
        printControls();
        
        return true;
    }
    
    void printAvailableAlgorithms() {
        std::cout << "\n=== Available Detection Algorithms ===" << std::endl;
        auto profiles = detector_.getAllProfiles();
        
        int index = 1;
        for (const auto& profile : profiles) {
            std::cout << index++ << ". " << profile.name 
                      << " - " << profile.description << std::endl;
            std::cout << "   Speed: " << std::string(profile.speed_rating, '*')
                      << ", Accuracy: " << std::string(profile.accuracy_rating, '*')
                      << ", Memory: " << std::string(profile.memory_efficiency, '*') << std::endl;
        }
        std::cout << std::endl;
    }
    
    void printControls() {
        std::cout << "=== Controls ===" << std::endl;
        std::cout << "1-9: Switch detection algorithm" << std::endl;
        std::cout << "'c': Show algorithm comparison" << std::endl;
        std::cout << "'b': Run benchmark on all algorithms" << std::endl;
        std::cout << "'r': Get algorithm recommendation" << std::endl;
        std::cout << "'s': Save current frame" << std::endl;
        std::cout << "'p': Toggle profiling display" << std::endl;
        std::cout << "ESC/q: Quit" << std::endl;
        std::cout << std::endl;
    }
    
    int run() {
        if (!camera_->start()) {
            std::cerr << "Failed to start camera" << std::endl;
            return -1;
        }
        
        running_ = true;
        cv::Mat frame;
        
        const std::string window_name = "Face Demo";
        cv::namedWindow(window_name, cv::WINDOW_NORMAL);
        cv::resizeWindow(window_name, face_config_.width, face_config_.height);
        
        auto last_fps_time = std::chrono::steady_clock::now();
        int frame_count = 0;
        double current_fps = 0.0;
        
        while (running_) {
            // Capture frame
            if (!camera_->captureFrame(frame)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            frame_count++;
            
            // Detect faces
            auto start_time = std::chrono::high_resolution_clock::now();
            auto detections = detector_.detectFaces(frame);
            auto end_time = std::chrono::high_resolution_clock::now();
            
            auto detection_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time).count();
            
            // Draw detections
            detector_.drawAdvancedDetections(frame, detections);
            
            // Calculate FPS
            auto now = std::chrono::steady_clock::now();
            auto fps_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_fps_time);
            
            if (fps_duration.count() >= 1000) {
                current_fps = frame_count * 1000.0 / fps_duration.count();
                algorithm_fps_[current_algorithm_] = current_fps;
                frame_count = 0;
                last_fps_time = now;
            }
            
            // Draw information overlay
            drawInfoOverlay(frame, detections.size(), current_fps, detection_time);
            
            // Show comparison if enabled
            if (show_comparison_) {
                drawComparisonOverlay(frame);
            }
            
            // Display frame
            cv::imshow(window_name, frame);
            
            // Handle key press
            int key = cv::waitKey(1) & 0xFF;
            if (!handleKeyPress(key, frame)) {
                break;
            }
        }
        
        cv::destroyAllWindows();
        return 0;
    }
    
private:
    void drawInfoOverlay(cv::Mat& frame, int face_count, double fps, double detection_time) {
        // Use smaller font and more compact layout
        int font = cv::FONT_HERSHEY_SIMPLEX;
        double font_scale = 0.4;
        int thickness = 1;
        cv::Scalar color(0, 255, 0);

        // Get short algorithm name
        std::string algo_name = getShortAlgorithmName(current_algorithm_);

        // Compact info display - single line
        std::string info_text = algo_name + " | " +
                               std::to_string(static_cast<int>(fps)) + "fps | " +
                               std::to_string(detection_time) + "ms | " +
                               std::to_string(face_count) + " faces";

        cv::putText(frame, info_text, cv::Point(5, 15),
                   font, font_scale, color, thickness);

        // Optional: Show algorithm rating in second line (very compact)
        auto profile = detector_.getAlgorithmProfile(current_algorithm_);
        std::string rating_text = "S:" + std::string(profile.speed_rating, '*') +
                                 " A:" + std::string(profile.accuracy_rating, '*');
        cv::putText(frame, rating_text, cv::Point(5, 30),
                   font, 0.3, cv::Scalar(255, 255, 0), 1);
    }

    std::string getShortAlgorithmName(DetectionAlgorithm algo) {
        switch (algo) {
            case DetectionAlgorithm::HAAR_CASCADE: return "Haar";
            case DetectionAlgorithm::DNN_CAFFE: return "DNN";
            case DetectionAlgorithm::DNN_TENSORFLOW: return "TF";
            case DetectionAlgorithm::DNN_ONNX: return "ONNX";
            case DetectionAlgorithm::YOLO_V3: return "YOLOv3";
            case DetectionAlgorithm::YOLO_V4: return "YOLOv4";
            case DetectionAlgorithm::YOLO_V5: return "YOLOv5";
            case DetectionAlgorithm::SSD_MOBILENET: return "SSD-MB";
            case DetectionAlgorithm::SSD_RESNET: return "SSD-RN";
            case DetectionAlgorithm::RETINANET: return "RetNet";
            case DetectionAlgorithm::MTCNN: return "MTCNN";
            case DetectionAlgorithm::LFFD: return "LFFD";
            case DetectionAlgorithm::YOLO_FACE: return "YOLO-F";
            default: return "Unknown";
        }
    }
    
    void drawComparisonOverlay(cv::Mat& frame) {
        int y_offset = frame.rows - 200;
        int x_offset = 10;
        
        cv::putText(frame, "Algorithm Performance Comparison:", 
                   cv::Point(x_offset, y_offset), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
        
        y_offset += 25;
        
        for (const auto& pair : algorithm_fps_) {
            std::string algo_name = AdvancedFaceDetector::algorithmToString(pair.first);
            std::string fps_text = algo_name + ": " + 
                                  std::to_string(static_cast<int>(pair.second)) + " FPS";
            
            cv::Scalar color = (pair.first == current_algorithm_) ? 
                              cv::Scalar(0, 255, 0) : cv::Scalar(255, 255, 255);
            
            cv::putText(frame, fps_text, cv::Point(x_offset, y_offset), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
            y_offset += 20;
        }
    }
    
    bool handleKeyPress(int key, const cv::Mat& frame) {
        switch (key) {
        case 27: // ESC
        case 'q':
            running_ = false;
            return false;
            
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            switchAlgorithm(key - '1');
            break;
            
        case 'c':
            show_comparison_ = !show_comparison_;
            std::cout << "Comparison overlay " << (show_comparison_ ? "enabled" : "disabled") << std::endl;
            break;
            
        case 'b':
            runBenchmark();
            break;
            
        case 'r':
            showRecommendation(frame.size());
            break;
            
        case 's':
            saveFrame(frame);
            break;
            
        case 'p':
            printProfilingResults();
            break;
            
        default:
            break;
        }
        
        return true;
    }
    
    void switchAlgorithm(int index) {
        auto profiles = detector_.getAllProfiles();
        if (index >= 0 && index < static_cast<int>(profiles.size())) {
            DetectionAlgorithm new_algorithm = profiles[index].algorithm;
            
            std::cout << "Switching to: " << profiles[index].name << std::endl;
            
            if (detector_.initialize(new_algorithm)) {
                current_algorithm_ = new_algorithm;
                std::cout << "Algorithm switched successfully" << std::endl;
            } else {
                std::cout << "Failed to switch algorithm: " << detector_.getLastError() << std::endl;
            }
        }
    }
    
    void runBenchmark() {
        std::cout << "\nRunning benchmark on available algorithms..." << std::endl;
        
        // Capture test frames
        std::vector<cv::Mat> test_frames;
        for (int i = 0; i < 10; ++i) {
            cv::Mat frame;
            if (camera_->captureFrame(frame)) {
                test_frames.push_back(frame.clone());
            }
        }
        
        if (test_frames.empty()) {
            std::cout << "No test frames captured" << std::endl;
            return;
        }
        
        auto algorithms = detector_.getAvailableAlgorithms();
        auto results = AdvancedDetectorUtils::benchmarkAlgorithms(test_frames, algorithms);
        
        std::cout << "\n=== Benchmark Results ===" << std::endl;
        std::cout << std::left << std::setw(15) << "Algorithm" 
                  << std::setw(12) << "Avg Time(ms)" 
                  << std::setw(10) << "Avg FPS" 
                  << std::setw(12) << "Detections" << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        
        for (const auto& result : results) {
            std::cout << std::left << std::setw(15) 
                      << AdvancedFaceDetector::algorithmToString(result.algorithm)
                      << std::setw(12) << std::fixed << std::setprecision(1) 
                      << result.avg_inference_time_ms
                      << std::setw(10) << std::fixed << std::setprecision(1) 
                      << result.avg_fps
                      << std::setw(12) << result.total_detections << std::endl;
        }
        std::cout << std::endl;
    }
    
    void showRecommendation(const cv::Size& image_size) {
        std::cout << "\n=== Algorithm Recommendation ===" << std::endl;
        
        auto real_time_algo = detector_.recommendAlgorithm(image_size, true, false);
        auto accuracy_algo = detector_.recommendAlgorithm(image_size, false, true);
        
        std::cout << "For real-time performance: " 
                  << AdvancedFaceDetector::algorithmToString(real_time_algo) << std::endl;
        std::cout << "For high accuracy: " 
                  << AdvancedFaceDetector::algorithmToString(accuracy_algo) << std::endl;
        std::cout << std::endl;
    }
    
    void saveFrame(const cv::Mat& frame) {
        std::string filename = "advanced_detection_" + 
                              std::to_string(std::time(nullptr)) + ".jpg";
        cv::imwrite(filename, frame);
        std::cout << "Frame saved: " << filename << std::endl;
    }
    
    void printProfilingResults() {
        auto results = detector_.getProfilingResults();
        if (results.empty()) {
            std::cout << "No profiling data available" << std::endl;
            return;
        }
        
        std::cout << "\n=== Profiling Results ===" << std::endl;
        for (const auto& pair : results) {
            std::cout << pair.first << ": " << pair.second << std::endl;
        }
        std::cout << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "=== Advanced Face Detection Demo ===" << std::endl;
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    std::cout << std::endl;
    
    // Parse command line arguments
    if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --help, -h    Show this help message" << std::endl;
        std::cout << "  --list        List available algorithms" << std::endl;
        return 0;
    }
    
    if (argc > 1 && std::string(argv[1]) == "--list") {
        AdvancedFaceDetector detector;
        auto profiles = detector.getAllProfiles();
        AdvancedDetectorUtils::printAlgorithmComparison(profiles);
        return 0;
    }
    
    AdvancedFaceDetectionDemo demo;
    
    if (!demo.initialize()) {
        std::cerr << "Failed to initialize demo" << std::endl;
        return -1;
    }
    
    return demo.run();
}

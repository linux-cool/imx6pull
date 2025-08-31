/*
 * Face Recognition Application for IMX6ULL Pro
 * 
 * This application provides real-time face detection and recognition
 * using USB camera and optimized algorithms for ARM Cortex-A7
 * 
 * Author: Face Recognition Team
 * License: MIT
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

#include "camera_api.h"
#include "face_engine.h"
#include "network_manager.h"
#include "config_manager.h"

class FaceRecognitionApp {
public:
    FaceRecognitionApp();
    ~FaceRecognitionApp();
    
    // Application lifecycle
    int Initialize(const std::string& config_file);
    int Run();
    void Shutdown();
    
    // Configuration
    int LoadConfig(const std::string& config_file);
    void PrintConfig() const;
    
private:
    // Core components
    std::unique_ptr<CameraAPI> camera_;
    std::unique_ptr<FaceEngine> face_engine_;
    std::unique_ptr<NetworkManager> network_;
    std::unique_ptr<ConfigManager> config_;
    
    // Threading
    std::thread capture_thread_;
    std::thread process_thread_;
    std::thread display_thread_;
    
    // Synchronization
    std::mutex frame_mutex_;
    std::condition_variable frame_cv_;
    std::atomic<bool> running_;
    
    // Frame buffers
    cv::Mat current_frame_;
    cv::Mat display_frame_;
    bool new_frame_available_;
    
    // Statistics
    std::atomic<int> frames_processed_;
    std::atomic<int> faces_detected_;
    std::atomic<int> faces_recognized_;
    std::chrono::steady_clock::time_point start_time_;
    
    // Configuration parameters
    struct AppConfig {
        // Camera settings
        int camera_id;
        int frame_width;
        int frame_height;
        int frame_fps;
        
        // Processing settings
        int process_width;
        int process_height;
        float detection_threshold;
        float recognition_threshold;
        int max_faces;
        
        // Network settings
        bool enable_network;
        int server_port;
        std::string wifi_ssid;
        std::string wifi_password;
        
        // Display settings
        bool enable_display;
        bool show_fps;
        bool show_confidence;
        
        // Model paths
        std::string face_model_path;
        std::string face_db_path;
        
        // Debug settings
        bool debug_mode;
        std::string log_file;
    } config_params_;
    
    // Private methods
    void CaptureThreadFunc();
    void ProcessThreadFunc();
    void DisplayThreadFunc();
    
    int InitializeCamera();
    int InitializeFaceEngine();
    int InitializeNetwork();
    
    void ProcessFrame(const cv::Mat& frame);
    void DrawResults(cv::Mat& frame, const std::vector<FaceResult>& results);
    void UpdateStatistics();
    void PrintStatistics() const;
    
    // Error handling
    void HandleError(const std::string& error_msg, int error_code);
    bool CheckSystemResources();
};

FaceRecognitionApp::FaceRecognitionApp()
    : running_(false)
    , new_frame_available_(false)
    , frames_processed_(0)
    , faces_detected_(0)
    , faces_recognized_(0)
{
    // Initialize default configuration
    config_params_.camera_id = 0;
    config_params_.frame_width = 640;
    config_params_.frame_height = 480;
    config_params_.frame_fps = 30;
    
    config_params_.process_width = 320;
    config_params_.process_height = 240;
    config_params_.detection_threshold = 0.7f;
    config_params_.recognition_threshold = 0.8f;
    config_params_.max_faces = 5;
    
    config_params_.enable_network = true;
    config_params_.server_port = 8080;
    config_params_.wifi_ssid = "";
    config_params_.wifi_password = "";
    
    config_params_.enable_display = false; // No display on embedded system
    config_params_.show_fps = true;
    config_params_.show_confidence = true;
    
    config_params_.face_model_path = "/opt/models/";
    config_params_.face_db_path = "/opt/face_db/";
    
    config_params_.debug_mode = false;
    config_params_.log_file = "/var/log/face_app.log";
}

FaceRecognitionApp::~FaceRecognitionApp()
{
    Shutdown();
}

int FaceRecognitionApp::Initialize(const std::string& config_file)
{
    std::cout << "Initializing Face Recognition Application..." << std::endl;
    
    // Load configuration
    if (LoadConfig(config_file) != 0) {
        std::cerr << "Failed to load configuration" << std::endl;
        return -1;
    }
    
    // Check system resources
    if (!CheckSystemResources()) {
        std::cerr << "Insufficient system resources" << std::endl;
        return -1;
    }
    
    // Initialize components
    if (InitializeCamera() != 0) {
        std::cerr << "Failed to initialize camera" << std::endl;
        return -1;
    }
    
    if (InitializeFaceEngine() != 0) {
        std::cerr << "Failed to initialize face engine" << std::endl;
        return -1;
    }
    
    if (config_params_.enable_network && InitializeNetwork() != 0) {
        std::cerr << "Failed to initialize network" << std::endl;
        return -1;
    }
    
    std::cout << "Application initialized successfully" << std::endl;
    PrintConfig();
    
    return 0;
}

int FaceRecognitionApp::Run()
{
    std::cout << "Starting Face Recognition Application..." << std::endl;
    
    running_ = true;
    start_time_ = std::chrono::steady_clock::now();
    
    // Start worker threads
    capture_thread_ = std::thread(&FaceRecognitionApp::CaptureThreadFunc, this);
    process_thread_ = std::thread(&FaceRecognitionApp::ProcessThreadFunc, this);
    
    if (config_params_.enable_display) {
        display_thread_ = std::thread(&FaceRecognitionApp::DisplayThreadFunc, this);
    }
    
    // Main loop - just monitor and print statistics
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        PrintStatistics();
    }
    
    return 0;
}

void FaceRecognitionApp::Shutdown()
{
    std::cout << "Shutting down application..." << std::endl;
    
    running_ = false;
    frame_cv_.notify_all();
    
    // Wait for threads to finish
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    
    if (process_thread_.joinable()) {
        process_thread_.join();
    }
    
    if (display_thread_.joinable()) {
        display_thread_.join();
    }
    
    // Cleanup components
    if (camera_) {
        camera_->Stop();
        camera_.reset();
    }
    
    if (face_engine_) {
        face_engine_.reset();
    }
    
    if (network_) {
        network_.reset();
    }
    
    std::cout << "Application shutdown complete" << std::endl;
}

int FaceRecognitionApp::LoadConfig(const std::string& config_file)
{
    // This would load configuration from JSON file
    // For now, use default values
    std::cout << "Loading configuration from: " << config_file << std::endl;
    
    // TODO: Implement JSON configuration loading
    // config_ = std::make_unique<ConfigManager>(config_file);
    
    return 0;
}

void FaceRecognitionApp::PrintConfig() const
{
    std::cout << "\n=== Application Configuration ===" << std::endl;
    std::cout << "Camera: " << config_params_.camera_id 
              << " (" << config_params_.frame_width << "x" << config_params_.frame_height 
              << "@" << config_params_.frame_fps << "fps)" << std::endl;
    std::cout << "Processing: " << config_params_.process_width << "x" << config_params_.process_height << std::endl;
    std::cout << "Detection threshold: " << config_params_.detection_threshold << std::endl;
    std::cout << "Recognition threshold: " << config_params_.recognition_threshold << std::endl;
    std::cout << "Max faces: " << config_params_.max_faces << std::endl;
    std::cout << "Network enabled: " << (config_params_.enable_network ? "Yes" : "No") << std::endl;
    std::cout << "Model path: " << config_params_.face_model_path << std::endl;
    std::cout << "================================\n" << std::endl;
}

int FaceRecognitionApp::InitializeCamera()
{
    std::cout << "Initializing camera..." << std::endl;
    
    camera_ = std::make_unique<CameraAPI>();
    
    CameraConfig cam_config;
    cam_config.device_id = config_params_.camera_id;
    cam_config.width = config_params_.frame_width;
    cam_config.height = config_params_.frame_height;
    cam_config.fps = config_params_.frame_fps;
    cam_config.format = CAMERA_FORMAT_MJPEG;
    
    int ret = camera_->Initialize(cam_config);
    if (ret != 0) {
        std::cerr << "Camera initialization failed: " << ret << std::endl;
        return ret;
    }
    
    ret = camera_->Start();
    if (ret != 0) {
        std::cerr << "Camera start failed: " << ret << std::endl;
        return ret;
    }
    
    std::cout << "Camera initialized successfully" << std::endl;
    return 0;
}

int FaceRecognitionApp::InitializeFaceEngine()
{
    std::cout << "Initializing face engine..." << std::endl;
    
    face_engine_ = std::make_unique<FaceEngine>();
    
    FaceEngineConfig engine_config;
    engine_config.model_path = config_params_.face_model_path;
    engine_config.detection_threshold = config_params_.detection_threshold;
    engine_config.recognition_threshold = config_params_.recognition_threshold;
    engine_config.max_faces = config_params_.max_faces;
    engine_config.input_width = config_params_.process_width;
    engine_config.input_height = config_params_.process_height;
    
    int ret = face_engine_->Initialize(engine_config);
    if (ret != 0) {
        std::cerr << "Face engine initialization failed: " << ret << std::endl;
        return ret;
    }
    
    // Load face database
    ret = face_engine_->LoadDatabase(config_params_.face_db_path);
    if (ret != 0) {
        std::cout << "Warning: Face database not loaded: " << ret << std::endl;
        // Continue without database - detection only mode
    }
    
    std::cout << "Face engine initialized successfully" << std::endl;
    return 0;
}

int FaceRecognitionApp::InitializeNetwork()
{
    std::cout << "Initializing network..." << std::endl;
    
    network_ = std::make_unique<NetworkManager>();
    
    NetworkConfig net_config;
    net_config.wifi_ssid = config_params_.wifi_ssid;
    net_config.wifi_password = config_params_.wifi_password;
    net_config.server_port = config_params_.server_port;
    net_config.enable_remote_config = true;
    
    int ret = network_->Initialize(net_config);
    if (ret != 0) {
        std::cerr << "Network initialization failed: " << ret << std::endl;
        return ret;
    }
    
    std::cout << "Network initialized successfully" << std::endl;
    return 0;
}

void FaceRecognitionApp::CaptureThreadFunc()
{
    std::cout << "Capture thread started" << std::endl;

    while (running_) {
        CameraFrame frame;
        int ret = camera_->GetFrame(frame);

        if (ret == 0) {
            // Convert to OpenCV Mat
            cv::Mat cv_frame;
            if (frame.format == CAMERA_FORMAT_MJPEG) {
                // Decode MJPEG
                std::vector<uchar> buffer(static_cast<uchar*>(frame.data),
                                        static_cast<uchar*>(frame.data) + frame.size);
                cv_frame = cv::imdecode(buffer, cv::IMREAD_COLOR);
            } else if (frame.format == CAMERA_FORMAT_YUYV) {
                // Convert YUYV to BGR
                cv::Mat yuyv_frame(frame.height, frame.width, CV_8UC2, frame.data);
                cv::cvtColor(yuyv_frame, cv_frame, cv::COLOR_YUV2BGR_YUYV);
            }

            if (!cv_frame.empty()) {
                std::lock_guard<std::mutex> lock(frame_mutex_);
                current_frame_ = cv_frame.clone();
                new_frame_available_ = true;
                frame_cv_.notify_one();
            }

            camera_->ReleaseFrame(frame);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    std::cout << "Capture thread stopped" << std::endl;
}

void FaceRecognitionApp::ProcessThreadFunc()
{
    std::cout << "Process thread started" << std::endl;

    while (running_) {
        cv::Mat frame_to_process;

        // Wait for new frame
        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            frame_cv_.wait(lock, [this] { return new_frame_available_ || !running_; });

            if (!running_) break;

            if (new_frame_available_) {
                frame_to_process = current_frame_.clone();
                new_frame_available_ = false;
            }
        }

        if (!frame_to_process.empty()) {
            ProcessFrame(frame_to_process);
            frames_processed_++;
        }
    }

    std::cout << "Process thread stopped" << std::endl;
}

void FaceRecognitionApp::DisplayThreadFunc()
{
    std::cout << "Display thread started" << std::endl;

    // Note: On embedded system without display, this thread might not be used
    // or could be used for saving debug images

    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30fps

        // Display logic would go here
        // For embedded system, might save frames to file instead
    }

    std::cout << "Display thread stopped" << std::endl;
}

void FaceRecognitionApp::ProcessFrame(const cv::Mat& frame)
{
    // Resize frame for processing to reduce computation
    cv::Mat process_frame;
    cv::resize(frame, process_frame,
               cv::Size(config_params_.process_width, config_params_.process_height));

    // Detect faces
    std::vector<FaceDetection> detections;
    int ret = face_engine_->DetectFaces(process_frame, detections);

    if (ret == 0 && !detections.empty()) {
        faces_detected_ += detections.size();

        // Recognize faces if database is loaded
        std::vector<FaceResult> results;
        ret = face_engine_->RecognizeFaces(process_frame, detections, results);

        if (ret == 0) {
            for (const auto& result : results) {
                if (!result.person_id.empty()) {
                    faces_recognized_++;

                    // Send result via network if enabled
                    if (network_ && config_params_.enable_network) {
                        network_->SendRecognitionResult(result);
                    }

                    std::cout << "Recognized: " << result.person_id
                              << " (confidence: " << result.confidence << ")" << std::endl;
                }
            }
        }
    }
}

void FaceRecognitionApp::PrintStatistics() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);

    std::cout << "\n=== Statistics (Runtime: " << duration.count() << "s) ===" << std::endl;
    std::cout << "Frames processed: " << frames_processed_.load() << std::endl;
    std::cout << "Faces detected: " << faces_detected_.load() << std::endl;
    std::cout << "Faces recognized: " << faces_recognized_.load() << std::endl;

    if (duration.count() > 0) {
        std::cout << "Processing FPS: " << frames_processed_.load() / duration.count() << std::endl;
    }

    std::cout << "==============================\n" << std::endl;
}

bool FaceRecognitionApp::CheckSystemResources()
{
    // Check available memory, CPU, etc.
    // This is a simplified check
    std::cout << "Checking system resources..." << std::endl;

    // TODO: Implement actual resource checking
    // - Check available memory
    // - Check CPU load
    // - Check disk space

    return true;
}

void FaceRecognitionApp::HandleError(const std::string& error_msg, int error_code)
{
    std::cerr << "Error: " << error_msg << " (code: " << error_code << ")" << std::endl;

    // TODO: Implement error recovery strategies
    // - Restart camera
    // - Reset face engine
    // - Send error notification
}

// Main function
int main(int argc, char* argv[])
{
    std::string config_file = "/etc/face_app_config.json";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--config" && i + 1 < argc) {
            config_file = argv[i + 1];
            i++;
        } else if (std::string(argv[i]) == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --config <file>  Configuration file path" << std::endl;
            std::cout << "  --help           Show this help message" << std::endl;
            return 0;
        }
    }
    
    // Create and run application
    FaceRecognitionApp app;
    
    int ret = app.Initialize(config_file);
    if (ret != 0) {
        std::cerr << "Application initialization failed: " << ret << std::endl;
        return ret;
    }
    
    // Setup signal handlers for graceful shutdown
    // TODO: Implement signal handling
    
    ret = app.Run();
    if (ret != 0) {
        std::cerr << "Application run failed: " << ret << std::endl;
        return ret;
    }
    
    return 0;
}

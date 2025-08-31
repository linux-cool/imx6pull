/*
 * Simple Face Detection Demo
 * 
 * A simplified version of the face detection demo for quick testing.
 * This version has minimal dependencies and focuses on core functionality.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <chrono>

class SimpleFaceDetector {
private:
    cv::CascadeClassifier face_cascade_;
    bool initialized_;
    
public:
    SimpleFaceDetector() : initialized_(false) {}
    
    bool initialize() {
        // Try to load Haar cascade from common locations
        std::vector<std::string> cascade_paths = {
            "/usr/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml",
            "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt.xml",
            "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml",
            "/usr/local/share/opencv/haarcascades/haarcascade_frontalface_alt.xml",
            "haarcascade_frontalface_alt.xml"
        };
        
        for (const auto& path : cascade_paths) {
            if (face_cascade_.load(path)) {
                std::cout << "Loaded Haar cascade from: " << path << std::endl;
                initialized_ = true;
                return true;
            }
        }
        
        std::cerr << "Error: Could not load Haar cascade classifier" << std::endl;
        std::cerr << "Please ensure OpenCV is properly installed with cascade files" << std::endl;
        return false;
    }
    
    std::vector<cv::Rect> detectFaces(const cv::Mat& image) {
        if (!initialized_) {
            return {};
        }
        
        cv::Mat gray;
        if (image.channels() == 3) {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = image.clone();
        }
        
        cv::equalizeHist(gray, gray);
        
        std::vector<cv::Rect> faces;
        face_cascade_.detectMultiScale(
            gray,
            faces,
            1.1,    // scale factor
            3,      // min neighbors
            0,      // flags
            cv::Size(30, 30),   // min size
            cv::Size(300, 300)  // max size
        );
        
        return faces;
    }
};

class SimpleCamera {
private:
    cv::VideoCapture cap_;
    bool opened_;
    
public:
    SimpleCamera() : opened_(false) {}
    
    bool open(int camera_id = 0) {
        cap_.open(camera_id);
        if (!cap_.isOpened()) {
            std::cerr << "Error: Cannot open camera " << camera_id << std::endl;
            return false;
        }
        
        // Set some basic properties
        cap_.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        cap_.set(cv::CAP_PROP_FPS, 30);
        
        opened_ = true;
        std::cout << "Camera " << camera_id << " opened successfully" << std::endl;
        
        // Print actual resolution
        int width = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH));
        int height = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT));
        double fps = cap_.get(cv::CAP_PROP_FPS);
        
        std::cout << "Resolution: " << width << "x" << height << std::endl;
        std::cout << "FPS: " << fps << std::endl;
        
        return true;
    }
    
    bool read(cv::Mat& frame) {
        if (!opened_) {
            return false;
        }
        
        return cap_.read(frame);
    }
    
    void release() {
        if (opened_) {
            cap_.release();
            opened_ = false;
        }
    }
    
    ~SimpleCamera() {
        release();
    }
};

void drawDetections(cv::Mat& image, const std::vector<cv::Rect>& faces) {
    for (size_t i = 0; i < faces.size(); ++i) {
        const auto& face = faces[i];
        
        // Choose color based on face index
        cv::Scalar color;
        switch (i % 6) {
            case 0: color = cv::Scalar(0, 255, 0); break;    // Green
            case 1: color = cv::Scalar(0, 0, 255); break;    // Red
            case 2: color = cv::Scalar(255, 0, 0); break;    // Blue
            case 3: color = cv::Scalar(0, 255, 255); break;  // Yellow
            case 4: color = cv::Scalar(255, 0, 255); break;  // Magenta
            case 5: color = cv::Scalar(255, 255, 0); break;  // Cyan
        }
        
        // Draw bounding box
        cv::rectangle(image, face, color, 2);
        
        // Draw face center
        cv::Point center(face.x + face.width/2, face.y + face.height/2);
        cv::circle(image, center, 3, color, -1);
        
        // Draw face number
        std::string face_text = "Face " + std::to_string(i + 1);
        cv::putText(image, face_text, 
                   cv::Point(face.x, face.y - 10),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
    }
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [camera_id]" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  camera_id    Camera device ID (default: 0)" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  ESC or 'q'   Quit the application" << std::endl;
    std::cout << "  's'          Save current frame" << std::endl;
    std::cout << "  'f'          Toggle fullscreen" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Simple Face Detection Demo ===" << std::endl;
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    std::cout << std::endl;
    
    // Parse command line arguments
    int camera_id = 0;
    if (argc > 1) {
        if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        
        try {
            camera_id = std::stoi(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid camera ID: " << argv[1] << std::endl;
            return 1;
        }
    }
    
    // Initialize face detector
    SimpleFaceDetector detector;
    if (!detector.initialize()) {
        return 1;
    }
    
    // Initialize camera
    SimpleCamera camera;
    if (!camera.open(camera_id)) {
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "Starting face detection..." << std::endl;
    std::cout << "Press ESC or 'q' to quit" << std::endl;
    std::cout << "Press 's' to save current frame" << std::endl;
    std::cout << std::endl;
    
    // Main processing loop
    cv::Mat frame;
    int frame_count = 0;
    int total_faces = 0;
    auto start_time = std::chrono::steady_clock::now();
    auto last_fps_time = start_time;
    int frames_since_last_fps = 0;
    double current_fps = 0.0;
    
    const std::string window_name = "Simple Face Detection Demo";
    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
    
    while (true) {
        // Capture frame
        if (!camera.read(frame)) {
            std::cerr << "Error: Failed to read frame from camera" << std::endl;
            break;
        }
        
        if (frame.empty()) {
            std::cerr << "Error: Empty frame received" << std::endl;
            continue;
        }
        
        frame_count++;
        frames_since_last_fps++;
        
        // Detect faces
        auto detect_start = std::chrono::high_resolution_clock::now();
        std::vector<cv::Rect> faces = detector.detectFaces(frame);
        auto detect_end = std::chrono::high_resolution_clock::now();
        
        auto detection_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            detect_end - detect_start);
        
        total_faces += faces.size();
        
        // Draw detections
        drawDetections(frame, faces);
        
        // Calculate and display FPS
        auto now = std::chrono::steady_clock::now();
        auto fps_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_fps_time);
        
        if (fps_duration.count() >= 1000) { // Update FPS every second
            current_fps = frames_since_last_fps * 1000.0 / fps_duration.count();
            frames_since_last_fps = 0;
            last_fps_time = now;
        }
        
        // Draw information overlay
        std::string fps_text = "FPS: " + std::to_string(static_cast<int>(current_fps));
        std::string faces_text = "Faces: " + std::to_string(faces.size());
        std::string time_text = "Detection: " + std::to_string(detection_time.count()) + "ms";
        
        cv::putText(frame, fps_text, cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        cv::putText(frame, faces_text, cv::Point(10, 60), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        cv::putText(frame, time_text, cv::Point(10, 90), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        
        // Display frame
        cv::imshow(window_name, frame);
        
        // Handle key press
        int key = cv::waitKey(1) & 0xFF;
        if (key == 27 || key == 'q') { // ESC or 'q'
            break;
        } else if (key == 's') { // Save frame
            std::string filename = "face_detection_" + 
                                 std::to_string(frame_count) + ".jpg";
            cv::imwrite(filename, frame);
            std::cout << "Saved frame to: " << filename << std::endl;
        } else if (key == 'f') { // Toggle fullscreen
            static bool fullscreen = false;
            fullscreen = !fullscreen;
            if (fullscreen) {
                cv::setWindowProperty(window_name, cv::WND_PROP_FULLSCREEN, 
                                    cv::WINDOW_FULLSCREEN);
            } else {
                cv::setWindowProperty(window_name, cv::WND_PROP_FULLSCREEN, 
                                    cv::WINDOW_AUTOSIZE);
            }
        }
    }
    
    // Print final statistics
    auto end_time = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time);
    
    std::cout << std::endl;
    std::cout << "=== Final Statistics ===" << std::endl;
    std::cout << "Total frames processed: " << frame_count << std::endl;
    std::cout << "Total faces detected: " << total_faces << std::endl;
    std::cout << "Average faces per frame: " << 
                 (frame_count > 0 ? static_cast<double>(total_faces) / frame_count : 0.0) << std::endl;
    std::cout << "Total runtime: " << total_duration.count() << " seconds" << std::endl;
    std::cout << "Average FPS: " << 
                 (total_duration.count() > 0 ? static_cast<double>(frame_count) / total_duration.count() : 0.0) << std::endl;
    std::cout << "========================" << std::endl;
    
    // Cleanup
    cv::destroyAllWindows();
    
    return 0;
}

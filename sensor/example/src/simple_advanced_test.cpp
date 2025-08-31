/*
 * Simple Advanced Demo Test
 * 
 * This program tests the advanced demo initialization step by step
 * to isolate where the failure occurs.
 */

#include <iostream>
#include <memory>
#include "camera_capture.h"
#include "advanced_face_detector.h"

int main() {
    std::cout << "=== Simple Advanced Demo Test ===" << std::endl;
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    std::cout << std::endl;
    
    // Test 1: Create CameraCapture object
    std::cout << "Test 1: Creating CameraCapture object..." << std::endl;
    try {
        std::unique_ptr<CameraCapture> camera = std::make_unique<CameraCapture>();
        std::cout << "✓ CameraCapture object created successfully" << std::endl;
        
        // Test 2: Initialize camera with device path (we know this works)
        std::cout << "\nTest 2: Initialize camera with /dev/video0..." << std::endl;
        if (camera->initialize("/dev/video0")) {
            std::cout << "✓ Camera initialization successful" << std::endl;
            
            // Test 3: Start camera
            std::cout << "\nTest 3: Start camera..." << std::endl;
            if (camera->start()) {
                std::cout << "✓ Camera start successful" << std::endl;
                
                // Test 4: Create AdvancedFaceDetector
                std::cout << "\nTest 4: Creating AdvancedFaceDetector..." << std::endl;
                try {
                    AdvancedFaceDetector detector;
                    std::cout << "✓ AdvancedFaceDetector created successfully" << std::endl;
                    
                    // Test 5: Initialize detector
                    std::cout << "\nTest 5: Initialize detector..." << std::endl;
                    if (detector.initialize()) {
                        std::cout << "✓ Detector initialization successful" << std::endl;
                        
                        // Test 6: Test detection on a frame
                        std::cout << "\nTest 6: Test detection..." << std::endl;
                        cv::Mat frame;
                        if (camera->captureFrame(frame)) {
                            auto detections = detector.detectFaces(frame);
                            std::cout << "✓ Detection successful, found " << detections.size() << " faces" << std::endl;
                        } else {
                            std::cout << "✗ Frame capture failed" << std::endl;
                        }
                        
                    } else {
                        std::cout << "✗ Detector initialization failed: " << detector.getLastError() << std::endl;
                    }
                    
                } catch (const std::exception& e) {
                    std::cout << "✗ Exception creating AdvancedFaceDetector: " << e.what() << std::endl;
                }
                
                camera->stop();
            } else {
                std::cout << "✗ Camera start failed" << std::endl;
            }
        } else {
            std::cout << "✗ Camera initialization failed" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception creating CameraCapture: " << e.what() << std::endl;
        return -1;
    }
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}

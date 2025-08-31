/*
 * Simple Camera Test Program
 * 
 * This program tests camera initialization in isolation to debug
 * the camera access issues in the advanced face detector.
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <memory>
#include "camera_capture.h"

int main() {
    std::cout << "=== Camera Test Program ===" << std::endl;
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    std::cout << std::endl;
    
    // Test 1: Direct OpenCV VideoCapture
    std::cout << "Test 1: Direct OpenCV VideoCapture" << std::endl;
    cv::VideoCapture cap(0);
    if (cap.isOpened()) {
        std::cout << "✓ Direct OpenCV VideoCapture successful" << std::endl;
        cap.release();
    } else {
        std::cout << "✗ Direct OpenCV VideoCapture failed" << std::endl;
    }
    
    // Test 2: CameraCapture class construction
    std::cout << "\nTest 2: CameraCapture class construction" << std::endl;
    try {
        std::unique_ptr<CameraCapture> camera = std::make_unique<CameraCapture>();
        std::cout << "✓ CameraCapture object created successfully" << std::endl;
        
        // Test 3: CameraCapture initialization with ID
        std::cout << "\nTest 3: CameraCapture initialization with ID 0" << std::endl;
        if (camera->initialize(0)) {
            std::cout << "✓ CameraCapture initialization with ID 0 successful" << std::endl;
            
            // Test 4: Start camera
            std::cout << "\nTest 4: Start camera" << std::endl;
            if (camera->start()) {
                std::cout << "✓ Camera start successful" << std::endl;
                
                // Test 5: Capture a frame
                std::cout << "\nTest 5: Capture a frame" << std::endl;
                cv::Mat frame;
                if (camera->captureFrame(frame)) {
                    std::cout << "✓ Frame capture successful" << std::endl;
                    std::cout << "Frame size: " << frame.cols << "x" << frame.rows << std::endl;
                } else {
                    std::cout << "✗ Frame capture failed" << std::endl;
                }
                
                camera->stop();
            } else {
                std::cout << "✗ Camera start failed" << std::endl;
            }
        } else {
            std::cout << "✗ CameraCapture initialization with ID 0 failed" << std::endl;
            
            // Test alternative initialization methods
            std::cout << "\nTest 3b: CameraCapture initialization with device path" << std::endl;
            camera = std::make_unique<CameraCapture>();
            if (camera->initialize("/dev/video0")) {
                std::cout << "✓ CameraCapture initialization with /dev/video0 successful" << std::endl;
            } else {
                std::cout << "✗ CameraCapture initialization with /dev/video0 failed" << std::endl;
                
                // Try other camera IDs
                for (int i = 1; i <= 3; ++i) {
                    std::cout << "\nTest 3c: CameraCapture initialization with ID " << i << std::endl;
                    camera = std::make_unique<CameraCapture>();
                    if (camera->initialize(i)) {
                        std::cout << "✓ CameraCapture initialization with ID " << i << " successful" << std::endl;
                        break;
                    } else {
                        std::cout << "✗ CameraCapture initialization with ID " << i << " failed" << std::endl;
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception during CameraCapture construction: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cout << "✗ Unknown exception during CameraCapture construction" << std::endl;
        return -1;
    }
    
    std::cout << "\n=== Camera Test Complete ===" << std::endl;
    return 0;
}

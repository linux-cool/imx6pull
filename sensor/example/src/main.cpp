/*
 * Face Detection Demo - Main Entry Point
 * 
 * This is the main entry point for the face detection demo application.
 * It handles command line argument parsing and application initialization.
 * 
 * Author: Face Detection Demo Team
 * License: MIT
 */

#include "face_detection_demo.h"
#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <memory>

// Global application instance for signal handling
std::unique_ptr<FaceDetectionDemo> g_app = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    if (g_app) {
        g_app->stop();
    }
}

// Print application banner
void printBanner() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Face Detection Demo v1.0.0" << std::endl;
    std::cout << "    OpenCV-based Real-time Face Detection" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
}

// Print usage information
void printUsage(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "OPTIONS:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -c, --camera ID         Camera ID (default: 0)" << std::endl;
    std::cout << "  -d, --device PATH       Device path (Linux, e.g., /dev/video0)" << std::endl;
    std::cout << "  -w, --width WIDTH       Frame width (default: 640)" << std::endl;
    std::cout << "  -h, --height HEIGHT     Frame height (default: 480)" << std::endl;
    std::cout << "  -f, --fps FPS           Target FPS (default: 30)" << std::endl;
    std::cout << "  -s, --scale FACTOR      Scale factor for detection (default: 1.1)" << std::endl;
    std::cout << "  -n, --neighbors NUM     Min neighbors for detection (default: 3)" << std::endl;
    std::cout << "  -m, --min-size SIZE     Minimum face size (default: 30)" << std::endl;
    std::cout << "  -M, --max-size SIZE     Maximum face size (default: 300)" << std::endl;
    std::cout << "  --no-fps                Don't show FPS counter" << std::endl;
    std::cout << "  --no-info               Don't show detection info" << std::endl;
    std::cout << "  --save-video FILE       Save video to file" << std::endl;
    std::cout << "  --config FILE           Load configuration from file" << std::endl;
    std::cout << "  --verbose               Enable verbose output" << std::endl;
    std::cout << "  --list-cameras          List available cameras and exit" << std::endl;
    std::cout << std::endl;
    std::cout << "EXAMPLES:" << std::endl;
    std::cout << "  " << program_name << "                    # Use default camera" << std::endl;
    std::cout << "  " << program_name << " --camera 1         # Use camera 1" << std::endl;
    std::cout << "  " << program_name << " --device /dev/video1  # Use specific device (Linux)" << std::endl;
    std::cout << "  " << program_name << " --width 1280 --height 720  # HD resolution" << std::endl;
    std::cout << "  " << program_name << " --save-video output.avi     # Save to video file" << std::endl;
    std::cout << "  " << program_name << " --config config.json       # Load from config file" << std::endl;
    std::cout << std::endl;
}

// Parse command line arguments
FaceDetectionConfig parseArguments(int argc, char* argv[]) {
    FaceDetectionConfig config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        }
        else if (arg == "--list-cameras") {
            std::cout << "Available cameras:" << std::endl;
            auto cameras = FaceDetectionUtils::getAvailableCameras();
            if (cameras.empty()) {
                std::cout << "  No cameras found" << std::endl;
            } else {
                for (int cam_id : cameras) {
                    std::cout << "  Camera " << cam_id;
                    if (FaceDetectionUtils::isCameraAvailable(cam_id)) {
                        std::cout << " (available)";
                    } else {
                        std::cout << " (unavailable)";
                    }
                    std::cout << std::endl;
                }
            }
            exit(0);
        }
        else if ((arg == "-c" || arg == "--camera") && i + 1 < argc) {
            config.camera_id = std::stoi(argv[++i]);
        }
        else if ((arg == "-d" || arg == "--device") && i + 1 < argc) {
            config.device_path = argv[++i];
        }
        else if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            config.width = std::stoi(argv[++i]);
        }
        else if ((arg == "--height") && i + 1 < argc) {  // Note: -h conflicts with --help
            config.height = std::stoi(argv[++i]);
        }
        else if ((arg == "-f" || arg == "--fps") && i + 1 < argc) {
            config.fps = std::stoi(argv[++i]);
        }
        else if ((arg == "-s" || arg == "--scale") && i + 1 < argc) {
            config.scale_factor = std::stod(argv[++i]);
        }
        else if ((arg == "-n" || arg == "--neighbors") && i + 1 < argc) {
            config.min_neighbors = std::stoi(argv[++i]);
        }
        else if ((arg == "-m" || arg == "--min-size") && i + 1 < argc) {
            config.min_size = std::stoi(argv[++i]);
        }
        else if ((arg == "-M" || arg == "--max-size") && i + 1 < argc) {
            config.max_size = std::stoi(argv[++i]);
        }
        else if (arg == "--no-fps") {
            config.show_fps = false;
        }
        else if (arg == "--no-info") {
            config.show_detection_info = false;
        }
        else if (arg == "--save-video" && i + 1 < argc) {
            config.save_video = true;
            config.output_filename = argv[++i];
        }
        else if (arg == "--config" && i + 1 < argc) {
            // Config file will be loaded later
            i++; // Skip the filename for now
        }
        else if (arg == "--verbose") {
            config.verbose = true;
        }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage(argv[0]);
            exit(1);
        }
    }
    
    return config;
}

// Load configuration from file if specified
bool loadConfigFile(int argc, char* argv[], FaceDetectionConfig& config) {
    for (int i = 1; i < argc - 1; i++) {
        std::string arg = argv[i];
        if (arg == "--config") {
            std::string config_file = argv[i + 1];
            std::cout << "Loading configuration from: " << config_file << std::endl;
            
            FaceDetectionDemo temp_demo;
            if (temp_demo.loadConfigFromFile(config_file)) {
                config = temp_demo.getConfig();
                return true;
            } else {
                std::cerr << "Failed to load configuration file: " << config_file << std::endl;
                return false;
            }
        }
    }
    return true; // No config file specified, which is OK
}

// Validate configuration
bool validateConfig(const FaceDetectionConfig& config) {
    if (config.width <= 0 || config.height <= 0) {
        std::cerr << "Error: Invalid resolution " << config.width << "x" << config.height << std::endl;
        return false;
    }
    
    if (config.fps <= 0 || config.fps > 120) {
        std::cerr << "Error: Invalid FPS " << config.fps << std::endl;
        return false;
    }
    
    if (config.scale_factor <= 1.0 || config.scale_factor > 2.0) {
        std::cerr << "Error: Invalid scale factor " << config.scale_factor << std::endl;
        return false;
    }
    
    if (config.min_neighbors < 1 || config.min_neighbors > 10) {
        std::cerr << "Error: Invalid min neighbors " << config.min_neighbors << std::endl;
        return false;
    }
    
    if (config.min_size < 10 || config.min_size > 500) {
        std::cerr << "Error: Invalid min size " << config.min_size << std::endl;
        return false;
    }
    
    if (config.max_size < config.min_size || config.max_size > 1000) {
        std::cerr << "Error: Invalid max size " << config.max_size << std::endl;
        return false;
    }
    
    return true;
}

// Print configuration summary
void printConfigSummary(const FaceDetectionConfig& config) {
    std::cout << "Configuration Summary:" << std::endl;
    std::cout << "  Camera ID: " << config.camera_id << std::endl;
    if (!config.device_path.empty()) {
        std::cout << "  Device Path: " << config.device_path << std::endl;
    }
    std::cout << "  Resolution: " << config.width << "x" << config.height << std::endl;
    std::cout << "  Target FPS: " << config.fps << std::endl;
    std::cout << "  Scale Factor: " << config.scale_factor << std::endl;
    std::cout << "  Min Neighbors: " << config.min_neighbors << std::endl;
    std::cout << "  Face Size Range: " << config.min_size << "-" << config.max_size << std::endl;
    std::cout << "  Show FPS: " << (config.show_fps ? "Yes" : "No") << std::endl;
    std::cout << "  Show Info: " << (config.show_detection_info ? "Yes" : "No") << std::endl;
    if (config.save_video) {
        std::cout << "  Save Video: " << config.output_filename << std::endl;
    }
    std::cout << "  Verbose: " << (config.verbose ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
}

// Main function
int main(int argc, char* argv[]) {
    try {
        // Print banner
        printBanner();
        
        // Parse command line arguments
        FaceDetectionConfig config = parseArguments(argc, argv);
        
        // Load configuration file if specified, or try default config
        if (!loadConfigFile(argc, argv, config)) {
            return 1;
        }

        // If no config file was specified via command line, try to load default config
        bool config_loaded = false;
        for (int i = 1; i < argc - 1; i++) {
            if (std::string(argv[i]) == "--config") {
                config_loaded = true;
                break;
            }
        }

        if (!config_loaded) {
            std::cout << "Attempting to load default configuration..." << std::endl;
            FaceDetectionDemo temp_demo;
            if (temp_demo.loadConfigFromFile("config/default_config.json")) {
                config = temp_demo.getConfig();
                std::cout << "Loaded default configuration from config/default_config.json" << std::endl;
            } else {
                std::cout << "No default config file found, using built-in defaults" << std::endl;
            }
        }
        
        // Apply command line overrides (parse again to override config file)
        FaceDetectionConfig cli_config = parseArguments(argc, argv);
        // TODO: Merge cli_config with loaded config properly
        
        // Validate configuration
        if (!validateConfig(config)) {
            return 1;
        }
        
        // Print configuration summary
        if (config.verbose) {
            printConfigSummary(config);
        }
        
        // Print system information
        if (config.verbose) {
            std::cout << "System Information:" << std::endl;
            std::cout << FaceDetectionUtils::getSystemInfo() << std::endl;
        }
        
        // Create and initialize application
        g_app = std::make_unique<FaceDetectionDemo>(config);
        
        // Setup signal handlers for graceful shutdown
        std::signal(SIGINT, signalHandler);   // Ctrl+C
        std::signal(SIGTERM, signalHandler);  // Termination request
#ifndef _WIN32
        std::signal(SIGQUIT, signalHandler);  // Quit signal (Unix only)
#endif
        
        // Initialize the application
        std::cout << "Initializing face detection demo..." << std::endl;
        if (!g_app->initialize()) {
            std::cerr << "Failed to initialize face detection demo" << std::endl;
            return 1;
        }
        
        std::cout << "Initialization successful!" << std::endl;
        std::cout << "Starting face detection... (Press Ctrl+C to stop)" << std::endl;
        std::cout << std::endl;
        
        // Run the application
        int result = g_app->run();
        
        // Print final statistics
        std::cout << std::endl;
        std::cout << "Final Statistics:" << std::endl;
        g_app->printStatistics();
        
        // Cleanup
        g_app.reset();
        
        std::cout << "Face detection demo finished." << std::endl;
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}

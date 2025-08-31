# CMakeLists.txt for Face Detection Demo
cmake_minimum_required(VERSION 3.10)

project(FaceDetectionDemo 
    VERSION 1.0.0
    DESCRIPTION "Face Detection Demo using OpenCV"
    LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
endif()

# Find required packages
find_package(PkgConfig REQUIRED)

# OpenCV
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs objdetect highgui videoio)
if(OpenCV_FOUND)
    message(STATUS "OpenCV version: ${OpenCV_VERSION}")
    message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
else()
    message(FATAL_ERROR "OpenCV not found")
endif()

# Threads
find_package(Threads REQUIRED)

# JSON library (optional, for configuration)
find_path(JSON_INCLUDE_DIR nlohmann/json.hpp
    HINTS /usr/local/include /usr/include)
if(JSON_INCLUDE_DIR)
    message(STATUS "nlohmann/json found at: ${JSON_INCLUDE_DIR}")
    add_definitions(-DHAVE_JSON)
    include_directories(${JSON_INCLUDE_DIR})
else()
    message(WARNING "nlohmann/json not found, configuration file support disabled")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/face_detection_demo.cpp
    src/camera_capture.cpp
    src/face_detector.cpp
    src/performance_monitor.cpp
    src/config_manager.cpp
    src/advanced_face_detector.cpp
)

# Advanced demo source files
set(ADVANCED_DEMO_SOURCES
    src/advanced_demo.cpp
    src/camera_capture.cpp
    src/advanced_face_detector.cpp
    src/performance_monitor.cpp
    src/config_manager.cpp
)

# Header files
set(HEADERS
    include/face_detection_demo.h
    include/camera_capture.h
    include/face_detector.h
    include/performance_monitor.h
    include/config_manager.h
    include/advanced_face_detector.h
)

# Create main executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Create advanced demo executable
add_executable(AdvancedFaceDetectionDemo ${ADVANCED_DEMO_SOURCES} ${HEADERS})

# Create camera test executable
add_executable(CameraTest
    src/camera_test.cpp
    src/camera_capture.cpp
    src/config_manager.cpp
    include/camera_capture.h
    include/config_manager.h
)

# Link libraries for main executable
target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    Threads::Threads
)

# Link libraries for advanced demo
target_link_libraries(AdvancedFaceDetectionDemo
    ${OpenCV_LIBS}
    Threads::Threads
)

# Link libraries for camera test
target_link_libraries(CameraTest
    ${OpenCV_LIBS}
    Threads::Threads
)

# Create simple advanced test executable
add_executable(SimpleAdvancedTest
    src/simple_advanced_test.cpp
    src/camera_capture.cpp
    src/advanced_face_detector.cpp
    src/config_manager.cpp
    include/camera_capture.h
    include/advanced_face_detector.h
    include/config_manager.h
)

# Link libraries for simple advanced test
target_link_libraries(SimpleAdvancedTest
    ${OpenCV_LIBS}
    Threads::Threads
)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    # Linux specific
    target_link_libraries(${PROJECT_NAME} pthread)
elseif(APPLE)
    # macOS specific
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(AVFOUNDATION_LIBRARY AVFoundation)
    target_link_libraries(${PROJECT_NAME} 
        ${COREFOUNDATION_LIBRARY} 
        ${AVFOUNDATION_LIBRARY}
    )
elseif(WIN32)
    # Windows specific
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

# Installation
install(TARGETS ${PROJECT_NAME} AdvancedFaceDetectionDemo
    RUNTIME DESTINATION bin
)

# Install configuration files
install(FILES config/default_config.json
    DESTINATION etc/${PROJECT_NAME}
    RENAME config.json
)

# Install documentation
install(FILES README.md
    DESTINATION share/doc/${PROJECT_NAME}
)

# Create alias for easier building
add_executable(face_detection_demo ALIAS ${PROJECT_NAME})

# Testing (optional)
option(BUILD_TESTS "Build test programs" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "face-detection-demo")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "Face Detection Demo Team")

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libopencv-dev (>= 4.0)")
    set(CPACK_RPM_PACKAGE_REQUIRES "opencv-devel >= 4.0")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
endif()

include(CPack)

# Summary
message(STATUS "")
message(STATUS "=== Build Configuration Summary ===")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
if(JSON_INCLUDE_DIR)
    message(STATUS "JSON support: Enabled")
else()
    message(STATUS "JSON support: Disabled")
endif()
message(STATUS "===================================")
message(STATUS "")

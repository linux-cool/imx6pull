# CMakeLists.txt for IMX6ULL Pro Camera Driver Project
cmake_minimum_required(VERSION 3.10)

project(IMX6ULL_Camera_Project 
    VERSION 1.0.0
    DESCRIPTION "USB Camera Driver and Face Recognition for IMX6ULL Pro"
    LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Platform detection
if(NOT DEFINED TARGET_PLATFORM)
    set(TARGET_PLATFORM "x86_64")
endif()

message(STATUS "Target platform: ${TARGET_PLATFORM}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DDEBUG")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
endif()

# ARM-specific optimizations
if(TARGET_PLATFORM STREQUAL "imx6ull")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a7 -mfpu=neon -mfloat-abi=hard")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a7 -mfpu=neon -mfloat-abi=hard")
    add_definitions(-DTARGET_IMX6ULL)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/api/camera_api
    ${CMAKE_SOURCE_DIR}/api/face_api
    ${CMAKE_SOURCE_DIR}/api/network_api
    ${CMAKE_SOURCE_DIR}/middleware/image_process
    ${CMAKE_SOURCE_DIR}/middleware/face_recognition
    ${CMAKE_SOURCE_DIR}/middleware/network
    ${CMAKE_SOURCE_DIR}/drivers/camera_driver
)

# Find required packages
find_package(PkgConfig REQUIRED)

# OpenCV
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs objdetect)
if(OpenCV_FOUND)
    message(STATUS "OpenCV version: ${OpenCV_VERSION}")
    include_directories(${OpenCV_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "OpenCV not found")
endif()

# Threads
find_package(Threads REQUIRED)

# NCNN (if available)
find_path(NCNN_INCLUDE_DIR ncnn/net.h
    HINTS /usr/local/include /usr/include)
find_library(NCNN_LIBRARY ncnn
    HINTS /usr/local/lib /usr/lib)

if(NCNN_INCLUDE_DIR AND NCNN_LIBRARY)
    message(STATUS "NCNN found: ${NCNN_LIBRARY}")
    set(NCNN_FOUND TRUE)
    include_directories(${NCNN_INCLUDE_DIR})
else()
    message(WARNING "NCNN not found, face recognition will be disabled")
    set(NCNN_FOUND FALSE)
endif()

# JSON library (nlohmann/json or jsoncpp)
find_path(JSON_INCLUDE_DIR nlohmann/json.hpp
    HINTS /usr/local/include /usr/include)
if(JSON_INCLUDE_DIR)
    message(STATUS "nlohmann/json found")
    include_directories(${JSON_INCLUDE_DIR})
    set(JSON_FOUND TRUE)
else()
    pkg_check_modules(JSONCPP jsoncpp)
    if(JSONCPP_FOUND)
        message(STATUS "jsoncpp found")
        include_directories(${JSONCPP_INCLUDE_DIRS})
        set(JSON_FOUND TRUE)
    else()
        message(WARNING "JSON library not found")
        set(JSON_FOUND FALSE)
    endif()
endif()

# Subdirectories
add_subdirectory(api)
add_subdirectory(middleware)
add_subdirectory(applications)

# Tests (optional)
option(BUILD_TESTS "Build test programs" OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Documentation (optional)
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()

# Installation
set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix")

# Install configuration files
install(DIRECTORY configs/
    DESTINATION etc/imx6ull_camera
    FILES_MATCHING PATTERN "*.json" PATTERN "*.conf")

# Install scripts
install(DIRECTORY scripts/
    DESTINATION bin
    FILES_MATCHING PATTERN "*.sh"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE)

# Install documentation
install(FILES README.md
    DESTINATION share/doc/imx6ull_camera)

install(DIRECTORY docs/
    DESTINATION share/doc/imx6ull_camera
    FILES_MATCHING PATTERN "*.md" PATTERN "*.txt")

# Package configuration
set(CPACK_PACKAGE_NAME "imx6ull-camera")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "IMX6ULL Camera Team")
set(CPACK_PACKAGE_CONTACT "team@example.com")

if(TARGET_PLATFORM STREQUAL "imx6ull")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "armv7hl")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
endif()

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libopencv-dev (>= 3.0), libjsoncpp-dev")
set(CPACK_RPM_PACKAGE_REQUIRES "opencv-devel >= 3.0, jsoncpp-devel")

include(CPack)

# Summary
message(STATUS "")
message(STATUS "=== Build Configuration Summary ===")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Platform: ${TARGET_PLATFORM}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "OpenCV: ${OpenCV_VERSION}")
message(STATUS "NCNN: ${NCNN_FOUND}")
message(STATUS "JSON: ${JSON_FOUND}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "===================================")
message(STATUS "")

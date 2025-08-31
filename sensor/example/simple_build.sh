#!/bin/bash

# Simple build script for the basic face detection demo
# This builds just the simple_demo.cpp file for quick testing

set -e

echo "=== Simple Face Detection Demo - Build Script ==="
echo ""

# Check dependencies
echo "Checking dependencies..."

if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "Error: No C++ compiler found (g++ or clang++)"
    exit 1
fi

if ! pkg-config --exists opencv4 && ! pkg-config --exists opencv; then
    echo "Error: OpenCV not found"
    echo "Please install OpenCV development packages:"
    echo "  Ubuntu/Debian: sudo apt install libopencv-dev"
    echo "  CentOS/RHEL:   sudo yum install opencv-devel"
    echo "  Fedora:        sudo dnf install opencv-devel"
    echo "  Arch:          sudo pacman -S opencv"
    exit 1
fi

# Determine OpenCV version and flags
if pkg-config --exists opencv4; then
    OPENCV_FLAGS=$(pkg-config --cflags --libs opencv4)
    OPENCV_VERSION=$(pkg-config --modversion opencv4)
    echo "✓ Found OpenCV 4.x version: $OPENCV_VERSION"
else
    OPENCV_FLAGS=$(pkg-config --cflags --libs opencv)
    OPENCV_VERSION=$(pkg-config --modversion opencv)
    echo "✓ Found OpenCV version: $OPENCV_VERSION"
fi

# Determine compiler
if command -v g++ &> /dev/null; then
    COMPILER="g++"
    echo "✓ Using g++ compiler"
elif command -v clang++ &> /dev/null; then
    COMPILER="clang++"
    echo "✓ Using clang++ compiler"
fi

echo ""

# Build command
BUILD_CMD="$COMPILER -std=c++14 -O2 -Wall -Wextra simple_demo.cpp $OPENCV_FLAGS -o simple_face_detection_demo"

echo "Building simple face detection demo..."
echo "Command: $BUILD_CMD"
echo ""

# Execute build
if $BUILD_CMD; then
    echo "✓ Build successful!"
    echo ""
    echo "Binary created: ./simple_face_detection_demo"
    echo ""
    echo "To run the demo:"
    echo "  ./simple_face_detection_demo"
    echo ""
    echo "To use a specific camera:"
    echo "  ./simple_face_detection_demo 1"
    echo ""
    echo "For help:"
    echo "  ./simple_face_detection_demo --help"
    echo ""
    
    # Check if binary exists and get its size
    if [ -f "simple_face_detection_demo" ]; then
        BINARY_SIZE=$(du -h simple_face_detection_demo | cut -f1)
        echo "Binary size: $BINARY_SIZE"
        
        # Make it executable
        chmod +x simple_face_detection_demo
        echo "Binary is executable"
    fi
    
else
    echo "✗ Build failed!"
    exit 1
fi

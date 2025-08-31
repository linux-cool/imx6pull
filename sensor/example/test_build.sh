#!/bin/bash

# Quick test build script for Face Detection Demo
# This script performs a minimal build to test if everything compiles correctly

set -e

echo "=== Face Detection Demo - Quick Build Test ==="
echo ""

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run from the example directory."
    exit 1
fi

# Check for required dependencies
echo "Checking dependencies..."

# Check CMake
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake not found. Please install CMake."
    exit 1
fi

# Check OpenCV
if pkg-config --exists opencv4; then
    echo "✓ OpenCV 4.x found"
elif pkg-config --exists opencv; then
    echo "✓ OpenCV found"
else
    echo "Warning: OpenCV not found via pkg-config. Build may fail."
fi

# Check compiler
if command -v g++ &> /dev/null; then
    echo "✓ g++ compiler found"
elif command -v clang++ &> /dev/null; then
    echo "✓ clang++ compiler found"
else
    echo "Error: No C++ compiler found"
    exit 1
fi

echo ""

# Create build directory
BUILD_DIR="build_test"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

echo "Creating build directory..."
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring project..."
cmake -DCMAKE_BUILD_TYPE=Release .. || {
    echo "Error: CMake configuration failed"
    exit 1
}

# Build
echo "Building project..."
make -j$(nproc 2>/dev/null || echo 4) || {
    echo "Error: Build failed"
    exit 1
}

# Check if binary was created
if [ -f "FaceDetectionDemo" ]; then
    echo ""
    echo "✓ Build successful!"
    echo "Binary created: $PWD/FaceDetectionDemo"
    echo ""
    echo "To run the demo:"
    echo "  cd $BUILD_DIR"
    echo "  ./FaceDetectionDemo"
    echo ""
    echo "For help:"
    echo "  ./FaceDetectionDemo --help"
    echo ""
    echo "To list available cameras:"
    echo "  ./FaceDetectionDemo --list-cameras"
else
    echo "Error: Binary not found after build"
    exit 1
fi

cd ..
echo "Test build completed successfully!"

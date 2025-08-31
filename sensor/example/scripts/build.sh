#!/bin/bash

# Face Detection Demo Build Script
# 
# This script builds the face detection demo project with proper error handling
# and platform detection.
# 
# Author: Face Detection Demo Team
# License: MIT

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="FaceDetectionDemo"
BUILD_TYPE="Release"
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Command line options
CLEAN_BUILD=false
INSTALL=false
RUN_TESTS=false
VERBOSE=false
DEBUG=false

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to print usage
print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "OPTIONS:"
    echo "  -h, --help          Show this help message"
    echo "  -c, --clean         Clean build directory before building"
    echo "  -d, --debug         Build in debug mode"
    echo "  -i, --install       Install after building"
    echo "  -t, --test          Run tests after building"
    echo "  -v, --verbose       Verbose output"
    echo "  -j, --jobs N        Number of parallel jobs (default: $PARALLEL_JOBS)"
    echo "  --prefix PATH       Installation prefix (default: $INSTALL_PREFIX)"
    echo ""
    echo "EXAMPLES:"
    echo "  $0                  # Basic build"
    echo "  $0 -c -d            # Clean debug build"
    echo "  $0 -c -i            # Clean build and install"
    echo "  $0 -t               # Build and run tests"
    echo ""
}

# Function to detect platform
detect_platform() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]]; then
        echo "windows"
    else
        echo "unknown"
    fi
}

# Function to check dependencies
check_dependencies() {
    print_info "Checking dependencies..."
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake is not installed"
        exit 1
    fi
    
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    print_info "Found CMake version: $CMAKE_VERSION"
    
    # Check compiler
    if command -v g++ &> /dev/null; then
        GCC_VERSION=$(g++ --version | head -n1)
        print_info "Found compiler: $GCC_VERSION"
    elif command -v clang++ &> /dev/null; then
        CLANG_VERSION=$(clang++ --version | head -n1)
        print_info "Found compiler: $CLANG_VERSION"
    else
        print_error "No suitable C++ compiler found"
        exit 1
    fi
    
    # Check OpenCV
    if pkg-config --exists opencv4; then
        OPENCV_VERSION=$(pkg-config --modversion opencv4)
        print_info "Found OpenCV version: $OPENCV_VERSION"
    elif pkg-config --exists opencv; then
        OPENCV_VERSION=$(pkg-config --modversion opencv)
        print_info "Found OpenCV version: $OPENCV_VERSION"
    else
        print_warning "OpenCV not found via pkg-config, CMake will try to find it"
    fi
    
    print_success "Dependency check completed"
}

# Function to setup build directory
setup_build_dir() {
    if [ "$CLEAN_BUILD" = true ] && [ -d "$BUILD_DIR" ]; then
        print_info "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    if [ ! -d "$BUILD_DIR" ]; then
        print_info "Creating build directory..."
        mkdir -p "$BUILD_DIR"
    fi
}

# Function to configure project
configure_project() {
    print_info "Configuring project..."
    
    cd "$BUILD_DIR"
    
    CMAKE_ARGS=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
    )
    
    if [ "$RUN_TESTS" = true ]; then
        CMAKE_ARGS+=("-DBUILD_TESTS=ON")
    fi
    
    if [ "$VERBOSE" = true ]; then
        CMAKE_ARGS+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
    fi
    
    # Platform-specific settings
    PLATFORM=$(detect_platform)
    case $PLATFORM in
        "linux")
            print_info "Configuring for Linux"
            ;;
        "macos")
            print_info "Configuring for macOS"
            CMAKE_ARGS+=("-DCMAKE_OSX_DEPLOYMENT_TARGET=10.14")
            ;;
        "windows")
            print_info "Configuring for Windows"
            CMAKE_ARGS+=("-G" "MinGW Makefiles")
            ;;
        *)
            print_warning "Unknown platform, using default settings"
            ;;
    esac
    
    print_info "Running: cmake ${CMAKE_ARGS[*]} .."
    cmake "${CMAKE_ARGS[@]}" ..
    
    cd ..
    print_success "Project configured successfully"
}

# Function to build project
build_project() {
    print_info "Building project..."
    
    cd "$BUILD_DIR"
    
    BUILD_ARGS=("--build" "." "--config" "$BUILD_TYPE")
    
    if [ "$PARALLEL_JOBS" -gt 1 ]; then
        BUILD_ARGS+=("--parallel" "$PARALLEL_JOBS")
    fi
    
    if [ "$VERBOSE" = true ]; then
        BUILD_ARGS+=("--verbose")
    fi
    
    print_info "Running: cmake ${BUILD_ARGS[*]}"
    cmake "${BUILD_ARGS[@]}"
    
    cd ..
    print_success "Project built successfully"
}

# Function to run tests
run_tests() {
    if [ "$RUN_TESTS" = true ]; then
        print_info "Running tests..."
        
        cd "$BUILD_DIR"
        
        if [ -f "CTestTestfile.cmake" ]; then
            ctest --output-on-failure
            print_success "Tests completed"
        else
            print_warning "No tests found"
        fi
        
        cd ..
    fi
}

# Function to install project
install_project() {
    if [ "$INSTALL" = true ]; then
        print_info "Installing project..."
        
        cd "$BUILD_DIR"
        
        if [ "$EUID" -eq 0 ] || [[ "$INSTALL_PREFIX" == "$HOME"* ]]; then
            cmake --install .
        else
            print_info "Installing with sudo..."
            sudo cmake --install .
        fi
        
        cd ..
        print_success "Project installed successfully"
    fi
}

# Function to print build summary
print_summary() {
    print_info "Build Summary:"
    echo "  Project: $PROJECT_NAME"
    echo "  Build Type: $BUILD_TYPE"
    echo "  Platform: $(detect_platform)"
    echo "  Parallel Jobs: $PARALLEL_JOBS"
    echo "  Install Prefix: $INSTALL_PREFIX"
    
    if [ -f "$BUILD_DIR/$PROJECT_NAME" ]; then
        BINARY_SIZE=$(du -h "$BUILD_DIR/$PROJECT_NAME" | cut -f1)
        echo "  Binary Size: $BINARY_SIZE"
    fi
    
    if [ "$INSTALL" = true ]; then
        echo "  Installed: Yes"
    else
        echo "  Installed: No"
    fi
    
    print_success "Build completed successfully!"
    echo ""
    echo "To run the demo:"
    if [ "$INSTALL" = true ]; then
        echo "  $PROJECT_NAME"
    else
        echo "  ./$BUILD_DIR/$PROJECT_NAME"
    fi
    echo ""
    echo "For help:"
    if [ "$INSTALL" = true ]; then
        echo "  $PROJECT_NAME --help"
    else
        echo "  ./$BUILD_DIR/$PROJECT_NAME --help"
    fi
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            exit 0
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            DEBUG=true
            shift
            ;;
        -i|--install)
            INSTALL=true
            shift
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        --prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        *)
            print_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Main execution
main() {
    print_info "Starting build process for $PROJECT_NAME"
    print_info "Build type: $BUILD_TYPE"
    print_info "Platform: $(detect_platform)"
    echo ""
    
    # Check if we're in the right directory
    if [ ! -f "CMakeLists.txt" ]; then
        print_error "CMakeLists.txt not found. Please run this script from the project root."
        exit 1
    fi
    
    # Execute build steps
    check_dependencies
    setup_build_dir
    configure_project
    build_project
    run_tests
    install_project
    
    echo ""
    print_summary
}

# Run main function
main "$@"

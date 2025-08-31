#!/bin/bash

# OpenCV Data Setup Script
# 
# This script sets up the required OpenCV data files (Haar cascades, etc.)
# for the face detection demo.
# 
# Author: Face Detection Demo Team
# License: MIT

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# Function to find OpenCV data directory
find_opencv_data_dir() {
    local opencv_dirs=(
        "/usr/share/opencv4"
        "/usr/share/opencv"
        "/usr/local/share/opencv4"
        "/usr/local/share/opencv"
        "/opt/opencv/share/opencv4"
        "/opt/opencv/share/opencv"
    )
    
    for dir in "${opencv_dirs[@]}"; do
        if [ -d "$dir/haarcascades" ]; then
            echo "$dir"
            return 0
        fi
    done
    
    return 1
}

# Function to setup Haar cascades
setup_haar_cascades() {
    print_info "Setting up Haar cascade files..."
    
    local opencv_data_dir
    if opencv_data_dir=$(find_opencv_data_dir); then
        print_info "Found OpenCV data directory: $opencv_data_dir"
        
        local cascade_dir="$opencv_data_dir/haarcascades"
        local required_cascades=(
            "haarcascade_frontalface_alt.xml"
            "haarcascade_frontalface_default.xml"
            "haarcascade_frontalface_alt2.xml"
            "haarcascade_eye.xml"
            "haarcascade_smile.xml"
        )
        
        # Create data directory if it doesn't exist
        mkdir -p data/haarcascades
        
        for cascade in "${required_cascades[@]}"; do
            local src_file="$cascade_dir/$cascade"
            local dst_file="data/haarcascades/$cascade"
            
            if [ -f "$src_file" ]; then
                if [ ! -f "$dst_file" ] || [ "$src_file" -nt "$dst_file" ]; then
                    cp "$src_file" "$dst_file"
                    print_success "Copied $cascade"
                else
                    print_info "$cascade already up to date"
                fi
            else
                print_warning "$cascade not found in $cascade_dir"
            fi
        done
        
        # Create symlinks in project root for backward compatibility
        for cascade in "${required_cascades[@]}"; do
            local src_file="data/haarcascades/$cascade"
            if [ -f "$src_file" ]; then
                ln -sf "$src_file" .
                print_info "Created symlink for $cascade"
            fi
        done
        
        # Create symlinks in build directories
        for build_dir in build build_test; do
            if [ -d "$build_dir" ]; then
                for cascade in "${required_cascades[@]}"; do
                    local src_file="../data/haarcascades/$cascade"
                    if [ -f "data/haarcascades/$cascade" ]; then
                        ln -sf "$src_file" "$build_dir/"
                        print_info "Created symlink in $build_dir for $cascade"
                    fi
                done
            fi
        done
        
    else
        print_error "OpenCV data directory not found"
        print_error "Please install OpenCV development packages:"
        print_error "  Ubuntu/Debian: sudo apt install libopencv-dev"
        print_error "  CentOS/RHEL:   sudo yum install opencv-devel"
        print_error "  Fedora:        sudo dnf install opencv-devel"
        return 1
    fi
}

# Function to setup DNN models (optional)
setup_dnn_models() {
    print_info "Setting up DNN models..."
    
    mkdir -p data/dnn
    
    # URLs for common face detection models
    local models=(
        "https://github.com/opencv/opencv_3rdparty/raw/dnn_samples_face_detector_20170830/opencv_face_detector.pbtxt:opencv_face_detector.pbtxt"
        "https://github.com/opencv/opencv_3rdparty/raw/dnn_samples_face_detector_20170830/opencv_face_detector_uint8.pb:opencv_face_detector_uint8.pb"
    )
    
    for model_info in "${models[@]}"; do
        local url="${model_info%:*}"
        local filename="${model_info#*:}"
        local dst_file="data/dnn/$filename"
        
        if [ ! -f "$dst_file" ]; then
            print_info "Downloading $filename..."
            if command -v wget &> /dev/null; then
                wget -q "$url" -O "$dst_file" || {
                    print_warning "Failed to download $filename"
                    rm -f "$dst_file"
                }
            elif command -v curl &> /dev/null; then
                curl -s "$url" -o "$dst_file" || {
                    print_warning "Failed to download $filename"
                    rm -f "$dst_file"
                }
            else
                print_warning "Neither wget nor curl available, skipping DNN model download"
                break
            fi
            
            if [ -f "$dst_file" ]; then
                print_success "Downloaded $filename"
            fi
        else
            print_info "$filename already exists"
        fi
    done
}

# Function to verify setup
verify_setup() {
    print_info "Verifying setup..."
    
    local required_files=(
        "haarcascade_frontalface_alt.xml"
        "haarcascade_frontalface_default.xml"
    )
    
    local all_good=true
    
    for file in "${required_files[@]}"; do
        if [ -f "$file" ]; then
            print_success "$file found"
        else
            print_error "$file not found"
            all_good=false
        fi
    done
    
    if [ "$all_good" = true ]; then
        print_success "All required files are available"
        return 0
    else
        print_error "Some required files are missing"
        return 1
    fi
}

# Function to print usage
print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "This script sets up OpenCV data files for the face detection demo."
    echo ""
    echo "OPTIONS:"
    echo "  -h, --help          Show this help message"
    echo "  --no-dnn            Skip DNN model download"
    echo "  --verify-only       Only verify existing setup"
    echo ""
}

# Parse command line arguments
DOWNLOAD_DNN=true
VERIFY_ONLY=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            exit 0
            ;;
        --no-dnn)
            DOWNLOAD_DNN=false
            shift
            ;;
        --verify-only)
            VERIFY_ONLY=true
            shift
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
    print_info "OpenCV Data Setup for Face Detection Demo"
    echo ""
    
    # Check if we're in the right directory
    if [ ! -f "CMakeLists.txt" ]; then
        print_error "CMakeLists.txt not found. Please run this script from the project root."
        exit 1
    fi
    
    if [ "$VERIFY_ONLY" = true ]; then
        verify_setup
        exit $?
    fi
    
    # Setup Haar cascades
    setup_haar_cascades || exit 1
    
    # Setup DNN models if requested
    if [ "$DOWNLOAD_DNN" = true ]; then
        setup_dnn_models
    fi
    
    # Verify setup
    echo ""
    verify_setup
    
    echo ""
    print_success "OpenCV data setup completed!"
    echo ""
    echo "You can now run the face detection demo:"
    echo "  ./simple_face_detection_demo"
    echo "  ./build_test/FaceDetectionDemo"
    echo ""
}

# Run main function
main "$@"

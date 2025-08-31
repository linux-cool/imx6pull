#!/bin/bash

# Face Detection Demo Dependencies Installation Script
# 
# This script installs the required dependencies for the face detection demo
# on different Linux distributions.
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

# Function to detect Linux distribution
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo $ID
    elif type lsb_release >/dev/null 2>&1; then
        lsb_release -si | tr '[:upper:]' '[:lower:]'
    elif [ -f /etc/lsb-release ]; then
        . /etc/lsb-release
        echo $DISTRIB_ID | tr '[:upper:]' '[:lower:]'
    elif [ -f /etc/debian_version ]; then
        echo "debian"
    elif [ -f /etc/fedora-release ]; then
        echo "fedora"
    elif [ -f /etc/redhat-release ]; then
        echo "rhel"
    else
        echo "unknown"
    fi
}

# Function to install dependencies on Ubuntu/Debian
install_ubuntu_debian() {
    print_info "Installing dependencies for Ubuntu/Debian..."
    
    # Update package list
    sudo apt update
    
    # Install build tools
    sudo apt install -y \
        build-essential \
        cmake \
        git \
        pkg-config
    
    # Install OpenCV dependencies
    sudo apt install -y \
        libopencv-dev \
        libopencv-contrib-dev \
        python3-opencv
    
    # Install additional libraries
    sudo apt install -y \
        libjpeg-dev \
        libpng-dev \
        libtiff-dev \
        libavcodec-dev \
        libavformat-dev \
        libswscale-dev \
        libv4l-dev \
        libxvidcore-dev \
        libx264-dev \
        libgtk-3-dev \
        libatlas-base-dev \
        gfortran
    
    # Install optional JSON library
    sudo apt install -y nlohmann-json3-dev || print_warning "nlohmann-json3-dev not available, JSON support will be disabled"
    
    print_success "Dependencies installed successfully on Ubuntu/Debian"
}

# Function to install dependencies on CentOS/RHEL/Fedora
install_redhat() {
    print_info "Installing dependencies for CentOS/RHEL/Fedora..."
    
    # Detect package manager
    if command -v dnf &> /dev/null; then
        PKG_MGR="dnf"
    elif command -v yum &> /dev/null; then
        PKG_MGR="yum"
    else
        print_error "No suitable package manager found"
        exit 1
    fi
    
    # Install EPEL repository for CentOS/RHEL
    if [[ "$DISTRO" == "centos" ]] || [[ "$DISTRO" == "rhel" ]]; then
        sudo $PKG_MGR install -y epel-release
    fi
    
    # Install build tools
    sudo $PKG_MGR groupinstall -y "Development Tools"
    sudo $PKG_MGR install -y \
        cmake \
        git \
        pkgconfig
    
    # Install OpenCV
    sudo $PKG_MGR install -y \
        opencv-devel \
        opencv-contrib-devel
    
    # Install additional libraries
    sudo $PKG_MGR install -y \
        libjpeg-turbo-devel \
        libpng-devel \
        libtiff-devel \
        ffmpeg-devel \
        libv4l-devel \
        gtk3-devel \
        atlas-devel
    
    print_success "Dependencies installed successfully on CentOS/RHEL/Fedora"
}

# Function to install dependencies on Arch Linux
install_arch() {
    print_info "Installing dependencies for Arch Linux..."
    
    # Update package database
    sudo pacman -Sy
    
    # Install dependencies
    sudo pacman -S --needed \
        base-devel \
        cmake \
        git \
        pkgconf \
        opencv \
        opencv-samples \
        hdf5 \
        vtk \
        glew \
        qt5-base \
        nlohmann-json
    
    print_success "Dependencies installed successfully on Arch Linux"
}

# Function to install dependencies on openSUSE
install_opensuse() {
    print_info "Installing dependencies for openSUSE..."
    
    # Install dependencies
    sudo zypper install -y \
        gcc-c++ \
        cmake \
        git \
        pkg-config \
        opencv-devel \
        libjpeg8-devel \
        libpng16-devel \
        libtiff-devel \
        libv4l-devel \
        gtk3-devel
    
    print_success "Dependencies installed successfully on openSUSE"
}

# Function to verify installation
verify_installation() {
    print_info "Verifying installation..."
    
    # Check CMake
    if command -v cmake &> /dev/null; then
        CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
        print_success "CMake found: $CMAKE_VERSION"
    else
        print_error "CMake not found"
        return 1
    fi
    
    # Check compiler
    if command -v g++ &> /dev/null; then
        GCC_VERSION=$(g++ --version | head -n1)
        print_success "Compiler found: $GCC_VERSION"
    else
        print_error "C++ compiler not found"
        return 1
    fi
    
    # Check OpenCV
    if pkg-config --exists opencv4; then
        OPENCV_VERSION=$(pkg-config --modversion opencv4)
        print_success "OpenCV found: $OPENCV_VERSION"
    elif pkg-config --exists opencv; then
        OPENCV_VERSION=$(pkg-config --modversion opencv)
        print_success "OpenCV found: $OPENCV_VERSION"
    else
        print_warning "OpenCV not found via pkg-config"
        # Try to find OpenCV headers
        if [ -f "/usr/include/opencv4/opencv2/opencv.hpp" ] || [ -f "/usr/include/opencv2/opencv.hpp" ]; then
            print_success "OpenCV headers found"
        else
            print_error "OpenCV not found"
            return 1
        fi
    fi
    
    print_success "All dependencies verified successfully"
    return 0
}

# Function to print post-installation instructions
print_instructions() {
    echo ""
    print_info "Installation completed successfully!"
    echo ""
    echo "Next steps:"
    echo "1. Navigate to the project directory:"
    echo "   cd example"
    echo ""
    echo "2. Build the project:"
    echo "   ./scripts/build.sh"
    echo ""
    echo "3. Run the demo:"
    echo "   ./build/FaceDetectionDemo"
    echo ""
    echo "For more options:"
    echo "   ./build/FaceDetectionDemo --help"
    echo ""
    echo "To install system-wide:"
    echo "   ./scripts/build.sh --install"
    echo ""
}

# Function to print usage
print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "This script installs dependencies for the Face Detection Demo."
    echo ""
    echo "OPTIONS:"
    echo "  -h, --help          Show this help message"
    echo "  -y, --yes           Assume yes for all prompts"
    echo "  --verify-only       Only verify existing installation"
    echo ""
    echo "Supported distributions:"
    echo "  - Ubuntu/Debian"
    echo "  - CentOS/RHEL/Fedora"
    echo "  - Arch Linux"
    echo "  - openSUSE"
    echo ""
}

# Parse command line arguments
ASSUME_YES=false
VERIFY_ONLY=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            exit 0
            ;;
        -y|--yes)
            ASSUME_YES=true
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
    print_info "Face Detection Demo Dependencies Installer"
    echo ""
    
    # Detect distribution
    DISTRO=$(detect_distro)
    print_info "Detected distribution: $DISTRO"
    
    # If verify only, just check and exit
    if [ "$VERIFY_ONLY" = true ]; then
        verify_installation
        exit $?
    fi
    
    # Ask for confirmation unless -y flag is used
    if [ "$ASSUME_YES" = false ]; then
        echo ""
        read -p "Do you want to install dependencies for $DISTRO? (y/N): " -n 1 -r
        echo ""
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            print_info "Installation cancelled"
            exit 0
        fi
    fi
    
    # Install dependencies based on distribution
    case $DISTRO in
        ubuntu|debian)
            install_ubuntu_debian
            ;;
        centos|rhel|fedora)
            install_redhat
            ;;
        arch|manjaro)
            install_arch
            ;;
        opensuse|suse)
            install_opensuse
            ;;
        *)
            print_error "Unsupported distribution: $DISTRO"
            echo ""
            echo "Please install the following dependencies manually:"
            echo "- CMake (>= 3.10)"
            echo "- OpenCV (>= 4.0) with development headers"
            echo "- C++ compiler with C++14 support"
            echo "- pkg-config"
            echo "- Build tools (make, etc.)"
            exit 1
            ;;
    esac
    
    # Verify installation
    echo ""
    if verify_installation; then
        print_instructions
    else
        print_error "Installation verification failed"
        exit 1
    fi
}

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    print_warning "Running as root. This is not recommended."
    print_warning "Please run as a regular user with sudo privileges."
    echo ""
fi

# Run main function
main "$@"

#!/bin/bash

# Build script for IMX6ULL Pro Camera Driver Project
# 
# This script automates the build process for both native and cross-compilation
# Supports different build configurations and target platforms
#
# Author: Build Team
# License: MIT

set -e  # Exit on any error

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_DIR="$PROJECT_ROOT/install"

# Default configuration
BUILD_TYPE="Release"
TARGET_PLATFORM="x86_64"
CLEAN_BUILD=false
VERBOSE=false
INSTALL=false
PACKAGE=false
RUN_TESTS=false

# Cross-compilation settings
CROSS_COMPILE=""
TOOLCHAIN_FILE=""
KERNEL_DIR=""

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Help function
show_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Build script for IMX6ULL Pro Camera Driver Project

OPTIONS:
    -h, --help              Show this help message
    -t, --target PLATFORM   Target platform (x86_64, imx6ull) [default: x86_64]
    -b, --build-type TYPE   Build type (Debug, Release) [default: Release]
    -c, --clean             Clean build directory before building
    -v, --verbose           Enable verbose output
    -i, --install           Install after building
    -p, --package           Create package after building
    -T, --test              Run tests after building
    --cross-compile PREFIX  Cross-compilation prefix (e.g., arm-linux-gnueabihf-)
    --toolchain FILE        CMake toolchain file
    --kernel-dir DIR        Kernel build directory (for driver compilation)

EXAMPLES:
    # Native build for development
    $0 --build-type Debug --verbose --test

    # Cross-compile for IMX6ULL
    $0 --target imx6ull --cross-compile arm-linux-gnueabihf- \\
       --toolchain cmake/arm-linux-gnueabihf.cmake

    # Clean release build with packaging
    $0 --clean --build-type Release --package

    # Build kernel driver only
    $0 --target imx6ull --kernel-dir /path/to/kernel/build

EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -t|--target)
                TARGET_PLATFORM="$2"
                shift 2
                ;;
            -b|--build-type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -i|--install)
                INSTALL=true
                shift
                ;;
            -p|--package)
                PACKAGE=true
                shift
                ;;
            -T|--test)
                RUN_TESTS=true
                shift
                ;;
            --cross-compile)
                CROSS_COMPILE="$2"
                shift 2
                ;;
            --toolchain)
                TOOLCHAIN_FILE="$2"
                shift 2
                ;;
            --kernel-dir)
                KERNEL_DIR="$2"
                shift 2
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# Validate configuration
validate_config() {
    log_info "Validating build configuration..."
    
    # Check target platform
    case $TARGET_PLATFORM in
        x86_64|imx6ull)
            ;;
        *)
            log_error "Unsupported target platform: $TARGET_PLATFORM"
            exit 1
            ;;
    esac
    
    # Check build type
    case $BUILD_TYPE in
        Debug|Release|RelWithDebInfo|MinSizeRel)
            ;;
        *)
            log_error "Invalid build type: $BUILD_TYPE"
            exit 1
            ;;
    esac
    
    # Check cross-compilation setup
    if [[ "$TARGET_PLATFORM" == "imx6ull" ]]; then
        if [[ -z "$CROSS_COMPILE" ]]; then
            log_warn "Cross-compilation prefix not specified, using default"
            CROSS_COMPILE="arm-linux-gnueabihf-"
        fi
        
        if [[ -z "$TOOLCHAIN_FILE" ]]; then
            TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/arm-linux-gnueabihf.cmake"
            log_info "Using default toolchain file: $TOOLCHAIN_FILE"
        fi
        
        if [[ ! -f "$TOOLCHAIN_FILE" ]]; then
            log_error "Toolchain file not found: $TOOLCHAIN_FILE"
            exit 1
        fi
    fi
    
    # Check dependencies
    check_dependencies
}

# Check build dependencies
check_dependencies() {
    log_info "Checking build dependencies..."
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake is required but not installed"
        exit 1
    fi
    
    # Check make
    if ! command -v make &> /dev/null; then
        log_error "Make is required but not installed"
        exit 1
    fi
    
    # Check cross-compiler
    if [[ "$TARGET_PLATFORM" == "imx6ull" ]]; then
        if ! command -v "${CROSS_COMPILE}gcc" &> /dev/null; then
            log_error "Cross-compiler not found: ${CROSS_COMPILE}gcc"
            exit 1
        fi
        log_info "Cross-compiler found: $(${CROSS_COMPILE}gcc --version | head -n1)"
    fi
    
    # Check OpenCV
    if [[ "$TARGET_PLATFORM" == "x86_64" ]]; then
        if ! pkg-config --exists opencv4 && ! pkg-config --exists opencv; then
            log_warn "OpenCV not found via pkg-config"
        fi
    fi
}

# Setup build environment
setup_environment() {
    log_info "Setting up build environment..."
    
    # Create build directory
    if [[ "$CLEAN_BUILD" == true ]] && [[ -d "$BUILD_DIR" ]]; then
        log_info "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    mkdir -p "$INSTALL_DIR"
    
    # Set environment variables for cross-compilation
    if [[ "$TARGET_PLATFORM" == "imx6ull" ]]; then
        export CROSS_COMPILE="$CROSS_COMPILE"
        export CC="${CROSS_COMPILE}gcc"
        export CXX="${CROSS_COMPILE}g++"
        export AR="${CROSS_COMPILE}ar"
        export STRIP="${CROSS_COMPILE}strip"
        export PKG_CONFIG_PATH="/usr/arm-linux-gnueabihf/lib/pkgconfig"
    fi
}

# Configure CMake
configure_cmake() {
    log_info "Configuring CMake..."
    
    cd "$BUILD_DIR"
    
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR"
        "-DTARGET_PLATFORM=$TARGET_PLATFORM"
    )
    
    # Add toolchain file for cross-compilation
    if [[ -n "$TOOLCHAIN_FILE" ]]; then
        cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE")
    fi
    
    # Add verbose output if requested
    if [[ "$VERBOSE" == true ]]; then
        cmake_args+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
    fi
    
    # Add test building if requested
    if [[ "$RUN_TESTS" == true ]]; then
        cmake_args+=("-DBUILD_TESTS=ON")
    fi
    
    log_info "CMake command: cmake ${cmake_args[*]} $PROJECT_ROOT"
    cmake "${cmake_args[@]}" "$PROJECT_ROOT"
}

# Build project
build_project() {
    log_info "Building project..."
    
    cd "$BUILD_DIR"
    
    local make_args=()
    
    # Add parallel jobs
    local num_jobs=$(nproc 2>/dev/null || echo 4)
    make_args+=("-j$num_jobs")
    
    # Add verbose output if requested
    if [[ "$VERBOSE" == true ]]; then
        make_args+=("VERBOSE=1")
    fi
    
    log_info "Make command: make ${make_args[*]}"
    make "${make_args[@]}"
}

# Build kernel driver
build_driver() {
    if [[ -z "$KERNEL_DIR" ]]; then
        log_info "Kernel directory not specified, skipping driver build"
        return 0
    fi
    
    log_info "Building kernel driver..."
    
    cd "$PROJECT_ROOT/drivers/camera_driver"
    
    local make_args=(
        "KERNEL_DIR=$KERNEL_DIR"
        "ARCH=arm"
        "CROSS_COMPILE=$CROSS_COMPILE"
    )
    
    log_info "Driver make command: make ${make_args[*]}"
    make "${make_args[@]}"
    
    # Copy driver to build directory
    mkdir -p "$BUILD_DIR/drivers"
    cp *.ko "$BUILD_DIR/drivers/" 2>/dev/null || true
}

# Run tests
run_tests() {
    if [[ "$RUN_TESTS" != true ]]; then
        return 0
    fi
    
    log_info "Running tests..."
    
    cd "$BUILD_DIR"
    
    if [[ "$TARGET_PLATFORM" == "x86_64" ]]; then
        # Run tests natively
        make test
    else
        log_warn "Cross-compiled tests cannot be run on host system"
        log_info "Tests built successfully, run them on target device"
    fi
}

# Install project
install_project() {
    if [[ "$INSTALL" != true ]]; then
        return 0
    fi
    
    log_info "Installing project..."
    
    cd "$BUILD_DIR"
    make install
    
    # Install driver if built
    if [[ -d "$BUILD_DIR/drivers" ]]; then
        mkdir -p "$INSTALL_DIR/lib/modules"
        cp "$BUILD_DIR/drivers"/*.ko "$INSTALL_DIR/lib/modules/" 2>/dev/null || true
    fi
}

# Create package
create_package() {
    if [[ "$PACKAGE" != true ]]; then
        return 0
    fi
    
    log_info "Creating package..."
    
    cd "$BUILD_DIR"
    make package
    
    # Move packages to project root
    mv *.deb *.rpm *.tar.gz "$PROJECT_ROOT/" 2>/dev/null || true
}

# Print build summary
print_summary() {
    log_success "Build completed successfully!"
    
    echo
    echo "=== Build Summary ==="
    echo "Target Platform: $TARGET_PLATFORM"
    echo "Build Type: $BUILD_TYPE"
    echo "Build Directory: $BUILD_DIR"
    
    if [[ "$INSTALL" == true ]]; then
        echo "Install Directory: $INSTALL_DIR"
    fi
    
    if [[ -n "$CROSS_COMPILE" ]]; then
        echo "Cross Compiler: $CROSS_COMPILE"
    fi
    
    echo
    echo "Built artifacts:"
    find "$BUILD_DIR" -name "*.so" -o -name "*.a" -o -name "face_*" -o -name "*.ko" | head -10
    
    if [[ "$PACKAGE" == true ]]; then
        echo
        echo "Packages created:"
        ls -la "$PROJECT_ROOT"/*.{deb,rpm,tar.gz} 2>/dev/null || echo "No packages found"
    fi
}

# Main function
main() {
    log_info "Starting build process..."
    
    parse_args "$@"
    validate_config
    setup_environment
    configure_cmake
    build_project
    build_driver
    run_tests
    install_project
    create_package
    print_summary
}

# Run main function
main "$@"

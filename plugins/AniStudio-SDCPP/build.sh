#!/bin/bash

# ===== CRITICAL: PURGE ALL WINDOWS PATHS FROM ENVIRONMENT =====
unset MSYSTEM
unset MSYSCON
unset MINGW_PREFIX
unset MINGW_CHOST
unset MINGW_PACKAGE_PREFIX
unset MSYS2_PATH
unset MSYS2_ENV_CONV_EXCL
unset WSLENV
unset WSL_INTEROP
unset WSL_DISTRO_NAME
unset WSL_UTF8

# Completely clear all include paths
export CPATH=""
export CPLUS_INCLUDE_PATH=""
export C_INCLUDE_PATH=""
export INCLUDE=""
export DEPENDENCIES_INCLUDE=""
export SDKROOT=""

# Clear compiler flags
export CFLAGS=""
export CXXFLAGS=""
export CPPFLAGS=""
export LDFLAGS=""

# Remove ANY path containing Windows/MSYS2 from PATH
export PATH=$(echo "$PATH" | tr ':' '\n' | grep -v -E '(msys64|mingw|ucrt64|windows|/mnt/[c-z])' | tr '\n' ':' | sed 's/:$//')

# Set explicit compiler paths
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++
export LD=/usr/bin/ld

echo "========================================"
echo "Environment Cleanup Complete"
echo "========================================"

# ==============================================================

echo "================================"
echo "DiffusionAddon Build Script"
echo "================================"

ADDON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ADDON_DIR/build"
ROOT_DIR="$ADDON_DIR/../.."

ADDON_NAME="DiffusionAddon"
BUILT_ADDONS_BASE="$ROOT_DIR/build/addons"
ADDON_MAIN_DIR="$BUILT_ADDONS_BASE/$ADDON_NAME"
STAGING_DIR="$ADDON_MAIN_DIR/staging"
LIBS_DIR="$ADDON_MAIN_DIR/libs"

# Backend options
SD_CUDA="OFF"
SD_VULKAN="OFF"
SD_HIPBLAS="OFF"
SD_METAL="OFF"
SD_OPENCL="OFF"
SD_SYCL="OFF"
SD_MUSA="OFF"
SD_FAST_SOFTMAX="OFF"
CLEAN_BUILD=0

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -clean) CLEAN_BUILD=1; shift ;;
        --cuda) SD_CUDA="ON"; shift ;;
        --vulkan) SD_VULKAN="ON"; shift ;;
        --hipblas) SD_HIPBLAS="ON"; shift ;;
        --metal) SD_METAL="ON"; shift ;;
        --opencl) SD_OPENCL="ON"; shift ;;
        --sycl) SD_SYCL="ON"; shift ;;
        --musa) SD_MUSA="ON"; shift ;;
        --fast-softmax) SD_FAST_SOFTMAX="ON"; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

cd "$ADDON_DIR"

# Check if main project is built
if [ ! -f "$ROOT_DIR/build/lib/AniEngineCore.so" ] && [ ! -f "$ROOT_DIR/build/lib/libAniEngineCore.so" ]; then
    echo "ERROR: Main project shared libraries not found!"
    exit 1
fi

# Clean old build if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    echo "Cleaning old build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
mkdir -p "$STAGING_DIR"
mkdir -p "$LIBS_DIR"

cd "$BUILD_DIR"

# Configure CMake
if [ ! -f "CMakeCache.txt" ]; then
    echo "Configuring CMake..."
    
    CMAKE_CMD="cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja"
    CMAKE_CMD="$CMAKE_CMD -DCMAKE_C_COMPILER=/usr/bin/gcc"
    CMAKE_CMD="$CMAKE_CMD -DCMAKE_CXX_COMPILER=/usr/bin/g++"
    
    # FIX: We keep _GNU_SOURCE but remove the -U flags that were breaking mathcalls.h
    CMAKE_CMD="$CMAKE_CMD -DCMAKE_C_FLAGS='-D_GNU_SOURCE'"
    CMAKE_CMD="$CMAKE_CMD -DCMAKE_CXX_FLAGS='-D_GNU_SOURCE'"
    
    CMAKE_CMD="$CMAKE_CMD -DSD_CUDA=$SD_CUDA"
    CMAKE_CMD="$CMAKE_CMD -DSD_VULKAN=$SD_VULKAN"
    CMAKE_CMD="$CMAKE_CMD -DSD_OPENCL=$SD_OPENCL"
    CMAKE_CMD="$CMAKE_CMD -DSD_HIPBLAS=$SD_HIPBLAS"
    CMAKE_CMD="$CMAKE_CMD -DSD_METAL=$SD_METAL"
    CMAKE_CMD="$CMAKE_CMD -DSD_SYCL=$SD_SYCL"
    CMAKE_CMD="$CMAKE_CMD -DSD_MUSA=$SD_MUSA"
    CMAKE_CMD="$CMAKE_CMD -DSD_FAST_SOFTMAX=$SD_FAST_SOFTMAX"
    
    # Ensure it only looks at Linux system files
    CMAKE_CMD="$CMAKE_CMD -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY"
    CMAKE_CMD="$CMAKE_CMD -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY"
    
    echo "Running: $CMAKE_CMD"
    eval $CMAKE_CMD
    
    if [ $? -ne 0 ]; then echo "ERROR: CMake configuration failed!"; exit 1; fi
fi

# Build the addon
echo "Building addon..."
ninja DiffusionAddon
if [ $? -ne 0 ]; then echo "ERROR: Build failed!"; exit 1; fi

echo "[OK] Build successful. Library located in: $STAGING_DIR"
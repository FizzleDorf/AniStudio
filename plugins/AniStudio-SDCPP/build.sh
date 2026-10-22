#!/bin/bash

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

export CPATH=""
export CPLUS_INCLUDE_PATH=""
export C_INCLUDE_PATH=""
export INCLUDE=""
export DEPENDENCIES_INCLUDE=""
export SDKROOT=""

export CFLAGS=""
export CXXFLAGS=""
export CPPFLAGS=""
export LDFLAGS=""

export PATH=$(echo "$PATH" | tr ':' '\n' | grep -v -E '(msys64|mingw|ucrt64|windows|/mnt/[c-z])' | tr '\n' ':' | sed 's/:$//')

export CC=/usr/bin/gcc
export CXX=/usr/bin/g++
export LD=/usr/bin/ld

echo "========================================"
echo "Environment Cleanup Complete"
echo "========================================"

echo "================================"
echo "AniStudio-SDCPP Build Script"
echo "================================"
echo ""

ADDON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$ADDON_DIR/../.."

ADDON_NAME="AniStudio-SDCPP"
BUILT_ADDONS_BASE="$ROOT_DIR/build/plugins"

SD_CUDA="OFF"
SD_VULKAN="OFF"
SD_HIPBLAS="OFF"
SD_METAL="OFF"
SD_OPENCL="OFF"
SD_SYCL="OFF"
SD_MUSA="OFF"
SD_FAST_SOFTMAX="OFF"
SD_USE_SYSTEM_GGML="OFF"
CLEAN_BUILD=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        -clean) CLEAN_BUILD=1 ;;
        --cuda) SD_CUDA="ON" ;;
        --vulkan) SD_VULKAN="ON" ;;
        --hipblas) SD_HIPBLAS="ON" ;;
        --metal) SD_METAL="ON" ;;
        --opencl) SD_OPENCL="ON" ;;
        --sycl) SD_SYCL="ON" ;;
        --musa) SD_MUSA="ON" ;;
        --system-ggml) SD_USE_SYSTEM_GGML="ON" ;;
    esac
    shift
done

cd "$ADDON_DIR"

echo "Directory Configuration:"
echo "  Addon Base Name: $ADDON_NAME"
echo "  Source Dir: $ADDON_DIR"
echo "  AniStudio Root: $ROOT_DIR"
echo "  Built Addons Base: $BUILT_ADDONS_BASE"
echo ""
echo "Backend Configuration:"
echo "  CUDA: $SD_CUDA"
echo "  Vulkan: $SD_VULKAN"
echo "  HipBLAS: $SD_HIPBLAS"
echo "  Metal: $SD_METAL"
echo "  OpenCL: $SD_OPENCL"
echo "  SYCL: $SD_SYCL"
echo "  MUSA: $SD_MUSA"
echo "  Use System GGML: $SD_USE_SYSTEM_GGML"
echo ""

BACKEND_SUFFIX="_CPU"
if [ "$SD_CUDA" = "ON" ]; then BACKEND_SUFFIX="_CUDA"; fi
if [ "$SD_VULKAN" = "ON" ]; then BACKEND_SUFFIX="_VULKAN"; fi
if [ "$SD_HIPBLAS" = "ON" ]; then BACKEND_SUFFIX="_HIP"; fi
if [ "$SD_METAL" = "ON" ]; then BACKEND_SUFFIX="_METAL"; fi
if [ "$SD_OPENCL" = "ON" ]; then BACKEND_SUFFIX="_OPENCL"; fi
if [ "$SD_SYCL" = "ON" ]; then BACKEND_SUFFIX="_SYCL"; fi
if [ "$SD_MUSA" = "ON" ]; then BACKEND_SUFFIX="_MUSA"; fi

TARGET_NAME="${ADDON_NAME}${BACKEND_SUFFIX}"
ADDON_MAIN_DIR="$BUILT_ADDONS_BASE/$TARGET_NAME"
BUILD_DIR="$ADDON_MAIN_DIR/build"
STAGING_DIR="$ADDON_MAIN_DIR/staging"
STAGING_SO="$STAGING_DIR/${TARGET_NAME}.so"
LIBS_DIR="$ADDON_MAIN_DIR/libs"

echo "Computed Target: $TARGET_NAME"
echo "  Build Dir: $BUILD_DIR"
echo "  Staging Dir: $STAGING_DIR"
echo "  SO: $STAGING_SO"
echo ""

if [ ! -f "$ROOT_DIR/build/lib/libAniEngineCore.so" ] && [ ! -f "$ROOT_DIR/build/lib/AniEngineCore.so" ]; then
    echo "ERROR: Main project not built yet!"
    echo "Please build the main AniStudio project first."
    echo "Expected: $ROOT_DIR/build/lib/libAniEngineCore.so"
    exit 1
fi
echo "[OK] Main project libraries found"

if [ ! -f "$ROOT_DIR/build/conan/conan_toolchain.cmake" ]; then
    echo "WARNING: Conan toolchain not found. Please run 'conan install' first."
    echo "You can use the provided conanfile.txt and install.sh."
    echo "Continuing anyway, but build may fail if dependencies are missing."
    echo ""
fi

if [ "$CLEAN_BUILD" -eq 1 ]; then
    if [ -d "$BUILD_DIR" ]; then
        echo "Cleaning old build directory..."
        rm -rf "$BUILD_DIR"
    fi
    if [ -d "$ADDON_MAIN_DIR" ]; then
        echo "Cleaning old addon directory..."
        rm -rf "$ADDON_MAIN_DIR"
    fi
fi

mkdir -p "$BUILD_DIR" 2>/dev/null
mkdir -p "$ADDON_MAIN_DIR" 2>/dev/null
mkdir -p "$STAGING_DIR" 2>/dev/null
mkdir -p "$LIBS_DIR" 2>/dev/null

cd "$BUILD_DIR"

if [ -f CMakeCache.txt ]; then
    CACHE_SOURCE=$(grep "^CMAKE_HOME_DIRECTORY:" CMakeCache.txt | cut -d= -f2)
    CACHE_SOURCE="${CACHE_SOURCE//\\//}"
    ADDON_DIR_NORM="${ADDON_DIR//\\//}"
    CACHE_SOURCE=$(realpath -m "$CACHE_SOURCE" 2>/dev/null || echo "$CACHE_SOURCE")
    ADDON_DIR_NORM=$(realpath -m "$ADDON_DIR_NORM" 2>/dev/null || echo "$ADDON_DIR_NORM")
    if [ "$CACHE_SOURCE" != "$ADDON_DIR_NORM" ]; then
        echo "Warning: CMake cache points to a different source directory."
        echo "  Cache says: $CACHE_SOURCE"
        echo "  Current source: $ADDON_DIR"
        echo "Deleting stale cache..."
        rm -f CMakeCache.txt
        rm -rf CMakeFiles 2>/dev/null
    fi
fi

if [ ! -f "CMakeCache.txt" ]; then
    echo "Configuring CMake..."

        CMAKE_ARGS=(
        "$ADDON_DIR"
        -DCMAKE_BUILD_TYPE=Release
        -DSD_CUDA="$SD_CUDA"
        -DSD_VULKAN="$SD_VULKAN"
        -DSD_HIPBLAS="$SD_HIPBLAS"
        -DSD_METAL="$SD_METAL"
        -DSD_OPENCL="$SD_OPENCL"
        -DSD_SYCL="$SD_SYCL"
        -DSD_MUSA="$SD_MUSA"
        -DSD_USE_SYSTEM_GGML="$SD_USE_SYSTEM_GGML"
        -DGGML_NATIVE=OFF
        -DGGML_AVX=ON
        -DGGML_AVX2=ON
        -DGGML_FMA=ON
        -DGGML_F16C=ON
        -DGGML_AVX512=OFF
        -DGGML_AVX512_VBMI=OFF
        -DGGML_AVX512_VNNI=OFF
        -DGGML_AVX512_BF16=OFF
    )

    echo "Running: cmake ${CMAKE_ARGS[*]}"
    echo ""
    cmake "${CMAKE_ARGS[@]}"

    if [ $? -ne 0 ]; then
        echo "ERROR: CMake configuration failed!"
        exit 1
    fi
    echo "[OK] CMake configuration successful"
else
    echo "[OK] Using existing CMake configuration"
    echo "Note: To reconfigure with different backends, use -clean flag"
fi

echo "Building addon..."
cmake --build . --config Release --target "$TARGET_NAME"
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed!"
    exit 1
fi

echo "[OK] Addon build successful"

if [ -f "$STAGING_SO" ]; then
    echo "[OK] Addon SO created: $STAGING_SO"
    echo "Size: $(stat -c%s "$STAGING_SO") bytes"
else
    echo "ERROR: Addon SO not found!"
    echo "Expected: $STAGING_SO"
    exit 1
fi

echo ""
echo "================================"
echo "BUILD COMPLETED SUCCESSFULLY!"
echo "================================"
echo ""
echo "Addon SO: $STAGING_SO"
echo ""

echo "Checking for backend libraries..."
LIBS_NEEDED=0

if [ "$SD_CUDA" = "ON" ]; then
    echo "  CUDA enabled - ensure CUDA runtime libraries are available"
    LIBS_NEEDED=1
fi
if [ "$SD_VULKAN" = "ON" ]; then
    echo "  Vulkan enabled - ensure Vulkan SDK libraries are available"
    LIBS_NEEDED=1
fi
if [ "$SD_OPENCL" = "ON" ]; then
    echo "  OpenCL enabled - ensure OpenCL runtime is available"
    LIBS_NEEDED=1
fi
if [ "$SD_USE_SYSTEM_GGML" = "ON" ]; then
    echo "  System GGML used - ensure libggml.so is in LD_LIBRARY_PATH or alongside the addon"
    LIBS_NEEDED=1
fi

if [ "$LIBS_NEEDED" -eq 1 ]; then
    echo ""
    echo "IMPORTANT: Ensure all required backend and runtime libraries are available."
    echo "Place any required libraries in:"
    echo "  - $STAGING_DIR"
    echo "  - $ROOT_DIR/build/bin"
fi

echo ""
echo "Usage:"
echo "  ./build.sh [-clean] [--cuda] [--vulkan] [--opencl] [--hipblas] [--metal] [--system-ggml]"
echo ""
echo "Examples:"
echo "  ./build.sh --cuda --system-ggml          Build with CUDA and system GGML"
echo "  ./build.sh --vulkan --opencl             Build with Vulkan and OpenCL (CPU fallback)"
echo "  ./build.sh -clean --cuda                 Clean rebuild with CUDA"
echo ""
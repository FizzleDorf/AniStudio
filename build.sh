#!/bin/bash

# Configuration defaults
BUILD_TYPE="Release"
PARALLEL_JOBS=$(nproc)
CLEAN_BUILD=false
BUILD_SHARED="OFF"
ENABLE_CUDA="OFF"
ENABLE_VULKAN="OFF"
ENABLE_HIPBLAS="OFF"
ENABLE_METAL="OFF"
ENABLE_OPENCL="OFF"
ENABLE_SYCL="OFF"
ENABLE_MUSA="OFF"
ENABLE_FAST_SOFTMAX="OFF"

# Function to show help
show_help() {
    echo ""
    echo "AniStudio Build Script Usage:"
    echo ""
    echo "./build.sh [options]"
    echo ""
    echo "Build Options:"
    echo "  --clean              Perform a clean build (removes build directory)"
    echo "  --debug              Build in Debug mode (default: Release)"
    echo "  --shared             Build shared libraries (default: static)"
    echo "  --jobs <num>         Number of parallel build jobs (default: $(nproc))"
    echo ""
    echo "Stable Diffusion Backend Options:"
    echo "  --cuda               Enable CUDA support (NVIDIA GPUs)"
    echo "  --vulkan / --vk      Enable Vulkan support (cross-platform)"
    echo "  --hipblas / --rocm   Enable HipBLAS support (AMD GPUs, requires ROCm)"
    echo "  --metal              Enable Metal support (Apple devices only)"
    echo "  --opencl             Enable OpenCL support (requires OpenCL SDK)"
    echo "  --sycl               Enable SYCL support (Intel GPUs, requires oneAPI)"
    echo "  --musa               Enable MUSA support (Moore Threads GPUs)"
    echo "  --fast-softmax       Enable fast softmax (CUDA/HipBLAS/MUSA only)"
    echo ""
    echo "Examples:"
    echo "  ./build.sh                           # Standard CPU-only build"
    echo "  ./build.sh --cuda                    # CUDA-enabled build"
    echo "  ./build.sh --vulkan                  # Vulkan-enabled build"
    echo "  ./build.sh --cuda --vulkan           # Both CUDA and Vulkan"
    echo "  ./build.sh --hipblas --fast-softmax  # AMD ROCm with fast softmax"
    echo "  ./build.sh --clean --debug --sycl    # Clean debug build with Intel SYCL"
    echo "  ./build.sh --opencl --jobs 16        # OpenCL with 16 parallel jobs"
    echo ""
    echo "Prerequisites by Backend:"
    echo "  CUDA:     NVIDIA CUDA Toolkit"
    echo "  Vulkan:   Vulkan SDK from LunarG"
    echo "  HipBLAS:  AMD ROCm toolkit"
    echo "  Metal:    Built into macOS (Apple devices only)"
    echo "  OpenCL:   OpenCL headers + ICD loader"
    echo "  SYCL:     Intel oneAPI Base toolkit"
    echo "  MUSA:     Moore Threads MUSA toolkit"
    echo ""
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --shared)
            BUILD_SHARED="ON"
            shift
            ;;
        --cuda)
            ENABLE_CUDA="ON"
            shift
            ;;
        --vulkan|--vk)
            ENABLE_VULKAN="ON"
            shift
            ;;
        --hipblas|--rocm)
            ENABLE_HIPBLAS="ON"
            shift
            ;;
        --metal)
            ENABLE_METAL="ON"
            shift
            ;;
        --opencl)
            ENABLE_OPENCL="ON"
            shift
            ;;
        --sycl)
            ENABLE_SYCL="ON"
            shift
            ;;
        --musa)
            ENABLE_MUSA="ON"
            shift
            ;;
        --fast-softmax)
            ENABLE_FAST_SOFTMAX="ON"
            shift
            ;;
        --jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            echo "Unknown argument: $1"
            show_help
            exit 1
            ;;
    esac
done

echo "============================================"
echo "AniStudio Optimized Build Script"
echo "============================================"
echo "Build Type: $BUILD_TYPE"
echo "Shared Libs: $BUILD_SHARED"
echo "Parallel Jobs: $PARALLEL_JOBS"
echo "Clean Build: $CLEAN_BUILD"
echo ""
echo "Stable Diffusion Backends:"
echo "  CUDA: $ENABLE_CUDA"
echo "  Vulkan: $ENABLE_VULKAN"
echo "  HipBLAS (AMD): $ENABLE_HIPBLAS"
echo "  Metal (Apple): $ENABLE_METAL"
echo "  OpenCL: $ENABLE_OPENCL"
echo "  SYCL (Intel): $ENABLE_SYCL"
echo "  MUSA: $ENABLE_MUSA"
echo "  Fast Softmax: $ENABLE_FAST_SOFTMAX"
echo "============================================"

# Backend validation and warnings
if [[ "$ENABLE_METAL" == "ON" ]]; then
    if [[ "$OSTYPE" != "darwin"* ]]; then
        echo "WARNING: Metal backend is only supported on macOS"
    fi
fi

if [[ "$ENABLE_SYCL" == "ON" ]]; then
    echo "NOTE: SYCL requires Intel oneAPI Base toolkit"
    echo "      Run 'source /opt/intel/oneapi/setvars.sh' before building"
fi

if [[ "$ENABLE_HIPBLAS" == "ON" ]]; then
    echo "NOTE: HipBLAS requires ROCm toolkit for AMD GPUs"
fi

if [[ "$ENABLE_OPENCL" == "ON" ]]; then
    echo "NOTE: OpenCL requires OpenCL headers and ICD loader"
    echo "      On Ubuntu/Debian: sudo apt install opencl-headers ocl-icd-libopencl1"
fi

if [[ "$ENABLE_MUSA" == "ON" ]]; then
    echo "NOTE: MUSA requires MUSA toolkit for Moore Threads GPUs"
fi

# Activate the virtual environment if it exists
if [[ -f "build/venv/bin/activate" ]]; then
    echo "Activating virtual environment..."
    source build/venv/bin/activate
else
    echo "No virtual environment found, continuing without activation..."
fi

# Handle clean build
if [[ "$CLEAN_BUILD" == "true" ]]; then
    echo "Performing clean build..."
    if [[ -d "build" ]]; then
        echo "Removing build directory..."
        rm -rf build/{Release,bin,external,x64} 2>/dev/null || true
        
        # Also clean any standalone library builds
        [[ -d "src/aniengine/build" ]] && rm -rf src/aniengine/build
        [[ -d "src/anistudio/build" ]] && rm -rf src/anistudio/build
        [[ -d "src/aniplugins/build" ]] && rm -rf src/aniplugins/build
    else
        echo "Build directory does not exist - creating it"
        mkdir -p build
    fi
fi

# Create build directory if it doesn't exist
if [[ ! -d "build" ]]; then
    echo "Creating build directory..."
    mkdir -p build
fi

cd build

# Configure with CMake
echo "============================================"
echo "Configuring with CMake..."
echo "============================================"

cmake .. \
    -DCMAKE_INSTALL_PREFIX="$HOME/.local" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_SHARED_LIBS="$BUILD_SHARED" \
    -DSD_CUDA="$ENABLE_CUDA" \
    -DSD_VULKAN="$ENABLE_VULKAN" \
    -DSD_HIPBLAS="$ENABLE_HIPBLAS" \
    -DSD_METAL="$ENABLE_METAL" \
    -DSD_OPENCL="$ENABLE_OPENCL" \
    -DSD_SYCL="$ENABLE_SYCL" \
    -DSD_MUSA="$ENABLE_MUSA" \
    -DSD_FAST_SOFTMAX="$ENABLE_FAST_SOFTMAX" \
    -DBUILD_ANIENGINE=ON \
    -DBUILD_ANISTUDIO=ON \
    -DBUILD_ANIPLUGINS=ON \
    -DBUILD_MAIN_APP=ON \
    -GNinja

if [[ $? -ne 0 ]]; then
    echo "============================================"
    echo "CMake configuration failed!"
    echo "============================================"
    cd ..
    [[ -f "build/venv/bin/activate" ]] && deactivate
    exit 1
fi

# Build the project
echo "============================================"
echo "Building project..."
echo "============================================"

cmake --build . --config "$BUILD_TYPE" --parallel "$PARALLEL_JOBS"

if [[ $? -ne 0 ]]; then
    echo "============================================"
    echo "Build failed!"
    echo "============================================"
    cd ..
    [[ -f "build/venv/bin/activate" ]] && deactivate
    exit 1
fi

echo "============================================"
echo "Build completed successfully!"
echo "============================================"
echo ""
echo "Binaries location: $(pwd)/bin"
echo "Libraries location: $(pwd)/lib"
echo "Plugins location: $(pwd)/plugins"
echo ""

# Check if executable was created
if [[ -f "bin/AniStudio" ]]; then
    echo "AniStudio executable: bin/AniStudio"
    
    # Ask if user wants to run the application
    read -p "Run AniStudio now? (y/n): " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Starting AniStudio..."
        ./bin/AniStudio
    fi
else
    echo "Warning: AniStudio executable was not found in bin directory"
fi

cd ..
[[ -f "build/venv/bin/activate" ]] && deactivate

echo ""
echo "Build completed successfully!"
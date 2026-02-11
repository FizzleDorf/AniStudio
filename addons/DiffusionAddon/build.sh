echo "================================"
echo "DiffusionAddon Build Script"
echo "================================"
echo

ADDON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ADDON_DIR/build"
ROOT_DIR="$ADDON_DIR/../.."

ADDON_NAME="DiffusionAddon"
BUILT_ADDONS_BASE="$ROOT_DIR/build/addons"
ADDON_MAIN_DIR="$BUILT_ADDONS_BASE/$ADDON_NAME"
STAGING_DIR="$ADDON_MAIN_DIR/staging"
STAGING_SO="$STAGING_DIR/lib$ADDON_NAME.so"
LIBS_DIR="$ADDON_MAIN_DIR/libs"

# Backend options (set to ON to enable)
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
        -clean)
            CLEAN_BUILD=1
            shift
            ;;
        --cuda)
            SD_CUDA="ON"
            shift
            ;;
        --vulkan)
            SD_VULKAN="ON"
            shift
            ;;
        --hipblas)
            SD_HIPBLAS="ON"
            shift
            ;;
        --metal)
            SD_METAL="ON"
            shift
            ;;
        --opencl)
            SD_OPENCL="ON"
            shift
            ;;
        --sycl)
            SD_SYCL="ON"
            shift
            ;;
        --musa)
            SD_MUSA="ON"
            shift
            ;;
        --fast-softmax)
            SD_FAST_SOFTMAX="ON"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

cd "$ADDON_DIR"

echo "Directory Configuration:"
echo "  Addon Name: $ADDON_NAME"
echo "  Source Dir: $ADDON_DIR"
echo "  AniStudio Root: $ROOT_DIR"
echo "  Built Addons Base: $BUILT_ADDONS_BASE"
echo "  Addon Main Dir: $ADDON_MAIN_DIR"
echo "  Staging Dir: $STAGING_DIR"
echo "  Libraries Dir: $LIBS_DIR"
echo "  Build Target: $STAGING_SO"
echo
echo "Backend Configuration:"
echo "  CUDA: $SD_CUDA"
echo "  Vulkan: $SD_VULKAN"
echo "  HipBLAS: $SD_HIPBLAS"
echo "  Metal: $SD_METAL"
echo "  OpenCL: $SD_OPENCL"
echo "  SYCL: $SD_SYCL"
echo "  MUSA: $SD_MUSA"
echo "  Fast Softmax: $SD_FAST_SOFTMAX"
echo

# Check if main project is built
if [ ! -f "$ROOT_DIR/build/lib/libAniEngineCore.a" ] && [ ! -f "$ROOT_DIR/build/lib/AniEngineCore.lib" ]; then
    echo "ERROR: Main project not built yet!"
    echo "Please build the main AniStudio project first."
    echo "Expected: $ROOT_DIR/build/lib/libAniEngineCore.a or $ROOT_DIR/build/lib/AniEngineCore.lib"
    exit 1
fi

echo "[OK] Main project libraries found"

# Clean old build if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    if [ -d "$BUILD_DIR" ]; then
        echo "Cleaning old build directory..."
        rm -rf "$BUILD_DIR"
    fi
fi

# Create directories
mkdir -p "$BUILD_DIR"
mkdir -p "$BUILT_ADDONS_BASE"
mkdir -p "$ADDON_MAIN_DIR"
mkdir -p "$STAGING_DIR"
mkdir -p "$LIBS_DIR"

cd "$BUILD_DIR"

# Configure CMake
if [ ! -f "CMakeCache.txt" ]; then
    echo "Configuring CMake..."
    
    # Build CMake command with backend options
    CMAKE_CMD="cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja"
    CMAKE_CMD="$CMAKE_CMD -DSD_CUDA=$SD_CUDA"
    CMAKE_CMD="$CMAKE_CMD -DSD_VULKAN=$SD_VULKAN"
    CMAKE_CMD="$CMAKE_CMD -DSD_HIPBLAS=$SD_HIPBLAS"
    CMAKE_CMD="$CMAKE_CMD -DSD_METAL=$SD_METAL"
    CMAKE_CMD="$CMAKE_CMD -DSD_OPENCL=$SD_OPENCL"
    CMAKE_CMD="$CMAKE_CMD -DSD_SYCL=$SD_SYCL"
    CMAKE_CMD="$CMAKE_CMD -DSD_MUSA=$SD_MUSA"
    CMAKE_CMD="$CMAKE_CMD -DSD_FAST_SOFTMAX=$SD_FAST_SOFTMAX"
    
    echo "Running: $CMAKE_CMD"
    echo
    eval $CMAKE_CMD
    
    if [ $? -ne 0 ]; then
        echo "ERROR: CMake configuration failed!"
        exit 1
    fi
    echo "[OK] CMake configuration successful"
else
    echo "[OK] Using existing CMake configuration"
    echo "Note: To reconfigure with different backends, use -clean flag"
fi

# Build the addon
echo "Building addon..."
ninja $ADDON_NAME
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed!"
    exit 1
fi

echo "[OK] Addon build successful"

# Check if shared library was created
if [ -f "$STAGING_SO" ]; then
    echo "[OK] Addon shared library created: $STAGING_SO"
    echo "Size: $(stat -c%s "$STAGING_SO") bytes"
else
    echo "ERROR: Addon shared library not found!"
    echo "Expected: $STAGING_SO"
    exit 1
fi

echo
echo "================================"
echo "BUILD COMPLETED SUCCESSFULLY!"
echo "================================"
echo
echo "Addon Shared Library: $STAGING_SO"
echo

# Check for backend libraries
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

if [ $LIBS_NEEDED -eq 1 ]; then
    echo
    echo "IMPORTANT: Ensure all required backend libraries are available at runtime!"
    echo "Place stable-diffusion shared libraries and backend-specific libraries in:"
    echo "  - $STAGING_DIR"
    echo "  - $ROOT_DIR/build/bin"
fi

echo
echo "Usage:"
echo "  ./build.sh [-clean] [--cuda] [--vulkan] [--opencl] [--hipblas] [--metal]"
echo
echo "Examples:"
echo "  ./build.sh --cuda              Build with CUDA support"
echo "  ./build.sh --vulkan --opencl   Build with Vulkan and OpenCL"
echo "  ./build.sh -clean --cuda       Clean rebuild with CUDA"
echo
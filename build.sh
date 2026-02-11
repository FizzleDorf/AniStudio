#!/bin/bash

# Configuration defaults
BUILD_TYPE="Release"
PARALLEL_JOBS=$(nproc)
CLEAN_BUILD=false
BUILD_SHARED="OFF"

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
    echo "Examples:"
    echo "  ./build.sh                           # Standard build"
    echo "  ./build.sh --clean --debug           # Clean debug build"
    echo "  ./build.sh --jobs 16                 # Use 16 parallel jobs"
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
echo "AniStudio Build Script"
echo "============================================"
echo "Build Type: $BUILD_TYPE"
echo "Shared Libs: $BUILD_SHARED"
echo "Parallel Jobs: $PARALLEL_JOBS"
echo "Clean Build: $CLEAN_BUILD"
echo "============================================"

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
    -DZEP_FEATURE_USE_VCPKG=OFF \
    -DBUILD_ANIENGINE=ON \
    -DBUILD_ANISTUDIO=ON \
    -DBUILD_ANIPLUGINS=ON \
    -DBUILD_MAIN_APP=ON \
    -DBUILD_DEMOS=OFF \
    -DBUILD_IMGUI=OFF \
    -DBUILD_QT=OFF \
    -DBUILD_TESTS=OFF \
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
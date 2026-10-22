#!/bin/bash

BUILD_TYPE="Release"
JOBS=8
CLEAN=0
SHARED="ON"
APP_MODE="local"
BUILD_DIR="build"

show_help() {
    echo "Usage: ./build.sh [options]"
    echo ""
    echo "Build type:"
    echo "  --debug               Build in Debug mode        (default: Release)"
    echo "  --release             Build in Release mode"
    echo ""
    echo "Build mode:"
    echo "  --local               Build the local desktop app  (default)"
    echo "  --server              Build headless AniServer"
    echo "  --client              Build networked AniClient"
    echo "  --both                Build AniServer AND AniClient"
    echo ""
    echo "Library linkage:"
    echo "  --shared              Build shared libraries (.so)  (default)"
    echo "  --static              Build static libraries"
    echo ""
    echo "Other:"
    echo "  --clean               Remove build directory before configuring"
    echo "  --jobs N              Parallel build jobs (default: 8)"
    echo "  --build-dir DIR       Use DIR as build directory (default: build)"
    echo "  --help, -h            Show this help"
    echo ""
    echo "Examples:"
    echo "  ./build.sh --local"
    echo "  ./build.sh --server --debug"
    echo "  ./build.sh --client --release --jobs 16"
    echo "  ./build.sh --both --clean"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug) BUILD_TYPE="Debug"; shift ;;
        --release) BUILD_TYPE="Release"; shift ;;
        --clean) CLEAN=1; shift ;;
        --shared) SHARED="ON"; shift ;;
        --static) SHARED="OFF"; shift ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --local) APP_MODE="local"; shift ;;
        --server) APP_MODE="server"; shift ;;
        --client) APP_MODE="client"; shift ;;
        --both) APP_MODE="both"; shift ;;
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --help|-h) show_help; exit 0 ;;
        *) echo "Unknown argument: $1"; show_help; exit 1 ;;
    esac
done

echo "=========================================="
echo "Building AniStudio"
echo "=========================================="
echo "Build type: $BUILD_TYPE"
echo "Build mode: $APP_MODE"
echo "Shared libs: $SHARED"
echo "Jobs: $JOBS"
echo "Clean: $CLEAN"
echo "Build dir: $BUILD_DIR"
echo "=========================================="

if [ "$CLEAN" -eq 1 ]; then
    echo "Removing $BUILD_DIR directory..."
    rm -rf "$BUILD_DIR" 2>/dev/null
fi

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

pushd "$BUILD_DIR" > /dev/null

echo ""
echo "Configuring with CMake..."
echo ""

cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_SHARED_LIBS="$SHARED" \
    -DBUILD_ANIENGINE=ON \
    -DBUILD_ANISTUDIO=ON \
    -DBUILD_ANIPLUGINS=ON \
    -DBUILD_MAIN_APP=ON \
    -DBUILD_APP_MODE="$APP_MODE" \
    -GNinja

if [ $? -ne 0 ]; then
    echo ""
    echo "CMake configuration failed!"
    popd > /dev/null
    exit 1
fi

echo ""
echo "Building..."
echo ""

cmake --build . --config "$BUILD_TYPE" --parallel "$JOBS"

if [ $? -ne 0 ]; then
    echo ""
    echo "Build failed!"
    popd > /dev/null
    exit 1
fi

echo ""
echo "=========================================="
echo "Build succeeded!"
echo "=========================================="

if [ "$APP_MODE" = "local" ]; then
    echo "Binary: $(pwd)/AniStudio"
fi
if [ "$APP_MODE" = "server" ]; then
    echo "Binary: $(pwd)/AniServer"
fi
if [ "$APP_MODE" = "client" ]; then
    echo "Binary: $(pwd)/AniClient"
fi
if [ "$APP_MODE" = "both" ]; then
    echo "Binaries: $(pwd)/AniServer"
    echo "          $(pwd)/AniClient"
fi
echo "Libraries: $(pwd)/lib"
echo ""

popd > /dev/null

if [ "$APP_MODE" = "server" ]; then
    read -p "Launch the server? [y/N] " LAUNCH
    if [[ "$LAUNCH" =~ ^[Yy]$ ]]; then
        "$BUILD_DIR/AniServer" 9000 &
    fi
fi
if [ "$APP_MODE" = "both" ]; then
    read -p "Launch the server? [y/N] " LAUNCH
    if [[ "$LAUNCH" =~ ^[Yy]$ ]]; then
        "$BUILD_DIR/AniServer" 9000 &
    fi
fi
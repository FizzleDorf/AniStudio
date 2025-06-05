#!/bin/bash

# ExamplePlugin build script
# Usage: ./build.sh [anistudio_build_dir] [build_type]

ANISTUDIO_BUILD_DIR="$1"
BUILD_TYPE="$2"

# Default values
if [ -z "$ANISTUDIO_BUILD_DIR" ]; then
    ANISTUDIO_BUILD_DIR="../../build"
fi

if [ -z "$BUILD_TYPE" ]; then
    BUILD_TYPE="Release"
fi

# Make paths absolute
ANISTUDIO_BUILD_DIR=$(realpath "$ANISTUDIO_BUILD_DIR" 2>/dev/null || echo "$(cd "$ANISTUDIO_BUILD_DIR" && pwd)")
SCRIPT_DIR=$(dirname "$(realpath "$0" 2>/dev/null || echo "$(cd "$(dirname "$0")" && pwd)")")

echo "=== ExamplePlugin Build Script ==="
echo "Plugin directory: $SCRIPT_DIR"
echo "AniStudio build dir: $ANISTUDIO_BUILD_DIR"
echo "Build type: $BUILD_TYPE"
echo

# Check if AniStudio is built
if [ ! -f "$ANISTUDIO_BUILD_DIR/lib/libAniStudioCore.a" ] && [ ! -f "$ANISTUDIO_BUILD_DIR/lib/AniStudioCore.lib" ]; then
    echo "Error: AniStudioCore library not found in '$ANISTUDIO_BUILD_DIR/lib/'"
    echo "Please build AniStudio first!"
    echo "Expected files:"
    echo "  Linux: $ANISTUDIO_BUILD_DIR/lib/libAniStudioCore.a"
    echo "  Windows: $ANISTUDIO_BUILD_DIR/lib/AniStudioCore.lib"
    exit 1
fi

# Check if CMake config exists (try install directory first, then build directory)
if [ -f "$ANISTUDIO_BUILD_DIR/install/lib/cmake/AniStudioCore/AniStudioCoreConfig.cmake" ]; then
    echo "Found AniStudioCore config in install directory"
elif [ -f "$ANISTUDIO_BUILD_DIR/lib/cmake/AniStudioCore/AniStudioCoreConfig.cmake" ]; then
    echo "Found AniStudioCore config in build directory"
else
    echo "Error: AniStudioCore CMake config not found!"
    echo "Checked:"
    echo "  $ANISTUDIO_BUILD_DIR/install/lib/cmake/AniStudioCore/AniStudioCoreConfig.cmake"
    echo "  $ANISTUDIO_BUILD_DIR/lib/cmake/AniStudioCore/AniStudioCoreConfig.cmake"
    echo "Please run 'cmake --install . --prefix ./install' from the AniStudio build directory"
    exit 1
fi

# Create build directory
BUILD_DIR="$SCRIPT_DIR/build"
mkdir -p "$BUILD_DIR"

echo "Cleaning previous build..."
rm -rf "$BUILD_DIR"/*

# Configure CMake
echo "Configuring CMake..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DANISTUDIO_BUILD_DIR="$ANISTUDIO_BUILD_DIR" \
    -DANISTUDIO_PLUGIN_DIR="$ANISTUDIO_BUILD_DIR/plugins" \
    "$SCRIPT_DIR"

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed!"
    exit 1
fi

# Build the plugin
echo "Building ExamplePlugin..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE"

if [ $? -ne 0 ]; then
    echo "Error: Plugin build failed!"
    exit 1
fi

echo
echo "=== Build Successful ==="
OUTPUT_DIR="$ANISTUDIO_BUILD_DIR/plugins"

# List the built plugin file
if [ -f "$OUTPUT_DIR/libExamplePlugin.so" ]; then
    echo "Built: $OUTPUT_DIR/libExamplePlugin.so"
    ls -la "$OUTPUT_DIR/libExamplePlugin.so"
elif [ -f "$OUTPUT_DIR/ExamplePlugin.dll" ]; then
    echo "Built: $OUTPUT_DIR/ExamplePlugin.dll"
    ls -la "$OUTPUT_DIR/ExamplePlugin.dll"
else
    echo "Warning: Plugin file not found in expected location: $OUTPUT_DIR"
    echo "Checking build directory for plugin files..."
    find "$BUILD_DIR" -name "*.so" -o -name "*.dll" -o -name "*.dylib" | head -5
fi

echo
echo "To use this plugin:"
echo "1. Start AniStudio"
echo "2. The plugin should be automatically loaded from: $OUTPUT_DIR"
echo "3. Check the plugin manager to verify it loaded successfully"
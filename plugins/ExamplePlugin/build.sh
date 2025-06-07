#!/bin/bash
# build_example_plugin.sh

set -e  # Exit on error

# Configuration
ANISTUDIO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ANISTUDIO_BUILD_DIR="$ANISTUDIO_ROOT/build"
PLUGIN_SOURCE_DIR="$ANISTUDIO_ROOT/plugins/ExamplePlugin"
PLUGIN_BUILD_DIR="$ANISTUDIO_BUILD_DIR/plugins/ExamplePlugin"
PLUGIN_OUTPUT_DIR="$ANISTUDIO_BUILD_DIR/plugins"

echo "=== Building ExamplePlugin ==="
echo "AniStudio root: $ANISTUDIO_ROOT"
echo "AniStudio build: $ANISTUDIO_BUILD_DIR" 
echo "Plugin source: $PLUGIN_SOURCE_DIR"
echo "Plugin build: $PLUGIN_BUILD_DIR"
echo "Plugin output: $PLUGIN_OUTPUT_DIR"

# Check that AniStudio has been built first
if [ ! -f "$ANISTUDIO_BUILD_DIR/lib/libAniStudioCore.a" ] && [ ! -f "$ANISTUDIO_BUILD_DIR/lib/AniStudioCore.lib" ]; then
    echo "ERROR: AniStudio must be built first! Run 'cmake --build build' in the AniStudio root directory."
    exit 1
fi

# Create plugin build directory
mkdir -p "$PLUGIN_BUILD_DIR"
mkdir -p "$PLUGIN_OUTPUT_DIR"

# Navigate to plugin build directory
cd "$PLUGIN_BUILD_DIR"

# Configure the plugin
cmake "$PLUGIN_SOURCE_DIR" \
    -DANISTUDIO_BUILD_DIR="$ANISTUDIO_BUILD_DIR" \
    -DANISTUDIO_PLUGIN_DIR="$PLUGIN_OUTPUT_DIR" \
    -DCMAKE_BUILD_TYPE=Release

# Build the plugin
cmake --build . --config Release

echo "=== Plugin build complete ==="
echo "Plugin location: $PLUGIN_OUTPUT_DIR/"

# List the built plugins
echo "Built plugins:"
ls -la "$PLUGIN_OUTPUT_DIR/"*.so "$PLUGIN_OUTPUT_DIR/"*.dll 2>/dev/null || echo "No plugins found"
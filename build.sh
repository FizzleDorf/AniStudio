#!/bin/bash

# Activate the virtual environment
source build/venv/bin/activate

# Navigate to the build directory if it exists
if [ -d "build" ]; then
    cd build
    
    # Delete specific directories if they exist
    [ -d "Release" ] && rm -rf Release
    [ -d "bin" ] && rm -rf bin
    [ -d "external" ] && rm -rf external
    [ -d "x64" ] && rm -rf x64
    
    # Delete all files directly in the build directory
    rm -f *
    
    # Go back to the parent directory
    cd ..
else
    echo "Build directory does not exist."
fi

# Create build directory if it was deleted or does not exist
mkdir -p build
cd build

# Configure with CMake (only build AniStudio)
cmake .. -DCMAKE_INSTALL_PREFIX="$HOME/.local" \
         -DCMAKE_BUILD_TYPE=Release \
         -DSD_VULKAN=ON \
         -DBUILD_PLUGINS=OFF \
         -DBUILD_ANISTUDIO=ON

# Build the project
cmake --build . --config Release

# Deactivate the virtual environment
deactivate
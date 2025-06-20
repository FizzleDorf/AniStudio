#!/bin/bash

# Activate the virtual environment
source build/venv/bin/activate

# Clean build directory more efficiently
if [ -d "build" ]; then
    # Only delete specific directories/files that actually need to be cleaned
    rm -rf build/{Release,bin,external,x64} 2>/dev/null
else
    echo "Build directory does not exist - creating it"
    mkdir -p build
fi

cd build

# Configure with CMake (only build AniStudio)
cmake .. -DCMAKE_INSTALL_PREFIX="$HOME/.local" \
         -DCMAKE_BUILD_TYPE=Release \
         -DSD_VULKAN=ON # \
         # -GNinja  # Use Ninja instead of Make for faster builds

# Build the project with all cores and skip clean (since we already cleaned)
cmake --build . --config Release --parallel $(nproc)

# Alternative if using Ninja (even faster):
# ninja -C . -j $(nproc)

deactivate
#!/bin/bash
set -e

# Function to detect the OS
detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$NAME
    elif type lsb_release >/dev/null 2>&1; then
        OS=$(lsb_release -si)
    elif [ -f /etc/lsb-release ]; then
        . /etc/lsb-release
        OS=$DISTRIB_ID
    else
        OS=$(uname -s)
    fi
}

# Detect the OS
detect_os

# Activate the virtual environment
echo "Activating virtual environment..."
if [ "$OS" = "macOS" ]; then
    source venv/bin/activate
else
    source venv/bin/activate
fi

# Remove build directory if it exists and create a new one
echo "Creating build directory..."
rm -rf build
mkdir build
cd build

echo "Configuring with CMake..."
if [ "$OS" = "macOS" ]; then
    cmake .. -G "Xcode" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
else
    cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
fi

echo "Building..."
cmake --build . --config Release

echo "Build completed successfully."

# Deactivate the virtual environment
deactivate

cd ..

# On macOS, we don't need to pause
if [ "$OS" != "macOS" ]; then
    echo "Press any key to continue..."
    read -n 1 -s
fi
#!/bin/bash

# Check for Python installation
if ! command -v python3 &> /dev/null; then
    echo "Python3 is not installed or not in PATH. Please install Python3 and try again."
    exit 1
fi

# Install required system packages
echo "Installing required system packages..."
if command -v apt &> /dev/null; then
    sudo apt update
    sudo apt install -y python3-full python3-venv ninja-build build-essential cmake
elif command -v dnf &> /dev/null; then
    sudo dnf install -y python3 python3-pip python3-virtualenv ninja-build gcc gcc-c++ make cmake
elif command -v pacman &> /dev/null; then
    sudo pacman -Sy --noconfirm python python-pip python-virtualenv ninja base-devel cmake
elif command -v zypper &> /dev/null; then
    sudo zypper install -y python3 python3-pip python3-virtualenv ninja gcc gcc-c++ make cmake
else
    echo "WARNING: Could not detect a supported package manager (apt/dnf/pacman/zypper)."
    echo "Please install python3, python3-venv, ninja-build, build-essential, and cmake manually."
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

# Remove existing venv if it's broken
if [ -d "build/venv" ] && [ ! -f "build/venv/bin/activate" ]; then
    echo "Removing broken virtual environment..."
    rm -rf build/venv
fi

# Check if virtual environment exists in build directory
if [ -d "build/venv" ]; then
    echo "Virtual environment already exists in build directory."
else
    echo "Creating virtual environment in build directory..."
    python3 -m venv build/venv
fi

# Activate virtual environment
source build/venv/bin/activate

# Upgrade pip
python -m pip install --upgrade pip

# Install Conan
pip install conan

# Create default Conan profile
conan profile detect --force

# Run Conan install
conan install . --build=missing -s compiler.cppstd=17

echo "Installation completed successfully."
echo "To build the project, run './build.sh'"

# Deactivate virtual environment
deactivate
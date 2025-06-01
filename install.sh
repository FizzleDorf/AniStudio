#!/bin/bash

# Check for Python installation
if ! command -v python3 &> /dev/null; then
    echo "Python is not installed or not in PATH. Please install Python and try again."
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

# Check if virtual environment exists in build directory
if [ -d "build/venv" ]; then
    echo "Virtual environment already exists in build directory."
else
    echo "Creating virtual environment in build directory..."
    python -m venv build/venv
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
conan install . --build=missing

echo "Installation completed successfully."
echo "To build the project, run './build.sh'"

# Deactivate virtual environment
deactivate
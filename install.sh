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

# Install Vulkan based on the OS
install_vulkan() {
    echo "Installing Vulkan..."
    case $OS in
        "Ubuntu" | "Debian GNU/Linux")
            sudo apt update
            sudo apt install -y libvulkan-dev vulkan-utils
            ;;
        "Fedora")
            sudo dnf install -y vulkan-loader-devel vulkan-tools
            ;;
        "macOS")
            brew install molten-vk
            ;;
        *)
            echo "Unsupported OS for Vulkan installation: $OS"
            exit 1
            ;;
    esac
    echo "Vulkan Installed"
}

# Check for Python installation
if ! command -v python3 &> /dev/null
then
    echo "Python3 is not installed or not in PATH. Please install Python3 and try again."
    exit 1
fi

# Install Vulkan
install_vulkan

# Check if virtual environment exists
if [ -d "venv" ]; then
    echo "Virtual environment already exists."
else
    echo "Creating virtual environment..."
    python3 -m venv venv
fi

# Activate virtual environment
source venv/bin/activate

# Upgrade pip
echo "Upgrading pip..."
python -m pip install --upgrade pip

# Install Conan
echo "Installing Conan..."
pip install conan

# Create default Conan profile
echo "Creating default Conan profile..."
conan profile detect --force

# Run Conan in the conan subdirectory
echo "Installing Conan dependencies..."
cd conan
conan install ../conan --build=missing
cd ..

echo "Installation completed successfully."
echo "To build the project, run './build.sh'"

# Deactivate virtual environment
deactivate

echo "Please restart your terminal to ensure all PATH changes take effect."
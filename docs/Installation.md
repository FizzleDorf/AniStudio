# Building and Installing from Source

*Disclaimer: builds were only tested with MSVC for windows and Gcc for Linux*

## Requirements

### **Windows**
- **Vulkan**  
  - Download from [here](https://vulkan.lunarg.com/sdk/home)).  
  - Verify with `vulkaninfo`.  

- **Python >= 3.10**  
  - Download from [here](https://www.python.org/).  
  - Ensure the installer adds Python to your PATH.  
  - Verify with `python --version`.  

- **CMake >= 3.20**  
  - Download from [here](https://cmake.org/download/).  
  - Verify with `cmake --version`.
    
- **Visual Studio 2022 (2015 at least)**
  -  Download from [here](https://visualstudio.microsoft.com/).
  -  make sure you install the MSVC build tools

### **Ubuntu/Debian**
- **Vulkan**  
  - Install:  
    ```bash
    sudo apt install vulkan-tools libvulkan-dev
    ```  
  - Verify with `vulkaninfo`.  

- **Python >= 3.10**  
  - Install:  
    ```bash
    sudo apt install python3 python3-pip
    ```  
  - If Python 3.10+ is not available, add a PPA and install:  
    ```bash
    sudo add-apt-repository ppa:deadsnakes/ppa
    sudo apt update
    sudo apt install python3.10 python3.10-venv python3.10-distutils
    ```  
  - Verify with `python --version`.  

- **CMake >= 3.20**  
  - Remove old CMake (if needed):  
    ```bash
    sudo apt remove --purge cmake
    ```  
  - Download and install:  
    ```bash
    sudo apt update
    sudo apt install wget
    wget https://github.com/Kitware/CMake/releases/download/v3.20.0/cmake-3.20.0-linux-x86_64.sh
    sudo bash cmake-3.20.0-linux-x86_64.sh --prefix=/usr/local --skip-license
    ```  
  - Verify with `cmake --version`.  

### **Fedora**
- **Vulkan**  
  - Install:  
    ```bash
    sudo dnf install vulkan vulkan-tools vulkan-headers
    ```  
  - Verify with `vulkaninfo`.  

- **Python >= 3.10**  
  - Install:  
    ```bash
    sudo dnf install python3 python3-pip
    ```  
  - Verify with `python --version`.  

- **CMake >= 3.20**  
  - Install:  
    ```bash
    sudo dnf install cmake
    ```  
  - If outdated, download and install manually (follow Ubuntu instructions).  
  - Verify with `cmake --version`.  

### **Arch Linux**
- **Vulkan**  
  - Install:  
    ```bash
    sudo pacman -Syu vulkan-tools vulkan-devel
    ```  
  - Verify with `vulkaninfo`.  

- **Python >= 3.10**  
  - Install:  
    ```bash
    sudo pacman -Syu python python-pip
    ```  
  - Verify with `python --version`.  

- **CMake >= 3.20**  
  - Install:  
    ```bash
    sudo pacman -Syu cmake
    ```  
  - Verify with `cmake --version`.  

---

## Clone
*cloning, installing and building will take a while. speeding up the build process using repo options is in the TODO*
```
git clone https://github.com/Fizzledorf/AniStudio.git
cd AniStudio
git submodule update --init --recursive

```
## Install Conan Package Manager

### Windows
.bat files have been made to assist installing conan and building from source
the .exe will be located in `./build/bin` directory.
run the scripts in this order:

```
.\install.bat
.\build.bat
```
*note: the script builds with the vulkan backend by default*

to install conan requirements from the command line, enter the following while pathed in the root:
*note: installing to a venv or conda environment is optional, this is the only required package*
```
pip install conan
conan profile detect --force
conan install . --build=missing -s compiler.cppstd=17
```

to build with CMake from the command line, enter the following while pathed in the root:
*note: the command with no backend flags builds with the CPU backend by default, see the supported flags later in the instructions*

```
cd build
cmake ..
cmake --build .
```

### Linux
.sh files have been made to assist installing conan and building from source
the .exe will be located in `./build/bin` directory.

#### Compilers
*You may need to make sure your compiler is up to date*

##### for gcc (Ubuntu/Debian)
```
sudo apt update
sudo apt install gcc g++
```

##### Clang (Ubuntu/Debian)
```
sudo apt update
sudo apt install clang clang-format lldb
```

run the scripts in this order:

```
.\install.sh
.\build.sh
```
*note: the script builds with the vulkan backend by default*

to install conan requirements from the command line, enter the following while pathed in the root:
*note: installing to a venv or conda environment is optional, this is the only required package*
```
pip3 install conan
conan profile detect --force
conan install . --build=missing -s compiler.cppstd=17
```

to build with CMake from the command line, enter the following while pathed in the root:
*note: the command with no backend flags builds with the CPU backend by default, see the supported flags later in the instructions*
```
cd build
cmake ..
cmake --build .
```

## Main Build Options

| Command Flag             | Description                                                                     |
|--------------------------|---------------------------------------------------------------------------------|
| ```-DBUILD_ANISTUDIO=ON```| Configures and builds the main AniStudio Project                               |
| ```-DBUILD_PLUGINS=ON```      | Configures and builds all the plugin projects in the root `/plugins` directory |

#### Stable Diffusion Build Options

for building with different inference backends, you need to use the appropriate flags.

| Command Flag             | Description                                                                     |
|--------------------------|---------------------------------------------------------------------------------|
| ```-DSD_CUBLAS=ON```      | Enable CUDA backend.                                                            |
| ```-DSD_HIPBLAS=ON```     | Enable ROCm (HIPBLAS) backend.                                                  |
| ```-DSD_METAL=ON```       | Enable Metal backend (macOS only).                                              |
| ```-DSD_VULKAN=ON```      | Enable Vulkan backend.                                                          |
| ```-DSD_SYCL=ON```        | Enable SYCL backend.                                                            |
| ```-DSD_FLASH_ATTN=ON```  | Use Flash Attention for x4 less memory usage.                                   |
| ```-DSD_FAST_SOFTMAX=ON```| Enable faster Softmax (1.5x speedup but indeterministic). CUDA only.            |
| ```-DSD_BUILD_SHARED_LIBS=ON```| Build shared libraries instead of static libraries.                        |

#### Example
for launching with a vulkan backend with flash attention, the build command would look like:
```
mkdir build
cd build
cmake .. -DSD_VULKAN=ON -DSD_FLASH_ATTN=ON
cmake --build .
```
*note: this build command works for both Windows and Linux*

## Issues with Installing, Building or Compiling
Plese leave a detailed report in an Issue. When writing an issue for this, please include any error messages and the conan profile that was used.
You can see the profile using ```conan profile detect --force```

# Anistudio
An executable editor for image and video diffusion generation and editing projects written in C/C++ by utilizing [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp) backend.

## Purpose
I created this application to leverage C/C++ for a highly modular and parallelized system capable of loading, inferencing, and training machine learning models, all within a unified UI. Existing UI solutions often rely on external tools for refining or editing, or suffer from performance issues due to JavaScript, TypeScript, or Python limitations—resulting in inconsistent rendering, slow loading, and cache misses. Most local open-source tools also require users to manage Python environments, adding complexity. AniStudio is designed to maintain consistent performance under heavy loads, allowing users to multitask traditional media creation with AI-assisted workflows, all in one seamless application.  

## Features
- Entity Component System for modular development with low technical debt
- Dockable Views to personalize your UI/UX experience
- Customizable ImGui Themes
- Image and video inference (StableDiffusion.cpp)
- Cross-platform (Windows, Linux)
- Included UI editor for ease of development

---

## Requirements

### **Windows**
- **Vulkan**  
  - Already included (or download from [here](https://vulkan.lunarg.com/sdk/home)).  
  - Verify with `vulkaninfo`.  

- **Python >= 3.10**  
  - Download from [here](https://www.python.org/).  
  - Ensure the installer adds Python to your PATH.  
  - Verify with `python --version`.  

- **CMake >= 3.20**  
  - Download from [here](https://cmake.org/download/).  
  - Verify with `cmake --version`.  

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

## Install
*cloning, installing and building will take a while. speeding up the build process using repo options is in the TODO*
```
git clone https://github.com/Fizzledorf/AniStudio.git
cd AniStudio
git submodule update --init --recursive

```
### Windows
After building, the .exe will be located in `./build/release` directory.
```
.\install.bat
.\build.bat
```
You can also build manually and specify which backend and featured you want to use (see manually building for details).


### Unix
After building, the .exe will be located in `./build/release` directory.

```
./install.sh
./build.sh
```
You can also build manually and specify which backend and featured you want to use (see manually building for details).

### Manual Building

The first thing you need is the app requirements.

Next you will need Conan installed to a python virtual environment (venv) in the source code root:
```
  python -m venv venv
  pip install conan
  conan install . --build=missing -s compiler.cppstd=17
```

for selecting different inference backends, you need to use the appropriate flags.

### Main Build Options
| Command Flag             | Description                                                                     |
|--------------------------|---------------------------------------------------------------------------------|
| ```-DBUILD_ANISTUDIO=ON```| Configures and builds the main AniStudio Project                               |
| ```-DSD_CUBLAS=ON```      | Configures and builds all the plugin projects in the root `/plugins` directory |

#### Stable Diffusion Build Options

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

Windows:
```
mkdir build
cd build
cmake .. -DSD_VULKAN=ON -DSD_FLASH_ATTN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```
Linux:
```
mkdir build
cd build
cmake .. -DSD_VULKAN=ON -DSD_FLASH_ATTN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

# Building and Installing from Source

*Disclaimer: builds were only tested with MSVC*

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
*optional*
```
python -m venv venv
```
*make sure you activate the script before installation*

```
  pip install conan
  conan install . --build=missing -s compiler.cppstd=17
```

### Main Build Options
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

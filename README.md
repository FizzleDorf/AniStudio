# Anistudio

## Features
- Dockable Windows to personalize your UI/UX experience
- Customizable hotkeys
- Customizable ImGui Themes
- Image inference backends in Python (ComfyUI) and C++ (StableDiffusion.cpp)
- Fastest UI performance with Vulkan rendering
- Cross-platform (Windows, Linux, MacOS)
- Entity Component System for modular development with low technical debt

## Requirements
- Vulkan SDK
- Python
- CMake

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
for selecting different inference backends, you need to use the appropriate flags.

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
for launching with a vulkan backend with flash attention, the build commands would look like:
```
mkdir build
cd build
cmake .. -DSD_VULKAN=ON -DSD_FLASH_ATTN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```


## Connecting with Comfyui api
Run the app and in the `settings` menu, select the `ComfyUI` tab.

## Purpose

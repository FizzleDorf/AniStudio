# Anistudio

## Features
- Dockable Windows to personalize your UI/UX experience
- Customizable hotkeys
- Customizable ImGui Themes
- Image inference backends in Python (ComfyUI) and C++ (StableDiffusion.cpp)
- Fastest UI performance with Vulkan rendering
- Cross-platform (Windows, Linux, MacOS)
- Entity Component System for modular development with low technical debt

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

### Unix
After building, the .exe will be located in `./build/release` directory.

```
./install.sh
./build.sh
```

## Connecting with Comfyui api
Run the app and in the `settings` menu, select the `ComfyUI` tab.

## Purpose

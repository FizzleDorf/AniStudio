
# Building AniStudio on Linux (GCC)

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Automated Installation](#automated-installation)
3. [Building the Project](#building-the-project)
4. [Manual Build Process](#manual-build-process)
5. [Build Outputs](#build-outputs)
6. [Troubleshooting](#troubleshooting)

---

## Prerequisites
Before building, ensure your Linux system has the necessary development tools. These instructions are tailored for Debian/Ubuntu-based distributions:

* **GCC/G++**: C++17 compatible compiler (typically version 7 or higher).
* **CMake**: Version 3.12 or higher.
* **Python 3**: Required for the Conan package manager.
* **Ninja Build**: (Optional but recommended) used by the automated scripts.

Install the essentials using:
```bash
sudo apt update
sudo apt install build-essential cmake python3-full python3-venv ninja-build
```


---

## Automated Installation

The project provides an `install.sh` script to set up the environment and handle dependencies automatically:

1. Open a terminal in the project root directory.
2. Make the script executable:
```bash
chmod +x install.sh

```


3. Run the script:
```bash
./install.sh

```



The script will create a Python virtual environment in `build/venv`, install Conan, and fetch all C++ dependencies (OpenCV, GLEW, GLFW, FFmpeg, etc.).

---

## Building the Project

Use the `build.sh` script to handle the CMake configuration and compilation process.

**Standard Release Build:**

```bash
chmod +x build.sh
./build.sh

```

**Common Build Arguments:**

* `--debug`: Builds the project in Debug mode.
* `--shared`: Builds shared libraries (.so) instead of static libraries.
* `--jobs <num>`: Specifies the number of parallel threads (defaults to your CPU core count).
* `--clean`: Removes the existing build directory for a fresh start.

---

## Manual Build Process

If you prefer to run the commands manually:

### 1. Configuration

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON

```

### 2. Compilation

```bash
cmake --build . --config Release --parallel $(nproc)

```

---

## Build Outputs

Once the build is successful, the binaries and assets are organized in the `build` directory:

| Directory | Content |
| --- | --- |
| `build/bin/` | `AniStudio` executable |
| `build/lib/` | Shared or Static libraries (`AniEngineCore.so`, etc.) |
| `build/plugins/` | Compiled external plugins |
| `build/shaders/` | Copied vertex and fragment shaders |
| `build/data/` | Default configuration and data files |

---

## Troubleshooting

* **Missing System Headers**: If you encounter errors related to OpenGL, install the development headers:
`sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`
* **Conan Not Found**: The scripts use a local virtual environment. If running manually, ensure you have activated it: `source build/venv/bin/activate`.
* **Shared Library Errors**: If the application fails to start because it cannot find its own libraries, try:
`export LD_LIBRARY_PATH=$PWD/build/lib:$LD_LIBRARY_PATH`
* **Permissions**: Ensure both `.sh` files have execute permissions via `chmod +x`.

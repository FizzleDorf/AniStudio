# Building AniStudio on Windows (MSVC)

## Table of Contents

1. [Prerequisites](https://www.google.com/search?q=%23prerequisites)
2. [Automated Installation](https://www.google.com/search?q=%23automated-installation)
3. [Building the Project](https://www.google.com/search?q=%23building-the-project)
4. [Manual Build Process](https://www.google.com/search?q=%23manual-build-process)
5. [Build Outputs](https://www.google.com/search?q=%23build-outputs)
6. [Troubleshooting](https://www.google.com/search?q=%23troubleshooting)

---

## Prerequisites

Before building, ensure your Windows system has the necessary development tools installed:

* **Visual Studio 2019 or 2022**: Must include the "Desktop development with C++" workload.
* **CMake**: Version 3.12 or higher.
* **Python 3**: Required for the Conan package manager.
* **Git**: Required for cloning submodules and fetching dependencies.

---

## Automated Installation

The project provides an `install.bat` script to set up the environment and handle dependencies automatically:

1. Open a Command Prompt (cmd.exe) in the project root directory.
2. Run the script:
```batch
install.bat

```



The script will create a Python virtual environment in `build\venv`, install Conan, and fetch all C++ dependencies (OpenCV, GLEW, GLFW, FFmpeg, etc.) while setting the C++ standard to 17.

---

## Building the Project

Use the `build.bat` script to handle the CMake configuration and compilation process.

**Standard Release Build:**

```batch
build.bat

```

**Common Build Arguments:**

* `--debug`: Builds the project in Debug mode.
* `--shared`: Builds shared libraries (.dll) instead of static libraries (.lib).
* `--jobs <num>`: Specifies the number of parallel threads (defaults to 8).
* `--clean`: Removes the existing build directory for a fresh start.

---

## Manual Build Process

If you prefer to run the commands manually or use the Visual Studio IDE:

### 1. Configuration

```batch
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF

```

### 2. Compilation

```batch
cmake --build . --config Release --parallel 8

```

---

## Build Outputs

Once the build is successful, the binaries and assets are organized in the `build` directory:

| Directory | Content |
| --- | --- |
| `build/bin/` | `AniStudio.exe` executable and required DLLs |
| `build/lib/` | Static libraries (`AniEngineCore.lib`, `AniStudioCore.lib`) |
| `build/plugins/` | Compiled external plugins |
| `build/shaders/` | Copied vertex and fragment shaders |
| `build/data/` | Default configuration and data files |

---

## Troubleshooting

* **Python Not Found**: Ensure Python is added to your system PATH. The script requires it to create the virtual environment.
* **Conan Toolchain Errors**: If CMake cannot find the dependencies, verify that `install.bat` completed successfully and generated `build/conan/conan_toolchain.cmake`.
* **Missing Shaders**: Shaders are automatically copied during the build. If they are missing from the `bin` or `shaders` folder, try re-running the configuration step.
* **MSVC Runtime**: The project is configured to use `MultiThreaded$<$<CONFIG:Debug>:Debug>DLL`. Ensure your Visual Studio installation is up to date.

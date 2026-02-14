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

Before building the project, ensure the following software is installed on your Windows system:

* 
**Visual Studio 2019 or 2022**: Must include the "Desktop development with C++" workload.


* 
**CMake**: Version 3.12 or higher is required.


* 
**Python**: Required for managing dependencies via Conan.


* 
**Conan Package Manager**: Used for handling libraries like OpenCV, GLEW, and FFmpeg.



## Automated Installation

The project provides an `install.bat` script to set up the necessary environment:

1. Open a command prompt in the project root directory.
2. Run `install.bat`.


3. The script will:
* Create a virtual environment in `build\venv`.


* Upgrade pip and install Conan.


* Detect your MSVC profile and install all required dependencies using C++17 standards.





## Building the Project

Use the `build.bat` script to compile the application. This script handles the CMake configuration and the build process automatically.

**Standard Release Build:**

```batch
build.bat

```

**Common Build Arguments:**

* 
`--debug`: Builds the project in Debug mode instead of Release.


* 
`--shared`: Builds shared libraries (.dll) instead of static libraries (.lib).


* 
`--jobs <num>`: Specifies the number of parallel threads to use for compilation (default is 8).


* 
`--clean`: Removes the existing build directory for a fresh start.



## Manual Build Process

If you prefer to use the command line directly, follow these steps:

### 1. Configuration

```batch
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF

```

Note: The project requires C++17. On Windows, the build system automatically handles `-DNOMINMAX` and `_CRT_SECURE_NO_WARNINGS` definitions.

### 2. Compilation

```batch
cmake --build . --config Release --parallel 8

```

## Build Outputs

After a successful build, the following directories will contain the project artifacts:

* 
**Binaries**: Located in `build/bin/`, including `AniStudio.exe`.


* 
**Libraries**: Located in `build/lib/`, containing `AniEngineCore` and `AniStudioCore`.


* 
**Plugins**: Located in `build/plugins/`.


* 
**Assets**: Shaders are copied to `build/shaders/`, and data/scripts are located in their respective folders in the build directory.



## Troubleshooting

* 
**Conan Errors**: If the build fails to find dependencies, ensure you ran `install.bat` first to generate `conan_toolchain.cmake`.


* 
**Missing Editor Features**: If the Zep source is not found in `external/zep`, the text editor features will be limited.


* 
**Runtime Issues**: Ensure you are running the application from the `build` directory or that the `data` and `shaders` folders are present alongside the executable.



Would you like me to provide a list of all the specific third-party libraries that Conan installs for this project?

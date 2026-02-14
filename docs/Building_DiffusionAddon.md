# Building DiffusionAddon

## Table of Contents

1. [Important Requirement](https://www.google.com/search?q=%23important-requirement)
2. [Prerequisites](https://www.google.com/search?q=%23prerequisites)
3. [Windows Build Instructions (MSVC)](https://www.google.com/search?q=%23windows-build-instructions-msvc)
4. [Linux Build Instructions (GCC)](https://www.google.com/search?q=%23linux-build-instructions-gcc)
5. [Available Build Arguments](https://www.google.com/search?q=%23available-build-arguments)
6. [Build Outputs](https://www.google.com/search?q=%23build-outputs)
7. [Troubleshooting](https://www.google.com/search?q=%23troubleshooting)

---

## Important Requirement

**You must build the main AniStudio application with shared libraries enabled before building this addon.** The addon relies on the core engine's shared libraries (`AniEngineCore` and `AniStudioCore`). If you built the main app using static libraries, the addon will fail to link. Ensure you ran the main build with the `--shared` flag (or `BUILD_SHARED_LIBS=ON`).

---

## Prerequisites

* **Main Application**: Built and present in the parent directory.
* **Conan Toolchain**: The environment established by the main project's `install` script must be active or available.
* **Hardware SDKs**: If using hardware acceleration (CUDA, Vulkan, etc.), ensure the relevant SDKs are installed on your system.

---

## Windows Build Instructions (MSVC)

1. Open a **Command Prompt** (cmd.exe) in the `DiffusionAddon` directory.
2. Run the build script with your desired backend flags:
```batch
build.bat --cuda

```



## Linux Build Instructions (GCC)

1. Open a **Terminal** in the `DiffusionAddon` directory.
2. Ensure the script has execute permissions:
```bash
chmod +x build.sh

```


3. Run the build script with your desired backend flags:
```bash
./build.sh --vulkan

```



---

## Available Build Arguments

Both the Windows and Linux build scripts support the following arguments to configure hardware acceleration and build behavior:

| Argument | Description |
| --- | --- |
| `-clean` | Removes the `build` directory before starting a fresh compilation. |
| `--cuda` | Enables NVIDIA GPU acceleration (requires CUDA Toolkit). |
| `--vulkan` | Enables Vulkan-based GPU acceleration. |
| `--opencl` | Enables OpenCL-based hardware acceleration. |
| `--hipblas` | Enables AMD GPU acceleration via HIP/ROCm. |
| `--sycl` | Enables Intel GPU acceleration via SYCL. |
| `--metal` | Enables Apple Silicon acceleration (macOS only). |
| `--musa` | Enables Moore Threads MUSA acceleration. |
| `--fast-softmax` | Enables optimized softmax operations for performance. |

---

## Build Outputs

Once the build is successful, the output is organized into a staging area within the main project's build folder:

* **Windows**: `../../build/addons/DiffusionAddon/staging/DiffusionAddon.dll`
* **Linux**: `../../build/addons/DiffusionAddon/staging/DiffusionAddon.so`
* **Dependencies**: Required backend libraries (like `stable-diffusion.dll/so`) are also placed in the staging directory.

---

## Troubleshooting

* **Environment Conflicts (Linux)**: The `build.sh` script automatically purges Windows-related paths (common when building in WSL) to prevent cross-compilation errors. If headers are still not found, ensure your `CPATH` is correctly pointing to your Linux system headers.
* **Linker Errors**: If you get "Undefined Reference" errors related to `AniEngine`, double-check that the main application was built as a **shared library**.
* **Missing DLLs/Libs**: After building, ensure you copy any backend-specific libraries (like CUDA runtimes) to the same directory as the addon if they are not in your system path.

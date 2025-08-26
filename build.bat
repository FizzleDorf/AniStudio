@echo off
setlocal enabledelayedexpansion

:: Configuration
set BUILD_TYPE=Release
set PARALLEL_JOBS=8
set CLEAN_BUILD=false
set BUILD_SHARED=OFF
set ENABLE_CUDA=OFF
set ENABLE_VULKAN=OFF
set ENABLE_HIPBLAS=OFF
set ENABLE_METAL=OFF
set ENABLE_OPENCL=OFF
set ENABLE_SYCL=OFF
set ENABLE_MUSA=OFF
set ENABLE_FAST_SOFTMAX=OFF

:: Parse command line arguments
:parse_args
if "%~1"=="" goto :after_parse
if /i "%~1"=="--clean" (
    set CLEAN_BUILD=true
    shift
    goto :parse_args
)
if /i "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%~1"=="--shared" (
    set BUILD_SHARED=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--cuda" (
    set ENABLE_CUDA=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--vulkan" (
    set ENABLE_VULKAN=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--vk" (
    set ENABLE_VULKAN=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--hipblas" (
    set ENABLE_HIPBLAS=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--rocm" (
    set ENABLE_HIPBLAS=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--metal" (
    set ENABLE_METAL=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--opencl" (
    set ENABLE_OPENCL=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--sycl" (
    set ENABLE_SYCL=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--musa" (
    set ENABLE_MUSA=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--fast-softmax" (
    set ENABLE_FAST_SOFTMAX=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--jobs" (
    set PARALLEL_JOBS=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="--help" (
    goto :show_help
)
echo Unknown argument: %~1
goto :show_help

:after_parse

echo ============================================
echo AniStudio Optimized Build Script
echo ============================================
echo Build Type: %BUILD_TYPE%
echo Shared Libs: %BUILD_SHARED%
echo Parallel Jobs: %PARALLEL_JOBS%
echo Clean Build: %CLEAN_BUILD%
echo.
echo Stable Diffusion Backends:
echo   CUDA: %ENABLE_CUDA%
echo   Vulkan: %ENABLE_VULKAN%
echo   HipBLAS (AMD): %ENABLE_HIPBLAS%
echo   Metal (Apple): %ENABLE_METAL%
echo   OpenCL: %ENABLE_OPENCL%
echo   SYCL (Intel): %ENABLE_SYCL%
echo   MUSA: %ENABLE_MUSA%
echo   Fast Softmax: %ENABLE_FAST_SOFTMAX%
echo ============================================

:: Backend validation and warnings
if "%ENABLE_METAL%"=="ON" (
    if not "%OS%"=="Darwin" (
        echo WARNING: Metal backend is only supported on macOS
    )
)

if "%ENABLE_SYCL%"=="ON" (
    echo NOTE: SYCL requires Intel oneAPI Base toolkit
    echo       Run 'call "C:\Program Files (x86)\Intel\oneAPI\setvars.bat"' before building
)

if "%ENABLE_HIPBLAS%"=="ON" (
    echo NOTE: HipBLAS requires ROCm toolkit for AMD GPUs
)

if "%ENABLE_OPENCL%"=="ON" (
    echo NOTE: OpenCL requires OpenCL headers and ICD loader
)

if "%ENABLE_MUSA%"=="ON" (
    echo NOTE: MUSA requires MUSA toolkit for Moore Threads GPUs
)

:: Activate the virtual environment if it exists
if exist venv\Scripts\activate.bat (
    echo Activating virtual environment...
    call venv\Scripts\activate.bat
) else (
    echo No virtual environment found, continuing without activation...
)

:: Handle clean build
if "%CLEAN_BUILD%"=="true" (
    echo Performing clean build...
    if exist build (
        echo Removing build directory...
        rmdir /s /q build
    )
    
    :: Also clean any standalone library builds
    if exist src\aniengine\build rmdir /s /q src\aniengine\build
    if exist src\anistudio\build rmdir /s /q src\anistudio\build
    if exist src\aniplugins\build rmdir /s /q src\aniplugins\build
)

:: Create build directory if it doesn't exist
if not exist build (
    echo Creating build directory...
    mkdir build
)

cd build

:: Configure with CMake
echo ============================================
echo Configuring with CMake...
echo ============================================

cmake .. ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_SHARED_LIBS=%BUILD_SHARED% ^
    -DSD_CUDA=%ENABLE_CUDA% ^
    -DSD_VULKAN=%ENABLE_VULKAN% ^
    -DSD_HIPBLAS=%ENABLE_HIPBLAS% ^
    -DSD_METAL=%ENABLE_METAL% ^
    -DSD_OPENCL=%ENABLE_OPENCL% ^
    -DSD_SYCL=%ENABLE_SYCL% ^
    -DSD_MUSA=%ENABLE_MUSA% ^
    -DSD_FAST_SOFTMAX=%ENABLE_FAST_SOFTMAX% ^
    -DBUILD_ANIENGINE=ON ^
    -DBUILD_ANISTUDIO=ON ^
    -DBUILD_ANIPLUGINS=ON ^
    -DBUILD_MAIN_APP=ON

if errorlevel 1 (
    echo ============================================
    echo CMake configuration failed!
    echo ============================================
    goto :error_exit
)

:: Build the project
echo ============================================
echo Building project...
echo ============================================

cmake --build . --config %BUILD_TYPE% --parallel %PARALLEL_JOBS%

if errorlevel 1 (
    echo ============================================
    echo Build failed!
    echo ============================================
    goto :error_exit
)

echo ============================================
echo Build completed successfully!
echo ============================================
echo.
echo Binaries location: %CD%\bin
echo Libraries location: %CD%\lib
echo Plugins location: %CD%\plugins
echo.

:: Check if executable was created
if exist bin\AniStudio.exe (
    echo AniStudio executable: bin\AniStudio.exe
    
    :: Ask if user wants to run the application
    set /p RUN_APP="Run AniStudio now? (y/n): "
    if /i "!RUN_APP!"=="y" (
        echo Starting AniStudio...
        bin\AniStudio.exe
    )
) else (
    echo Warning: AniStudio.exe was not found in bin directory
)

goto :success_exit

:show_help
echo.
echo AniStudio Build Script Usage:
echo.
echo build.bat [options]
echo.
echo Build Options:
echo   --clean              Perform a clean build (removes build directory)
echo   --debug              Build in Debug mode (default: Release)
echo   --shared             Build shared libraries (default: static)
echo   --jobs ^<num^>          Number of parallel build jobs (default: 8)
echo.
echo Stable Diffusion Backend Options:
echo   --cuda               Enable CUDA support (NVIDIA GPUs)
echo   --vulkan / --vk      Enable Vulkan support (cross-platform)
echo   --hipblas / --rocm   Enable HipBLAS support (AMD GPUs, requires ROCm)
echo   --metal              Enable Metal support (Apple devices only)
echo   --opencl             Enable OpenCL support (requires OpenCL SDK)
echo   --sycl               Enable SYCL support (Intel GPUs, requires oneAPI)
echo   --musa               Enable MUSA support (Moore Threads GPUs)
echo   --fast-softmax       Enable fast softmax (CUDA/HipBLAS/MUSA only)
echo.
echo Examples:
echo   build.bat                           # Standard CPU-only build
echo   build.bat --cuda                    # CUDA-enabled build
echo   build.bat --vulkan                  # Vulkan-enabled build
echo   build.bat --cuda --vulkan           # Both CUDA and Vulkan
echo   build.bat --hipblas --fast-softmax  # AMD ROCm with fast softmax
echo   build.bat --clean --debug --sycl    # Clean debug build with Intel SYCL
echo   build.bat --jobs 16                 # Use 16 parallel jobs
echo.
echo Prerequisites by Backend:
echo   CUDA:     NVIDIA CUDA Toolkit
echo   Vulkan:   Vulkan SDK from LunarG
echo   HipBLAS:  AMD ROCm toolkit
echo   Metal:    Built into macOS (Apple devices only)
echo   OpenCL:   OpenCL headers + ICD loader
echo   SYCL:     Intel oneAPI Base toolkit
echo   MUSA:     Moore Threads MUSA toolkit
echo.
goto :end

:error_exit
cd ..
if exist venv\Scripts\activate.bat deactivate
echo.
echo Build failed! Check the output above for errors.
echo.
echo Common issues:
echo - Missing required SDK/toolkit for enabled backend
echo - Insufficient VRAM for multiple GPU backends
echo - Conflicting backend combinations
echo.
pause
exit /b 1

:success_exit
cd ..
if exist venv\Scripts\activate.bat deactivate
echo.
echo Build completed successfully!
pause
exit /b 0

:end
if exist venv\Scripts\activate.bat deactivate
exit /b 0
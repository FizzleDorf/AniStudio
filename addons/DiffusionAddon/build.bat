@echo off
setlocal enabledelayedexpansion

echo ================================
echo DiffusionAddon Build Script
echo ================================
echo.

set ADDON_DIR=%~dp0
set BUILD_DIR=%ADDON_DIR%build
set ROOT_DIR=%ADDON_DIR%..\..

set ADDON_NAME=DiffusionAddon
set BUILT_ADDONS_BASE=%ROOT_DIR%\build\addons
set ADDON_MAIN_DIR=%BUILT_ADDONS_BASE%\%ADDON_NAME%
set STAGING_DIR=%ADDON_MAIN_DIR%\staging
set STAGING_DLL=%STAGING_DIR%\%ADDON_NAME%.dll
set LIBS_DIR=%ADDON_MAIN_DIR%\libs

set SD_CUDA=OFF
set SD_VULKAN=OFF
set SD_HIPBLAS=OFF
set SD_METAL=OFF
set SD_OPENCL=OFF
set SD_SYCL=OFF
set SD_MUSA=OFF
set SD_FAST_SOFTMAX=OFF
set CLEAN_BUILD=0

:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="-clean" set CLEAN_BUILD=1
if /i "%~1"=="--cuda" set SD_CUDA=ON
if /i "%~1"=="--vulkan" set SD_VULKAN=ON
if /i "%~1"=="--hipblas" set SD_HIPBLAS=ON
if /i "%~1"=="--metal" set SD_METAL=ON
if /i "%~1"=="--opencl" set SD_OPENCL=ON
if /i "%~1"=="--sycl" set SD_SYCL=ON
if /i "%~1"=="--musa" set SD_MUSA=ON
if /i "%~1"=="--fast-softmax" set SD_FAST_SOFTMAX=ON
shift
goto parse_args
:end_parse

cd /d "%ADDON_DIR%"

echo Directory Configuration:
echo   Addon Name: %ADDON_NAME%
echo   Source Dir: %ADDON_DIR%
echo   AniStudio Root: %ROOT_DIR%
echo   Built Addons Base: %BUILT_ADDONS_BASE%
echo   Addon Main Dir: %ADDON_MAIN_DIR%
echo   Staging Dir: %STAGING_DIR%
echo   Libraries Dir: %LIBS_DIR%
echo   Build Target: %STAGING_DLL%
echo.
echo Backend Configuration:
echo   CUDA: %SD_CUDA%
echo   Vulkan: %SD_VULKAN%
echo   HipBLAS: %SD_HIPBLAS%
echo   Metal: %SD_METAL%
echo   OpenCL: %SD_OPENCL%
echo   SYCL: %SD_SYCL%
echo   MUSA: %SD_MUSA%
echo   Fast Softmax: %SD_FAST_SOFTMAX%
echo.

if not exist "%ROOT_DIR%\build\lib\AniEngineCore.lib" (
    echo ERROR: Main project not built yet!
    echo Please build the main AniStudio project first.
    echo Expected: %ROOT_DIR%\build\lib\AniEngineCore.lib
    pause
    exit /b 1
)

echo [OK] Main project libraries found

if %CLEAN_BUILD%==1 (
    if exist "%BUILD_DIR%" (
        echo Cleaning old build directory...
        rmdir /s /q "%BUILD_DIR%"
    )
)

mkdir "%BUILD_DIR%" 2>nul
mkdir "%BUILT_ADDONS_BASE%" 2>nul
mkdir "%ADDON_MAIN_DIR%" 2>nul
mkdir "%STAGING_DIR%" 2>nul
mkdir "%LIBS_DIR%" 2>nul

cd /d "%BUILD_DIR%"

if not exist "CMakeCache.txt" (
    echo Configuring CMake...
    
    set CMAKE_CMD=cmake .. -DCMAKE_BUILD_TYPE=Release
    set CMAKE_CMD=!CMAKE_CMD! -DSD_CUDA=%SD_CUDA%
    set CMAKE_CMD=!CMAKE_CMD! -DSD_VULKAN=%SD_VULKAN%
    set CMAKE_CMD=!CMAKE_CMD! -DSD_HIPBLAS=%SD_HIPBLAS%
    set CMAKE_CMD=!CMAKE_CMD! -DSD_METAL=%SD_METAL%
    set CMAKE_CMD=!CMAKE_CMD! -DSD_OPENCL=%SD_OPENCL%
    set CMAKE_CMD=!CMAKE_CMD! -DSD_SYCL=%SD_SYCL%
    set CMAKE_CMD=!CMAKE_CMD! -DSD_MUSA=%SD_MUSA%
    set CMAKE_CMD=!CMAKE_CMD! -DSD_FAST_SOFTMAX=%SD_FAST_SOFTMAX%
    
    echo Running: !CMAKE_CMD!
    echo.
    !CMAKE_CMD!
    
    if !errorlevel! neq 0 (
        echo ERROR: CMake configuration failed!
        pause
        exit /b 1
    )
    echo [OK] CMake configuration successful
) else (
    echo [OK] Using existing CMake configuration
    echo Note: To reconfigure with different backends, use -clean flag
)

echo Building addon...
cmake --build . --config Release --target %ADDON_NAME% 
if !errorlevel! neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo [OK] Addon build successful

if exist "%STAGING_DLL%" (
    echo [OK] Addon DLL created: %STAGING_DLL%
    for %%I in ("%STAGING_DLL%") do echo Size: %%~zI bytes
) else (
    echo ERROR: Addon DLL not found!
    echo Expected: %STAGING_DLL%
    pause
    exit /b 1
)

echo.
echo ================================
echo BUILD COMPLETED SUCCESSFULLY!
echo ================================
echo.
echo Addon DLL: %STAGING_DLL%
echo.

echo Checking for backend libraries...
set LIBS_NEEDED=0

if "%SD_CUDA%"=="ON" (
    echo   CUDA enabled - ensure CUDA runtime libraries are available
    set LIBS_NEEDED=1
)

if "%SD_VULKAN%"=="ON" (
    echo   Vulkan enabled - ensure Vulkan SDK libraries are available
    set LIBS_NEEDED=1
)

if "%SD_OPENCL%"=="ON" (
    echo   OpenCL enabled - ensure OpenCL runtime is available
    set LIBS_NEEDED=1
)

if %LIBS_NEEDED%==1 (
    echo.
    echo IMPORTANT: Ensure all required backend libraries are available at runtime!
    echo Place stable-diffusion.dll and backend-specific DLLs in:
    echo   - %STAGING_DIR%
    echo   - %ROOT_DIR%\build\bin
)

echo.
echo Usage:
echo   build.bat [-clean] [--cuda] [--vulkan] [--opencl] [--hipblas] [--metal]
echo.
echo Examples:
echo   build.bat --cuda              Build with CUDA support
echo   build.bat --vulkan --opencl   Build with Vulkan and OpenCL
echo   build.bat -clean --cuda       Clean rebuild with CUDA
echo.

pause
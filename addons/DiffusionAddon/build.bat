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

REM Check if main project is built
if not exist "%ROOT_DIR%\build\lib\AniEngineCore.lib" (
    echo ERROR: Main project not built yet!
    echo Please build the main AniStudio project first.
    echo Expected: %ROOT_DIR%\build\lib\AniEngineCore.lib
    pause
    exit /b 1
)

echo [OK] Main project libraries found

REM Clean old build if requested
if "%1"=="-clean" (
    if exist "%BUILD_DIR%" (
        echo Cleaning old build directory...
        rmdir /s /q "%BUILD_DIR%"
    )
)

REM Create directories
mkdir "%BUILD_DIR%" 2>nul
mkdir "%BUILT_ADDONS_BASE%" 2>nul
mkdir "%ADDON_MAIN_DIR%" 2>nul
mkdir "%STAGING_DIR%" 2>nul
mkdir "%LIBS_DIR%" 2>nul

cd /d "%BUILD_DIR%"

REM Configure CMake
if not exist "CMakeCache.txt" (
    echo Configuring CMake...
    cmake .. -DCMAKE_BUILD_TYPE=Release
    if %errorlevel% neq 0 (
        echo ERROR: CMake configuration failed!
        pause
        exit /b 1
    )
    echo [OK] CMake configuration successful
) else (
    echo [OK] Using existing CMake configuration
)

REM Build the addon
echo Building addon...
cmake --build . --config Release --target %ADDON_NAME%
if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo [OK] Addon build successful

REM Check if DLL was created
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
echo Important: Download SDCPP libraries to: %LIBS_DIR%
echo   - stable-diffusion.dll
echo   - ggml.dll
echo.
echo The addon will dynamically load these at runtime.
echo.

pause
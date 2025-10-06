@echo off
setlocal enabledelayedexpansion

echo ================================
echo Example Plugin Build Script
echo ================================
echo.

set PLUGIN_DIR=%~dp0
set BUILD_DIR=%PLUGIN_DIR%build
set ROOT_DIR=%PLUGIN_DIR%..\..

set PLUGIN_NAME=ExamplePlugin
set BUILT_PLUGINS_BASE=%ROOT_DIR%\build\plugins
set PLUGIN_MAIN_DIR=%BUILT_PLUGINS_BASE%\%PLUGIN_NAME%
set STAGING_DIR=%PLUGIN_MAIN_DIR%\staging
set STAGING_DLL=%STAGING_DIR%\%PLUGIN_NAME%.dll

cd /d "%PLUGIN_DIR%"

echo Directory Configuration:
echo   Plugin Name: %PLUGIN_NAME%
echo   Source Dir: %PLUGIN_DIR%
echo   AniStudio Root: %ROOT_DIR%
echo   Build Target: %STAGING_DLL%
echo.

REM Check if main project is built
if not exist "%ROOT_DIR%\build\lib\AniStudioCore.lib" (
    echo ERROR: Main project not built yet!
    echo Please build the main AniStudio project first.
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
mkdir "%BUILT_PLUGINS_BASE%" 2>nul
mkdir "%PLUGIN_MAIN_DIR%" 2>nul
mkdir "%STAGING_DIR%" 2>nul

cd /d "%BUILD_DIR%"

REM Configure CMake - let it auto-detect generator
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

REM Build the plugin
echo Building plugin...
cmake --build . --config Release --target %PLUGIN_NAME%
if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo [OK] Plugin build successful

REM Check if DLL was created
if exist "%STAGING_DLL%" (
    echo [OK] Plugin DLL created: %STAGING_DLL%
    for %%I in ("%STAGING_DLL%") do echo Size: %%~zI bytes
) else (
    echo ERROR: Plugin DLL not found!
    pause
    exit /b 1
)

echo.
echo ================================
echo BUILD COMPLETED SUCCESSFULLY!
echo ================================
pause
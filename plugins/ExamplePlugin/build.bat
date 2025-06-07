@echo off
setlocal enabledelayedexpansion

REM Build script for AniStudio plugins
REM This script builds plugins against the AniStudio core library

echo === Building ExamplePlugin ===

REM Get paths relative to this script - FIXED PATH CALCULATION
set "SCRIPT_DIR=%~dp0"
REM Remove trailing backslash
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Calculate AniStudio root (go up two levels from plugins/ExamplePlugin)
for %%I in ("%SCRIPT_DIR%\..\..\") do set "ANISTUDIO_ROOT=%%~fI"
REM Remove trailing backslash
set "ANISTUDIO_ROOT=%ANISTUDIO_ROOT:~0,-1%"

set "ANISTUDIO_BUILD=%ANISTUDIO_ROOT%\build"
set "PLUGIN_SOURCE=%SCRIPT_DIR%"
set "PLUGIN_BUILD=%ANISTUDIO_BUILD%\plugins\ExamplePlugin"
set "PLUGIN_OUTPUT=%ANISTUDIO_BUILD%\plugins"

echo AniStudio root: %ANISTUDIO_ROOT%
echo AniStudio build: %ANISTUDIO_BUILD%
echo Plugin source: %PLUGIN_SOURCE%
echo Plugin build: %PLUGIN_BUILD%
echo Plugin output: %PLUGIN_OUTPUT%
echo.

REM Check if AniStudio has been built by looking for key files
set "BUILD_VALID=1"

echo === Checking AniStudio Build Status ===

if not exist "%ANISTUDIO_BUILD%" (
    echo ERROR: Build directory does not exist: %ANISTUDIO_BUILD%
    set "BUILD_VALID=0"
) else (
    echo ? Build directory exists
)

if not exist "%ANISTUDIO_BUILD%\conan\conan_toolchain.cmake" (
    echo ERROR: Conan toolchain not found: %ANISTUDIO_BUILD%\conan\conan_toolchain.cmake
    set "BUILD_VALID=0"
) else (
    echo ? Conan toolchain found
)

if not exist "%ANISTUDIO_BUILD%\lib\AniStudioCore.lib" (
    echo ERROR: AniStudioCore library not found: %ANISTUDIO_BUILD%\lib\AniStudioCore.lib
    set "BUILD_VALID=0"
) else (
    echo ? AniStudioCore library found
)

if not exist "%ANISTUDIO_BUILD%\lib\ImGui.lib" (
    echo ERROR: ImGui library not found: %ANISTUDIO_BUILD%\lib\ImGui.lib
    set "BUILD_VALID=0"
) else (
    echo ? ImGui library found
)

if not exist "%ANISTUDIO_BUILD%\bin\AniStudio.exe" (
    echo ERROR: AniStudio executable not found: %ANISTUDIO_BUILD%\bin\AniStudio.exe
    set "BUILD_VALID=0"
) else (
    echo ? AniStudio executable found
)

if "!BUILD_VALID!"=="0" (
    echo.
    echo ERROR: AniStudio build verification failed
    echo.
    echo Make sure you have built AniStudio first by running:
    echo   cd "%ANISTUDIO_ROOT%"
    echo   cmake --build build --config Release
    echo.
    echo Current build directory contents:
    if exist "%ANISTUDIO_BUILD%" (
        dir "%ANISTUDIO_BUILD%" /b
    ) else (
        echo   Build directory does not exist
    )
    echo.
    pause
    exit /b 1
)

echo ? All required AniStudio components found

REM Create plugin build directory
if not exist "%PLUGIN_BUILD%" mkdir "%PLUGIN_BUILD%"
if not exist "%PLUGIN_OUTPUT%" mkdir "%PLUGIN_OUTPUT%"

REM Configure the plugin
echo.
echo === Configuring Plugin ===
cd /d "%PLUGIN_BUILD%"

cmake "%PLUGIN_SOURCE%" ^
    -DANISTUDIO_BUILD_DIR="%ANISTUDIO_BUILD%" ^
    -DANISTUDIO_PLUGIN_DIR="%PLUGIN_OUTPUT%" ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo ERROR: Plugin configuration failed
    echo.
    echo CMake configuration output above may contain details about the error.
    echo.
    pause
    exit /b 1
)

echo ? Plugin configured successfully

REM Build the plugin
echo.
echo === Building Plugin ===
cmake --build . --config Release

if errorlevel 1 (
    echo ERROR: Plugin build failed
    echo.
    echo Build output above may contain details about the error.
    echo.
    pause
    exit /b 1
)

echo.
echo === Build Complete ===
echo.

REM Check if the plugin was built successfully
if exist "%PLUGIN_OUTPUT%\ExamplePlugin.dll" (
    echo ? Plugin built successfully!
    echo.
    echo Plugin details:
    dir "%PLUGIN_OUTPUT%\ExamplePlugin.dll"
    echo.
    echo To use this plugin:
    echo 1. Start AniStudio from: %ANISTUDIO_BUILD%\bin\AniStudio.exe
    echo 2. The plugin should be automatically loaded from: %PLUGIN_OUTPUT%
    echo 3. Look for "ExampleView" in the AniStudio interface
    echo 4. Check the plugin manager to verify it loaded successfully
) else (
    echo ERROR: Plugin DLL was not created
    echo.
    echo Expected location: %PLUGIN_OUTPUT%\ExamplePlugin.dll
    echo.
    echo Contents of plugin output directory:
    if exist "%PLUGIN_OUTPUT%" (
        dir "%PLUGIN_OUTPUT%" /b
    ) else (
        echo   Plugin output directory does not exist
    )
    echo.
    pause
    exit /b 1
)

echo.
pause
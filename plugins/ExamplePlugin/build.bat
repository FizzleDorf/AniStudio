@echo off
setlocal EnableDelayedExpansion

REM ExamplePlugin build script
REM Usage: build.bat [anistudio_build_dir] [build_type]

set ANISTUDIO_BUILD_DIR=%1
set BUILD_TYPE=%2

REM Default values
if "%ANISTUDIO_BUILD_DIR%"=="" set ANISTUDIO_BUILD_DIR=..\..\build
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

REM Get absolute paths
for %%I in ("%ANISTUDIO_BUILD_DIR%") do set ANISTUDIO_BUILD_DIR=%%~fI
for %%I in ("%~dp0") do set SCRIPT_DIR=%%~fI
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

echo === ExamplePlugin Build Script ===
echo Plugin directory: %SCRIPT_DIR%
echo AniStudio build dir: %ANISTUDIO_BUILD_DIR%
echo Build type: %BUILD_TYPE%
echo.

REM Check if AniStudio is built
if not exist "%ANISTUDIO_BUILD_DIR%\lib\AniStudioCore.lib" (
    echo Error: AniStudioCore library not found in '%ANISTUDIO_BUILD_DIR%\lib\'
    echo Please build AniStudio first!
    echo Expected file: %ANISTUDIO_BUILD_DIR%\lib\AniStudioCore.lib
    exit /b 1
)

REM Check if CMake config exists (try install directory first, then build directory)
if exist "%ANISTUDIO_BUILD_DIR%\install\lib\cmake\AniStudioCore\AniStudioCoreConfig.cmake" (
    echo Found AniStudioCore config in install directory
) else if exist "%ANISTUDIO_BUILD_DIR%\lib\cmake\AniStudioCore\AniStudioCoreConfig.cmake" (
    echo Found AniStudioCore config in build directory
) else (
    echo Error: AniStudioCore CMake config not found!
    echo Checked:
    echo   %ANISTUDIO_BUILD_DIR%\install\lib\cmake\AniStudioCore\AniStudioCoreConfig.cmake
    echo   %ANISTUDIO_BUILD_DIR%\lib\cmake\AniStudioCore\AniStudioCoreConfig.cmake
    echo Please run 'cmake --install . --prefix ./install' from the AniStudio build directory
    exit /b 1
)

REM Create build directory
set BUILD_DIR=%SCRIPT_DIR%\build
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo Cleaning previous build...
if exist "%BUILD_DIR%\*" (
    del /q "%BUILD_DIR%\*" >nul 2>&1
    for /d %%i in ("%BUILD_DIR%\*") do rmdir /s /q "%%i" >nul 2>&1
)

REM Configure CMake
echo Configuring CMake...
cmake -B "%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DANISTUDIO_BUILD_DIR="%ANISTUDIO_BUILD_DIR%" ^
    -DANISTUDIO_PLUGIN_DIR="%ANISTUDIO_BUILD_DIR%\plugins" ^
    "%SCRIPT_DIR%"

if errorlevel 1 (
    echo Error: CMake configuration failed!
    exit /b 1
)

REM Build the plugin
echo Building ExamplePlugin...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE%

if errorlevel 1 (
    echo Error: Plugin build failed!
    exit /b 1
)

echo.
echo === Build Successful ===
set OUTPUT_DIR=%ANISTUDIO_BUILD_DIR%\plugins

REM List the built plugin file
if exist "%OUTPUT_DIR%\ExamplePlugin.dll" (
    echo Built: %OUTPUT_DIR%\ExamplePlugin.dll
    dir "%OUTPUT_DIR%\ExamplePlugin.dll"
) else (
    echo Warning: Plugin file not found in expected location: %OUTPUT_DIR%
    echo Checking build directory for plugin files...
    dir /b /s "%BUILD_DIR%\*.dll" 2>nul | findstr /i "ExamplePlugin"
    if errorlevel 1 (
        echo No ExamplePlugin.dll found in build directory
        dir /b /s "%BUILD_DIR%\*.dll" 2>nul
    )
)

echo.
echo To use this plugin:
echo 1. Start AniStudio
echo 2. The plugin should be automatically loaded from: %OUTPUT_DIR%
echo 3. Check the plugin manager to verify it loaded successfully

endlocal
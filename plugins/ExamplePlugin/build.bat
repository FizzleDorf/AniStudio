@echo off
echo Building ExamplePlugin...

REM Set paths
set PLUGIN_DIR=%~dp0
set ANISTUDIO_ROOT=%PLUGIN_DIR%\..\..
set BUILD_DIR=%PLUGIN_DIR%\build
set PLUGINS_OUTPUT_DIR=%ANISTUDIO_ROOT%\build\plugins

echo Plugin directory: %PLUGIN_DIR%
echo AniStudio root: %ANISTUDIO_ROOT%
echo Build directory: %BUILD_DIR%
echo Output directory: %PLUGINS_OUTPUT_DIR%

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Change to build directory
cd /d "%BUILD_DIR%"

REM Configure - just use the fucking current directory
echo Configuring...
cmake .. -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!

REM Copy the DLL manually to the plugins directory
if not exist "%PLUGINS_OUTPUT_DIR%" mkdir "%PLUGINS_OUTPUT_DIR%"

if exist "ExamplePlugin.dll" (
    copy "ExamplePlugin.dll" "%PLUGINS_OUTPUT_DIR%\"
    echo Plugin copied to: %PLUGINS_OUTPUT_DIR%\ExamplePlugin.dll
) else if exist "Release\ExamplePlugin.dll" (
    copy "Release\ExamplePlugin.dll" "%PLUGINS_OUTPUT_DIR%\"
    echo Plugin copied to: %PLUGINS_OUTPUT_DIR%\ExamplePlugin.dll
) else (
    echo Plugin DLL not found in build directory
    echo Contents of build directory:
    dir /b
)

pause
@echo off
echo ========================================
echo Building ExamplePlugin (Fixed Version)
echo ========================================

REM Check if we're in the right directory
if not exist "ExamplePlugin.cpp" (
    echo ERROR: ExamplePlugin.cpp not found!
    echo Make sure you're running this from the plugin directory.
    pause
    exit /b 1
)

REM Check for PluginAPI.hpp
if not exist "../../include/PluginAPI.hpp" (
    echo ERROR: PluginAPI.hpp not found at ../../include/PluginAPI.hpp
    echo Please make sure the PluginAPI.hpp is in the correct location.
    pause
    exit /b 1
)

echo Found PluginAPI.hpp at: ../../include/PluginAPI.hpp
echo.

REM Create build directory
if not exist "build" mkdir build
cd build

echo ========================================
echo Configuring CMake...
echo ========================================

REM Configure with minimal dependencies
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo ========================================
echo Building plugin...
echo ========================================

REM Build the plugin
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================

REM The DLL should be output directly to the correct location by CMake
echo Plugin should be available at: ../../build/plugins/ExamplePlugin/ExamplePlugin.dll
echo.

REM Verify the plugin was created in the right place
if exist "../../build/plugins/ExamplePlugin/ExamplePlugin.dll" (
    echo SUCCESS: Plugin DLL found at correct location!
    echo Ready to load in AniStudio Plugin Manager.
) else (
    echo WARNING: Plugin DLL not found at expected location.
    echo Check CMake output above for any issues.
)

cd ..
echo.
echo Done! Press any key to exit.
pause >nul
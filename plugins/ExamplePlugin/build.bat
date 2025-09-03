@echo off
echo ================================
echo Example Plugin Build Script
echo ================================
echo.

set PLUGIN_DIR=%~dp0
set BUILD_DIR=%PLUGIN_DIR%build
set ROOT_DIR=%PLUGIN_DIR%..\..\

cd /d "%PLUGIN_DIR%"

REM Check if main project is built
if not exist "%ROOT_DIR%build\lib\AniStudioCore.lib" (
    echo ERROR: Main project not built yet!
    echo Please build the main AniStudio project first.
    echo Expected: %ROOT_DIR%build\lib\AniStudioCore.lib
    pause
    exit /b 1
)

echo [OK] Main project libraries found

REM Clean old build
if exist "%BUILD_DIR%" (
    echo Cleaning old build directory...
    rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Configure CMake
echo Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo [OK] CMake configuration successful

REM Build the plugin
echo Building plugin...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo [OK] Plugin build successful

REM Check if DLL was created
set "EXPECTED_DLL=%ROOT_DIR%build\plugins\ExamplePlugin.dll"
set "ACTUAL_DLL=%PLUGIN_DIR%build\plugins\ExamplePlugin.dll"

if exist "%EXPECTED_DLL%" (
    echo [OK] Plugin DLL created successfully
    echo Location: %EXPECTED_DLL%
    
    REM Get file size
    for %%I in ("%EXPECTED_DLL%") do echo Size: %%~zI bytes
    
) else if exist "%ACTUAL_DLL%" (
    echo [WARNING] Plugin DLL created in local build directory
    echo Location: %ACTUAL_DLL%
    echo This indicates CMake output directory configuration needs fixing.
    
    REM Get file size
    for %%I in ("%ACTUAL_DLL%") do echo Size: %%~zI bytes
    
    REM Copy to expected location
    echo Copying to main plugins directory...
    copy "%ACTUAL_DLL%" "%EXPECTED_DLL%"
    if %errorlevel% == 0 (
        echo [OK] DLL copied to main plugins directory
    )
    
) else (
    echo ERROR: Plugin DLL not found!
    echo Expected: %EXPECTED_DLL%
    echo Alternative: %ACTUAL_DLL%
    echo.
    echo Checking build directories:
    echo Main build:
    if exist "%ROOT_DIR%build\" dir /s "%ROOT_DIR%build\*.dll"
    echo Plugin build:
    if exist "%BUILD_DIR%\" dir /s "%BUILD_DIR%\*.dll"
    pause
    exit /b 1
)

echo.
echo ================================
echo Plugin build completed successfully!
echo ================================

REM Optional: Run diagnostics
choice /c yn /m "Run plugin diagnostics? (y/n)"
if %errorlevel% == 1 (
    echo.
    echo Running plugin diagnostics...
    
    REM Check exports with dumpbin if available
    where dumpbin >nul 2>&1
    if %errorlevel% == 0 (
        echo.
        echo Checking exports:
        dumpbin /exports "%ROOT_DIR%build\plugins\ExamplePlugin.dll"
        echo.
        echo Checking dependencies:  
        dumpbin /dependents "%ROOT_DIR%build\plugins\ExamplePlugin.dll"
    ) else (
        echo dumpbin not available - install Visual Studio Build Tools for detailed analysis
    )
)

pause
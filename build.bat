@echo off
setlocal enabledelayedexpansion

:: Configuration
set BUILD_TYPE=Release
set PARALLEL_JOBS=8
set CLEAN_BUILD=false
set BUILD_SHARED=OFF
set ENABLE_CUDA=OFF

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
echo CUDA Support: %ENABLE_CUDA%
echo Parallel Jobs: %PARALLEL_JOBS%
echo Clean Build: %CLEAN_BUILD%
echo ============================================

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
    -DSD_VULKAN=ON ^
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
echo Options:
echo   --clean         Perform a clean build (removes build directory)
echo   --debug         Build in Debug mode (default: Release)
echo   --shared        Build shared libraries (default: static)
echo   --cuda          Enable CUDA support (default: disabled)
echo   --jobs ^<num^>     Number of parallel build jobs (default: 8)
echo   --help          Show this help message
echo.
echo Examples:
echo   build.bat                    # Standard release build
echo   build.bat --clean --debug   # Clean debug build
echo   build.bat --shared --cuda   # Shared libraries with CUDA
echo   build.bat --jobs 16         # Use 16 parallel jobs
echo.
goto :end

:error_exit
cd ..
if exist venv\Scripts\activate.bat deactivate
echo.
echo Build failed! Check the output above for errors.
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
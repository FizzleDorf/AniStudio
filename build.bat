@echo off
setlocal enabledelayedexpansion

:: Configuration
set BUILD_TYPE=Release
set PARALLEL_JOBS=8
set BUILD_SHARED=OFF

:: Parse command line arguments
:parse_args
if "%~1"=="" goto :after_parse
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
echo ============================================

:: Activate the virtual environment if it exists
if exist venv\Scripts\activate.bat (
    echo Activating virtual environment...
    call venv\Scripts\activate.bat
) else (
    echo No virtual environment found, continuing without activation...
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
    echo AniStudio executable: AniStudio.exe
    
    :: Ask if user wants to run the application
    set /p RUN_APP="Run AniStudio now? (y/n): "
    if /i "!RUN_APP!"=="y" (
        echo Starting AniStudio...
        bin\AniStudio.exe
    )
) else (
    echo Warning: AniStudio.exe was not found
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
echo Examples:
echo   build.bat                           # Standard CPU-only build
echo   build.bat --clean --debug --sycl    # Clean debug build with Intel SYCL
echo   build.bat --jobs 16                 # Use 16 parallel jobs
echo.
goto :end

:error_exit
cd ..
if exist venv\Scripts\activate.bat deactivate
echo.
echo Build failed! Check the output above for errors.
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
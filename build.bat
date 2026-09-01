@echo off
setlocal

:: Defaults
set BUILD_TYPE=Release
set JOBS=8
set CLEAN=0
set SHARED=OFF

:: Parse arguments (simple and safe)
:parse
if "%1"=="" goto :build
if /i "%1"=="--debug" set BUILD_TYPE=Debug & shift & goto :parse
if /i "%1"=="--clean" set CLEAN=1 & shift & goto :parse
if /i "%1"=="--shared" set SHARED=ON & shift & goto :parse
if /i "%1"=="--jobs" set JOBS=%2 & shift & shift & goto :parse
if /i "%1"=="--help" goto :help
echo Unknown argument: %1
:help
echo Usage: build.bat [--debug] [--clean] [--shared] [--jobs N]
exit /b 0

:build
echo ==========================================
echo Building AniStudio
echo ==========================================
echo Build type: %BUILD_TYPE%
echo Shared libs: %SHARED%
echo Jobs: %JOBS%
echo Clean: %CLEAN%
echo ==========================================

:: Clean if requested
if %CLEAN%==1 (
    echo Removing build directory...
    rmdir /s /q build 2>nul
)

if not exist build mkdir build
cd build

:: Configure with CMake (NO Python flags)
echo.
echo Configuring with CMake...
echo.

cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_SHARED_LIBS=%SHARED% ^
    -DBUILD_ANIENGINE=ON ^
    -DBUILD_ANISTUDIO=ON ^
    -DBUILD_ANIPLUGINS=ON ^
    -DBUILD_MAIN_APP=ON

if errorlevel 1 (
    echo.
    echo CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

:: Build
echo.
echo Building...
echo.

cmake --build . --config %BUILD_TYPE% --parallel %JOBS%

if errorlevel 1 (
    echo.
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo ==========================================
echo Build succeeded!
echo ==========================================
echo Binaries: %CD%
echo Libraries: %CD%\lib
echo.

cd ..
pause
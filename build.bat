@echo off
setlocal EnableDelayedExpansion

:: ------------------------------------------------------------
:: Defaults
:: ------------------------------------------------------------
set BUILD_TYPE=Release
set JOBS=8
set CLEAN=0
set SHARED=ON
set APP_MODE=local
set BUILD_DIR=build

:: ------------------------------------------------------------
:: Parse arguments
:: ------------------------------------------------------------
:parse
if "%~1"=="" goto :build

if /i "%~1"=="--debug"    set BUILD_TYPE=Debug & shift & goto :parse
if /i "%~1"=="--release"  set BUILD_TYPE=Release & shift & goto :parse
if /i "%~1"=="--clean"    set CLEAN=1 & shift & goto :parse
if /i "%~1"=="--shared"   set SHARED=ON & shift & goto :parse
if /i "%~1"=="--static"   set SHARED=OFF & shift & goto :parse
if /i "%~1"=="--jobs"     set JOBS=%~2 & shift & shift & goto :parse

if /i "%~1"=="--local"    set APP_MODE=local & shift & goto :parse
if /i "%~1"=="--server"   set APP_MODE=server & shift & goto :parse
if /i "%~1"=="--client"   set APP_MODE=client & shift & goto :parse
if /i "%~1"=="--both"     set APP_MODE=both & shift & goto :parse

if /i "%~1"=="--build-dir" set BUILD_DIR=%~2 & shift & shift & goto :parse

if /i "%~1"=="--help"     goto :help
if /i "%~1"=="-h"         goto :help

echo Unknown argument: %~1
goto :help

:help
echo Usage: build.bat [options]
echo.
echo Build type:
echo   --debug               Build in Debug mode        (default: Release)
echo   --release             Build in Release mode
echo.
echo Build mode:
echo   --local               Build the local desktop app  (default)
echo   --server              Build headless AniServer
echo   --client              Build networked AniClient
echo   --both                Build AniServer AND AniClient
echo.
echo Library linkage:
echo   --shared              Build shared libraries (.dll/.so)  (default)
echo   --static              Build static libraries
echo.
echo Other:
echo   --clean               Remove build directory before configuring
echo   --jobs N              Parallel build jobs (default: 8)
echo   --build-dir DIR       Use DIR as build directory (default: build)
echo   --help, -h            Show this help
echo.
echo Examples:
echo   build.bat --local
echo   build.bat --server --debug
echo   build.bat --client --release --jobs 16
echo   build.bat --both --clean
exit /b 0

:: ------------------------------------------------------------
:: Build
:: ------------------------------------------------------------
:build
echo ==========================================
echo Building AniStudio
echo ==========================================
echo Build type: %BUILD_TYPE%
echo Build mode: %APP_MODE%
echo Shared libs: %SHARED%
echo Jobs: %JOBS%
echo Clean: %CLEAN%
echo Build dir: %BUILD_DIR%
echo ==========================================

:: Clean if requested
if %CLEAN%==1 (
    echo Removing %BUILD_DIR% directory...
    rmdir /s /q "%BUILD_DIR%" 2>nul
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
pushd "%BUILD_DIR%"

:: Configure with CMake
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
    -DBUILD_MAIN_APP=ON ^
    -DBUILD_APP_MODE=%APP_MODE%

if errorlevel 1 (
    echo.
    echo CMake configuration failed!
    popd
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
    popd
    pause
    exit /b 1
)

echo.
echo ==========================================
echo Build succeeded!
echo ==========================================

if /i "%APP_MODE%"=="local" (
    echo Binary: %CD%\AniStudio.exe
)
if /i "%APP_MODE%"=="server" (
    echo Binary: %CD%\AniServer.exe
)
if /i "%APP_MODE%"=="client" (
    echo Binary: %CD%\AniClient.exe
)
if /i "%APP_MODE%"=="both" (
    echo Binaries: %CD%\AniServer.exe
    echo           %CD%\AniClient.exe
)
echo Libraries: %CD%\lib
echo.

popd

:: ------------------------------------------------------------
:: Quick launch prompt
:: ------------------------------------------------------------
if /i "%APP_MODE%"=="server" (
    echo Launch the server? [y/N]
    set /p LAUNCH=
    if /i "!LAUNCH!"=="y" (
        start "AniServer" "%BUILD_DIR%\AniServer.exe" 9000
    )
)
if /i "%APP_MODE%"=="both" (
    echo Launch the server? [y/N]
    set /p LAUNCH=
    if /i "!LAUNCH!"=="y" (
        start "AniServer" "%BUILD_DIR%\AniServer.exe" 9000
    )
)

pause
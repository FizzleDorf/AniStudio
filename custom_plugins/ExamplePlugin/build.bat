@echo off
echo ================================
echo Example Plugin Build Script (Fixed Paths)
echo ================================
echo.

set PLUGIN_DIR=%~dp0
set BUILD_DIR=%PLUGIN_DIR%build
REM FIXED: Remove trailing backslash and space that was breaking paths
set ROOT_DIR=%PLUGIN_DIR%..\..

REM FIXED: Plugin builds to build directory with proper path concatenation
set PLUGIN_NAME=ExamplePlugin
set BUILT_PLUGINS_BASE=%ROOT_DIR%\build\plugins
set PLUGIN_MAIN_DIR=%BUILT_PLUGINS_BASE%\%PLUGIN_NAME%
set STAGING_DIR=%PLUGIN_MAIN_DIR%\staging
set STAGING_DLL=%STAGING_DIR%\%PLUGIN_NAME%.dll

cd /d "%PLUGIN_DIR%"

echo Fixed Directory Configuration:
echo   Plugin Name: %PLUGIN_NAME%
echo   Source Dir: %PLUGIN_DIR%
echo   AniStudio Root: %ROOT_DIR%
echo   Built Plugins Base: %BUILT_PLUGINS_BASE%
echo   Plugin Main Dir: %PLUGIN_MAIN_DIR%
echo   Staging Dir: %STAGING_DIR%
echo   Build Target: %STAGING_DLL%
echo   Versioned DLLs: %PLUGIN_MAIN_DIR%\%PLUGIN_NAME%_v*.dll
echo.

REM Check if main project is built - FIXED PATH
if not exist "%ROOT_DIR%\build\lib\AniStudioCore.lib" (
    echo ERROR: Main project not built yet!
    echo Please build the main AniStudio project first.
    echo Expected: %ROOT_DIR%\build\lib\AniStudioCore.lib
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

REM Configure CMake
if not exist "CMakeCache.txt" (
    echo Configuring CMake...
    cmake .. -G "Visual Studio 17 2022" -A x64
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

REM Check if DLL was created in STAGING
if exist "%STAGING_DLL%" (
    echo [OK] Plugin DLL created in CORRECT STAGING LOCATION
    echo Location: %STAGING_DLL%
    
    REM Get file info
    for %%I in ("%STAGING_DLL%") do (
        echo Size: %%~zI bytes
        echo Modified: %%~tI
    )
    
    echo.
    echo ================================
    echo VERSIONED HOT RELOAD SUCCESS
    echo ================================
    
    REM Check for existing versioned DLLs
    set "VERSIONED_COUNT=0"
    set "HIGHEST_VERSION=0"
    
    for %%F in ("%PLUGIN_MAIN_DIR%\%PLUGIN_NAME%_v*.dll") do (
        set /a VERSIONED_COUNT+=1
        
        REM Extract version number
        set "FILENAME=%%~nF"
        for /f "tokens=2 delims=v" %%V in ("!FILENAME!") do (
            if %%V GTR !HIGHEST_VERSION! set "HIGHEST_VERSION=%%V"
        )
    )
    
    if %VERSIONED_COUNT% GTR 0 (
        echo Found %VERSIONED_COUNT% existing versioned DLL(s)
        echo Highest version: v%HIGHEST_VERSION%
        echo.
        echo STATUS: New DLL ready in staging!
        echo Hot reload will create: %PLUGIN_NAME%_v!HIGHEST_VERSION!+1.dll
    ) else (
        echo No existing versioned DLLs found
        echo.
        echo STATUS: First build ready in staging!
        echo Hot reload will create: %PLUGIN_NAME%_v1.dll
    )
    
    echo.
    echo Hot Reload Flow:
    echo 1. Build creates DLL in staging
    echo 2. PluginManager detects new DLL
    echo 3. Creates new versioned DLL
    echo 4. Preserves registrations (no re-opening views!)
    echo 5. Auto-cleans old versions
    
) else (
    echo ERROR: Plugin DLL not found in staging!
    echo Expected: %STAGING_DLL%
    echo.
    echo Checking build output:
    if exist "%BUILD_DIR%" (
        dir /s "%BUILD_DIR%\*.dll" 2>nul
    )
    pause
    exit /b 1
)

goto :success

:success
echo.
echo ================================
echo BUILD COMPLETED SUCCESSFULLY!
echo ================================
echo.
echo Summary:
echo   Build Target: %STAGING_DLL%
echo   Plugin will be detected in: %PLUGIN_MAIN_DIR%
echo.
echo Next Steps:
echo - Load plugin via PluginView in application
echo - Hot reload will happen automatically on next build
echo - No need to re-open views after hot reload!
echo.

pause
@echo off
echo Building ExamplePlugin...

REM Set paths
set PLUGIN_DIR=%~dp0
set ANISTUDIO_ROOT=%PLUGIN_DIR%\..\..
set BUILD_DIR=%ANISTUDIO_ROOT%\build\plugins\ExamplePlugin
set STAGING_DIR=%BUILD_DIR%\staging

REM Create build and staging directories
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%STAGING_DIR%" mkdir "%STAGING_DIR%"

REM Change to build directory
cd /d "%BUILD_DIR%"

REM SIMPLE CMAKE COMMAND
cmake "%PLUGIN_DIR%"

REM Build
cmake --build . --config Release

REM Copy the DLL to staging for hot reload
copy /Y "Release\*.dll" "%STAGING_DIR%\"

echo Done! DLL copied to staging: %STAGING_DIR%
echo PluginManager will hot reload from staging to active directory
pause
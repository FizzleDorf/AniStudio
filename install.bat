@echo off
setlocal enabledelayedexpansion

:: Find a real Python (not WindowsApps)
set PYTHON_EXE=
for /f "delims=" %%i in ('where python 2^>nul') do (
    echo %%i | findstr /i "WindowsApps" >nul
    if errorlevel 1 (
        set PYTHON_EXE=%%i
        goto :python_found
    )
)
:python_found
if not defined PYTHON_EXE (
    echo ERROR: Could not find Python outside WindowsApps. Please install Python from python.org.
    exit /b 1
)
echo Using Python: %PYTHON_EXE%

:: Create build directory if it doesn't exist
if not exist build mkdir build

:: Check if virtual environment exists in build directory
if exist build\venv (
    echo Virtual environment already exists in build directory.
) else (
    echo Creating virtual environment in build directory...
    "%PYTHON_EXE%" -m venv build\venv
)

:: Activate virtual environment
call build\venv\Scripts\activate

:: Upgrade pip
python -m pip install --upgrade pip

:: Install Conan
pip install conan

:: Create default Conan profile
conan profile detect --force

:: Run Conan install
conan install . --build=missing -s compiler.cppstd=17

echo Installation completed successfully.
echo To build the project, run 'build.bat'

pause
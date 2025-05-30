@echo off
setlocal enabledelayedexpansion

:: Check for Python installation
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Python is not installed or not in PATH. Please install Python and try again.
    exit /b 1
)

:: Create build directory if it doesn't exist
if not exist build (
    mkdir build
)

:: Check if virtual environment exists in build directory
if exist build\venv (
    echo Virtual environment already exists in build directory.
) else (
    echo Creating virtual environment in build directory...
    python -m venv build\venv
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

:: Deactivate virtual environment
deactivate

echo Please restart your command prompt to ensure all PATH changes take effect.
pause
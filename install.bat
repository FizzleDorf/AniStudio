@echo off
setlocal enabledelayedexpansion

:: Check for Python installation
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Python is not installed or not in PATH. Please install Python and try again.
    exit /b 1
)

:: Check for 7-Zip installation
:: where 7z >nul 2>&1
:: if %errorlevel% neq 0 (
::     echo 7-Zip is not installed. Installing 7-Zip...
::     powershell -Command "& {Invoke-WebRequest -Uri 'https://www.7-zip.org/a/7z2301-x64.exe' -OutFile '7z-installer.exe'}"
::     7z-installer.exe /S
::     set "PATH=%PATH%;C:\Program Files\7-Zip"
::     del 7z-installer.exe
:: )

:: Check if virtual environment exists
if exist venv (
    echo Virtual environment already exists.
) else (
    echo Creating virtual environment...
    python -m venv venv
)

:: Activate virtual environment
call venv\Scripts\activate

:: Upgrade pip
python -m pip install --upgrade pip

:: Install Conan
pip install conan

:: Create default Conan profile
conan profile detect --force

:: Run Conan install
conan install . --build=missing -s compiler.cppstd=17
cd ..

echo Installation completed successfully.
echo To build the project, run 'build.bat'

:: Deactivate virtual environment
deactivate

echo Please restart your command prompt to ensure all PATH changes take effect.
pause
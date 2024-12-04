@echo off
:: Activate the virtual environment
call venv\Scripts\activate.bat

:: Remove build directory if it exists and create a new one
if exist build rmdir /s /q build
mkdir build
cd build


:: Configure with CMake
:: cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake .. -DSD_VULKAN=ON -DCMAKE_BUILD_TYPE=Release

:: Build the project
cmake --build . --config Release

:: Deactivate the virtual environment
deactivate

cd ..
:: Keep the command prompt open
pause
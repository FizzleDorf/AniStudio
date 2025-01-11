@echo off
:: Activate the virtual environment
call venv\Scripts\activate.bat

:: Navigate to the build directory if it exists
if exist build (
    cd build
    
    :: Delete specific directories if they exist
    if exist Release rmdir /s /q Release
    if exist bin rmdir /s /q bin
    if exist external rmdir /s /q external
    if exist x64 rmdir /s /q x64
    
    :: Delete all files directly in the build directory
    del /q *

    :: Go back to the parent directory
    cd ..
) else (
    echo "Build directory does not exist."
)

:: Create build directory if it was deleted or does not exist
mkdir build
cd build

:: Configure with CMake (only build AniStudio)
cmake .. -DSD_VULKAN=ON ^
         -DCMAKE_BUILD_TYPE=Release ^
         -DCMAKE_TOOLCHAIN_FILE=generators\conan_toolchain.cmake ^
         -DCMAKE_POLICY_DEFAULT_CMP0091=NEW ^
         -DBUILD_PLUGINS=OFF ^
         -DBUILD_ANISTUDIO=ON

:: Build the project
cmake --build . --config Release

:: Deactivate the virtual environment
deactivate

cd ..
:: Keep the command prompt open
pause
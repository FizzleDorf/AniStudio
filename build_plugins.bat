@echo off
:: Activate the virtual environment
call venv\Scripts\activate.bat

cd build

:: Configure with CMake for plugins only
cmake .. -DCMAKE_BUILD_TYPE=Release ^
         -DBUILD_PLUGINS=ON ^
         -DBUILD_ANISTUDIO=OFF

:: Build just the plugins without rebuilding everything else
cmake --build . --config Release --target ExamplePlugin

:: Deactivate the virtual environment
deactivate

cd ..

echo.
echo Plugin build complete. DLLs can be found in build/plugins
echo.
pause
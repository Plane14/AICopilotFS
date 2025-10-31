@echo off
call vcvars_custom.bat
if exist build rd /s /q build
mkdir build
cd build
cmake .. -G Ninja -DBUILD_WITHOUT_SIMCONNECT=ON
cmake --build . --config Release
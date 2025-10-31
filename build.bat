@echo off
REM Build script for AICopilotFS

REM Set up VS environment
call vcvars_custom.bat

REM Clean build
if exist build rd /s /q build
mkdir build
cd build

REM Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64 -DBUILD_WITHOUT_SIMCONNECT=ON -DCMAKE_CXX_COMPILER="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe" -DCMAKE_C_COMPILER="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe"

REM Build
cmake --build . --config Release

cd ..
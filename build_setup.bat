@echo off
cd /d c:\Users\marti\source\repos\Plane14\AICopilotFS
rmdir /s /q build 2>nul
mkdir build
cd build
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
echo Environment initialized, running CMake with NMake...
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DENABLE_OLLAMA=OFF
if errorlevel 1 (
    echo CMake configuration failed
    exit /b 1
)
echo Configuration successful, building...
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed
    exit /b 1
)
echo Build completed successfully

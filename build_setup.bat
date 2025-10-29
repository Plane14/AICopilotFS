@echo off
cd /d c:\Users\marti\source\repos\Plane14\AICopilotFS\build
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
echo Reconfiguring without tests...
cmake .. -DBUILD_TESTS=OFF >nul 2>&1
echo Building...
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed
    exit /b 1
)
echo Build completed successfully
echo Executables:
dir /s bin\*.exe 2>nul

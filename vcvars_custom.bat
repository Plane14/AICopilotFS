@echo off
REM ============================================================
REM Wrapper for vcvarsall.bat with fixed SDK version
REM ============================================================

REM Choose your target/host architecture
set TARGET_ARCH=x64
set HOST_ARCH=x64

REM Pin a specific Windows SDK version (check installed versions under
REM C:\Program Files (x86)\Windows Kits\10\Include\ )
set WINSDK_VER=10.0.22621.0

REM Optionally, pin a specific VC toolset version
REM set VCVARS_VER=14.38

REM Call the official vcvarsall.bat with your chosen settings
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" %HOST_ARCH%_%TARGET_ARCH% %WINSDK_VER% -vcvars_ver=%VCVARS_VER%

REM Print confirmation
echo.
echo [vcvars_custom] Environment initialized for %HOST_ARCH% -> %TARGET_ARCH% with SDK %WINSDK_VER%
echo.
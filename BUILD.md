# Building AI Copilot FS

## Prerequisites

### Required Software
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or later
- Git (for cloning the repository)

### Required SDKs
You need at least one of the following:

**Microsoft Flight Simulator 2024 SDK**
- Download from: https://docs.flightsimulator.com/msfs2024/html/1_Introduction/Introduction.htm
- Install to a known location
- Set environment variable: `MSFS_SDK_PATH=/path/to/MSFS2024/SDK`

**Prepar3D V6 SDK**
- Download from: https://www.prepar3d.com/SDKv6/sdk/sdk_overview.html
- Install to a known location
- Set environment variable: `P3D_SDK_PATH=/path/to/Prepar3D/SDK`

## Build Steps

### Linux / macOS

```bash
# Clone the repository
git clone https://github.com/Plane14/AICopilotFS.git
cd AICopilotFS

# Set SDK path (choose one)
export MSFS_SDK_PATH=/path/to/MSFS2024/SDK
# or
export P3D_SDK_PATH=/path/to/Prepar3D/SDK

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
make -j$(nproc)

# Optional: Install
sudo make install
```

### Windows (Visual Studio)

```cmd
REM Clone the repository
git clone https://github.com/Plane14/AICopilotFS.git
cd AICopilotFS

REM Set SDK path (choose one)
set MSFS_SDK_PATH=C:\MSFS2024\SDK
REM or
set P3D_SDK_PATH=C:\Prepar3D\SDK

REM Create build directory
mkdir build
cd build

REM Configure for Visual Studio
cmake .. -G "Visual Studio 16 2019" -A x64

REM Build
cmake --build . --config Release

REM Optional: Install
cmake --install . --config Release
```

### Windows (MinGW)

```cmd
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

## Build Options

### CMake Options

```bash
# Disable examples
cmake .. -DBUILD_EXAMPLES=OFF

# Enable tests
cmake .. -DBUILD_TESTS=ON

# Set build type
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Custom install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/aicopilot
```

### Complete Build Example

```bash
cd AICopilotFS
mkdir build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install
```

## Build Output

After a successful build:

```
build/
├── bin/
│   └── aicopilot_example     # Example application
└── lib/
    └── libaicopilot.a        # Static library
```

## Troubleshooting

### CMake can't find compiler

```bash
# Specify compiler explicitly
cmake .. -DCMAKE_CXX_COMPILER=g++
cmake .. -DCMAKE_CXX_COMPILER=clang++
```

### Missing SimConnect headers

Make sure the SDK environment variable is set correctly:
```bash
echo $MSFS_SDK_PATH
echo $P3D_SDK_PATH
```

The SDK should contain:
- `SimConnect SDK/include/SimConnect.h` (MSFS)
- `inc/SimConnect.h` (P3D)

### Build warnings about SimConnect

The current implementation includes SimConnect as a stub. To complete integration:

1. Uncomment the SimConnect library linking in CMakeLists.txt
2. Implement actual SimConnect API calls in `simconnect_wrapper.cpp`
3. Link against `SimConnect.lib` (Windows) or `libSimConnect.so` (Linux)

## Running Tests

```bash
cd build
## Cleaning Build

### Required SDKs
You need at least one of the following to enable SimConnect integration:

Microsoft Flight Simulator 2024 SDK
- Download: https://docs.flightsimulator.com/msfs2024/html/1_Introduction/Introduction.htm
- Default path used by CMake: `C:/MSFS 2024 SDK`

Prepar3D v6 SDK
- Download: https://www.prepar3d.com/SDKv6/sdk/sdk_overview.html
- Default path used by CMake: `D:/Lockheed Martin/Prepar3D v6 SDK`

You can override the SDK locations when configuring CMake using these cache variables:
- `-DMSFS_2024_SDK_BASE="C:/Path/To/MSFS 2024 SDK"`
- `-DP3D_V6_SDK_BASE="D:/Path/To/Prepar3D v6 SDK"`

Alternatively, you can build without SimConnect (stub mode) using:
- `-DBUILD_WITHOUT_SIMCONNECT=ON`

### Windows (Visual Studio 2022)

REM Configure for Visual Studio 2022 (choose ONE of the following)

REM 1) Stub mode (no SDK required)
cmake .. -G "Visual Studio 17 2022" -A x64 -DBUILD_WITHOUT_SIMCONNECT=ON

REM 2) MSFS 2024 SDK
cmake .. -G "Visual Studio 17 2022" -A x64 -DUSE_MSFS_2024_SDK=ON -DMSFS_2024_SDK_BASE="C:/MSFS 2024 SDK"

REM 3) Prepar3D v6 SDK
cmake .. -G "Visual Studio 17 2022" -A x64 -DUSE_P3D_V6_SDK=ON -DP3D_V6_SDK_BASE="D:/Lockheed Martin/Prepar3D v6 SDK"

REM 4) Support both SDKs (one must be installed)
cmake .. -G "Visual Studio 17 2022" -A x64 -DSUPPORT_BOTH_SDKS=ON -DMSFS_2024_SDK_BASE="C:/MSFS 2024 SDK" -DP3D_V6_SDK_BASE="D:/Lockheed Martin/Prepar3D v6 SDK"
make clean
### Build Options

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake .. -DCMAKE_BUILD_TYPE=Debug

# SimConnect selection
cmake .. -DUSE_MSFS_2024_SDK=ON -DMSFS_2024_SDK_BASE="C:/MSFS 2024 SDK"
cmake .. -DUSE_P3D_V6_SDK=ON -DP3D_V6_SDK_BASE="D:/Lockheed Martin/Prepar3D v6 SDK"
cmake .. -DSUPPORT_BOTH_SDKS=ON
cmake .. -DBUILD_WITHOUT_SIMCONNECT=ON
## IDE Integration
### Missing SimConnect headers or libraries

Pass the correct SDK base path to CMake using:
```cmd
cmake .. -DUSE_MSFS_2024_SDK=ON -DMSFS_2024_SDK_BASE="C:/MSFS 2024 SDK"
cmake .. -DUSE_P3D_V6_SDK=ON -DP3D_V6_SDK_BASE="D:/Lockheed Martin/Prepar3D v6 SDK"
```

The SDK should contain:
- MSFS: `SimConnect SDK/include/SimConnect.h` and `SimConnect SDK/lib/SimConnect.lib`
- P3D: `inc/SimConnect/SimConnect.h` and `lib/SimConnect/SimConnect.lib`

If you cannot install an SDK, use stub mode: `-DBUILD_WITHOUT_SIMCONNECT=ON`.

### LINK : fatal error LNK1104: cannot open 'libucrt.lib'

Install or repair the Windows 10/11 SDK in Visual Studio Installer and ensure the selected SDK version is available. Then re-run the Developer Command Prompt and reconfigure CMake.
### Visual Studio
### Notes about SimConnect

The library now supports three modes:
- Full MSFS 2024 SDK
- Full Prepar3D v6 SDK
- Stub mode (no SDK; SimConnect calls are no-ops so you can build and test non-SDK features)

Selection is controlled by the CMake options shown above. No absolute include paths are required; CMake configures include and link directories.
Then open `AICopilotFS.sln`

## Next Steps

After building, see:
- [aicopilot/README.md](../aicopilot/README.md) - Usage documentation
- [EXAMPLES.md](EXAMPLES.md) - Example code and scenarios

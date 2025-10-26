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
ctest --output-on-failure
```

## Cleaning Build

```bash
# Remove all build artifacts
rm -rf build/

# Or use make
cd build
make clean
```

## IDE Integration

### Visual Studio Code

Install CMake Tools extension and open the folder. CMake will configure automatically.

### CLion

Open the project folder directly. CLion will detect CMakeLists.txt.

### Visual Studio

Generate Visual Studio solution:
```cmd
cmake .. -G "Visual Studio 16 2019"
```
Then open `AICopilotFS.sln`

## Next Steps

After building, see:
- [aicopilot/README.md](../aicopilot/README.md) - Usage documentation
- [EXAMPLES.md](EXAMPLES.md) - Example code and scenarios

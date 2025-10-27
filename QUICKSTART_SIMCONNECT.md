# SimConnect Wrapper - Quick Start Guide

## Overview

The SimConnect wrapper is **fully implemented** and ready for use. This guide will get you up and running in minutes.

## Files

- **Header:** `aicopilot/include/simconnect_wrapper.h`
- **Implementation:** `aicopilot/src/simconnect/simconnect_wrapper.cpp` (1200 lines)
- **Integration Guide:** `SIMCONNECT_INTEGRATION.md`
- **Status:** `IMPLEMENTATION_STATUS.md`

## Prerequisites

1. **Simulator** (one of):
   - Microsoft Flight Simulator 2024
   - Prepar3D v6

2. **SDK** (installed automatically with simulator):
   - MSFS 2024: `c:\MSFS 2024 SDK`
   - P3D v6: `d:\Lockheed Martin\Prepar3D v6 SDK`

3. **Build Tools**:
   - Visual Studio 2022 (with C++ workload)
   - CMake 3.10+

## 5-Minute Setup

### Step 1: Verify SDK Installation

```powershell
# Check MSFS 2024 SDK
Test-Path "c:\MSFS 2024 SDK\SimConnect SDK\include\SimConnect.h"

# OR check P3D v6 SDK
Test-Path "d:\Lockheed Martin\Prepar3D v6 SDK\inc\SimConnect\SimConnect.h"
```

### Step 2: Build the Project

```powershell
cd c:\Users\marti\source\repos\Plane14\AICopilotFS
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Step 3: Verify Build Output

Look for:
```
-- Using MSFS 2024 SDK at: c:/MSFS 2024 SDK
-- Linking SimConnect library: ...
-- Build files written to: ...
```

## First Program (3 minutes)

Create `test_simconnect.cpp`:

```cpp
#include "simconnect_wrapper.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace AICopilot;

int main() {
    // Create wrapper instance
    SimConnectWrapper sim;
    
    // Connect to MSFS 2024 (change to P3D_V6 if needed)
    std::cout << "Connecting to simulator..." << std::endl;
    if (!sim.connect(SimulatorType::MSFS2024, "Test Program")) {
        std::cerr << "Failed to connect!" << std::endl;
        return 1;
    }
    std::cout << "Connected!" << std::endl;
    
    // Read aircraft state 10 times
    for (int i = 0; i < 10; i++) {
        // Process messages (required!)
        sim.processMessages();
        
        // Get current state
        AircraftState state = sim.getAircraftState();
        
        // Print position
        std::cout << "Position: " 
                  << state.position.latitude << ", "
                  << state.position.longitude << ", "
                  << state.position.altitude << " ft" << std::endl;
        std::cout << "Speed: " << state.indicatedAirspeed << " kts" << std::endl;
        std::cout << "On Ground: " << (state.onGround ? "YES" : "NO") << std::endl;
        std::cout << std::endl;
        
        // Wait 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Disconnect
    sim.disconnect();
    std::cout << "Disconnected." << std::endl;
    
    return 0;
}
```

Build and run:
```powershell
# Add to CMakeLists.txt:
# add_executable(test_simconnect test_simconnect.cpp)
# target_link_libraries(test_simconnect aicopilot)

cmake --build . --config Release
.\bin\Release\test_simconnect.exe
```

## Common Use Cases

### 1. Monitor Aircraft State

```cpp
sim.subscribeToAircraftState([](const AircraftState& state) {
    std::cout << "Alt: " << state.position.altitude 
              << " Speed: " << state.indicatedAirspeed << std::endl;
});

while (sim.isConnected()) {
    sim.processMessages();  // Triggers callback
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

### 2. Control Autopilot

```cpp
// Enable autopilot
sim.setAutopilotMaster(true);

// Set heading to 090° (East)
sim.setAutopilotHeading(90.0);

// Climb to 10,000 feet
sim.setAutopilotAltitude(10000.0);

// Maintain 250 knots
sim.setAutopilotSpeed(250.0);
```

### 3. Manual Flight

```cpp
// Take manual control
sim.setAutopilotMaster(false);

// Set throttle to 75%
sim.setThrottle(0.75);

// Pitch up slightly
sim.setElevator(0.1);

// Bank right
sim.setAileron(0.2);
```

### 4. Landing Configuration

```cpp
// Extend gear
sim.setGear(true);

// Set flaps to landing position (100%)
sim.setFlaps(100);

// Enable approach mode
sim.setAutopilotApproach(true);

// Turn on landing lights
sim.setLight("LANDING", true);
```

## Troubleshooting

### "Failed to connect"
1. Make sure simulator is **running**
2. Check if you're in a flight (not main menu)
3. Verify SimConnect is enabled in simulator settings

### "Cannot find SimConnect.h"
1. Check SDK installation
2. Set environment variable:
   ```powershell
   $env:MSFS_SDK_PATH = "c:\MSFS 2024 SDK"
   ```
3. Re-run CMake

### "No data updates"
1. Ensure you call `processMessages()` regularly
2. Check if data subscription succeeded
3. Verify simulator is not paused

### Build errors
1. Check Visual Studio C++ workload is installed
2. Ensure x64 architecture (not x86)
3. Verify CMake version is 3.10+

## API Reference Quick Card

### Connection
- `connect(SimulatorType, appName)` - Connect to simulator
- `disconnect()` - Disconnect
- `isConnected()` - Check connection status
- `processMessages()` - **Call regularly!** (10-100ms)

### Data Reading
- `getAircraftState()` - Get current state
- `getAutopilotState()` - Get AP state
- `getPosition()` - Get position only
- `subscribeToAircraftState(callback)` - Auto updates

### Autopilot
- `setAutopilotMaster(bool)` - Toggle AP
- `setAutopilotHeading(degrees)` - 0-360
- `setAutopilotAltitude(feet)` - MSL
- `setAutopilotSpeed(knots)` - IAS
- `setAutopilotVerticalSpeed(fpm)` - Climb/descent rate
- `setAutopilotNav(bool)` - NAV mode
- `setAutopilotApproach(bool)` - Approach mode

### Flight Controls
- `setThrottle(0.0-1.0)` - Power
- `setElevator(-1.0 to 1.0)` - Pitch
- `setAileron(-1.0 to 1.0)` - Roll
- `setRudder(-1.0 to 1.0)` - Yaw

### Systems
- `setFlaps(0-100)` - Flaps percentage
- `setGear(bool)` - true=down, false=up
- `setSpoilers(bool)` - Deploy/retract
- `setParkingBrake(bool)` - Set/release
- `setBrakes(0.0-1.0)` - Brake pressure

### Engine
- `setMixture(0.0-1.0)` - 1.0=rich, 0.0=lean
- `setPropellerPitch(0.0-1.0)` - 1.0=fine, 0.0=coarse
- `setMagnetos(0-4)` - 0=off, 3=both
- `toggleEngineStarter(1-4)` - Engine index

### Lights
- `setLight(name, bool)` - Name: NAV, BEACON, LANDING, TAXI, STROBE, PANEL, etc.

### ATC
- `requestATCMenu()` - Open ATC
- `sendATCMenuSelection(0-9)` - Select option

## Performance Tips

1. **Update Rate**: Call `processMessages()` every 10-100ms
   ```cpp
   std::this_thread::sleep_for(std::chrono::milliseconds(50));  // 20Hz
   ```

2. **Use Subscriptions**: More efficient than polling
   ```cpp
   sim.subscribeToAircraftState(callback);  // Better than repeatedly calling getAircraftState()
   ```

3. **Batch Commands**: Group related operations
   ```cpp
   sim.setAutopilotMaster(true);
   sim.setAutopilotHeading(270.0);
   sim.setAutopilotAltitude(5000.0);
   // All sent in quick succession
   ```

## Next Steps

1. ✅ You now have a working SimConnect wrapper
2. ✅ Read full docs: `SIMCONNECT_INTEGRATION.md`
3. ✅ Check examples: `aicopilot/examples/`
4. ✅ Build your AI pilot!

## Support

- Full documentation: `SIMCONNECT_INTEGRATION.md`
- Implementation details: `IMPLEMENTATION_STATUS.md`
- Example code: `aicopilot/examples/main_example.cpp`
- SimConnect docs: https://docs.flightsimulator.com/

---

**Status:** ✅ Ready to Use  
**Implementation:** Complete  
**Testing:** Recommended before production

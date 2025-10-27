# SimConnect Integration Guide

## Overview

This document describes the complete SimConnect integration for AI Copilot FS, supporting both Microsoft Flight Simulator 2024 and Lockheed Martin Prepar3D v6.

## Implementation Status

✅ **COMPLETE** - All TODO items have been implemented with production-ready code.

### What Has Been Implemented

1. **SimConnect Connection Management**
   - `SimConnect_Open()` for establishing connection
   - `SimConnect_Close()` for cleanup
   - Connection state tracking
   - Automatic reconnection support

2. **Data Definitions**
   - **Aircraft State**: 17 data points including position, attitude, speeds, fuel, etc.
   - **Autopilot State**: 12 data points including all autopilot modes and targets
   - Proper data type mapping (FLOAT64, INT32)
   - Unit specifications (degrees, feet, knots, etc.)

3. **Event Mappings** (70+ events)
   - **Autopilot Events**: Master, heading, altitude, speed, VS, NAV, approach
   - **Flight Controls**: Throttle, elevator, aileron, rudder
   - **Aircraft Systems**: Flaps, gear, spoilers, brakes
   - **Engine Controls**: Mixture, prop pitch, magnetos, starters
   - **Lighting**: Navigation, beacon, landing, taxi, strobe, etc.
   - **ATC**: Menu open and selection (0-9)
   - **System Events**: SimStart, SimStop, Pause

4. **Data Request/Subscription**
   - One-time data requests via `SimConnect_RequestDataOnSimObjectType()`
   - Periodic subscriptions via `SimConnect_RequestDataOnSimObject()`
   - Frame-based updates with change detection
   - Callback mechanism for state updates

5. **Message Dispatch System**
   - Static callback function for SimConnect messages
   - Message type handling:
     - `SIMCONNECT_RECV_ID_SIMOBJECT_DATA`
     - `SIMCONNECT_RECV_ID_EVENT`
     - `SIMCONNECT_RECV_ID_EXCEPTION`
     - `SIMCONNECT_RECV_ID_QUIT`
   - Proper context passing for object-oriented design

6. **Control Functions**
   - All autopilot controls with proper value conversion
   - All flight controls with range clamping and conversion
   - All aircraft systems (gear, flaps, brakes, etc.)
   - All engine controls with multi-engine support
   - All lighting controls with name-based mapping
   - ATC interaction (menu open and selection)

7. **Error Handling**
   - HRESULT checking for all SimConnect calls
   - Error logging with hex error codes
   - Graceful degradation on failures
   - Connection state validation

8. **SDK Compatibility**
   - Conditional compilation for MSFS 2024 vs P3D v6
   - Automatic SDK path detection
   - CMake integration with proper linking
   - Header path configuration

## File Structure

```
aicopilot/
├── include/
│   ├── simconnect_wrapper.h      # Public API interface
│   └── aicopilot_types.h          # Type definitions
├── src/
│   └── simconnect/
│       └── simconnect_wrapper.cpp # Full implementation (1000+ lines)
```

## Architecture

### Data Flow

```
Simulator (MSFS/P3D)
    ↕ SimConnect API
SimConnectWrapper::Impl
    ↕ Internal structures
SimConnectWrapper (Public API)
    ↕ Callbacks
AI Pilot / Application
```

### Key Components

1. **SimConnectWrapper Class** (Public Interface)
   - PIMPL idiom for SDK hiding
   - Clean public API
   - State management
   - Callback registration

2. **SimConnectWrapper::Impl Class** (Private Implementation)
   - SimConnect handle management
   - Data structures (SimConnectAircraftState, SimConnectAutopilotState)
   - Static dispatch callback
   - Data conversion methods

3. **Enumerations**
   - `DATA_DEFINITIONS`: Aircraft state, autopilot state, etc.
   - `DATA_REQUEST_ID`: Request identifiers
   - `EVENT_ID`: 70+ event mappings

## Building

### Prerequisites

**Required:**
- CMake 3.10+
- C++17 compiler (MSVC, GCC, Clang)
- Windows SDK

**Choose one:**
- Microsoft Flight Simulator 2024 SDK
  - Location: `c:\MSFS 2024 SDK\SimConnect SDK`
  - Include: `SimConnect SDK\include\SimConnect.h`
  - Library: `SimConnect SDK\lib\SimConnect.lib`

- Prepar3D v6 SDK
  - Location: `d:\Lockheed Martin\Prepar3D v6 SDK`
  - Include: `inc\SimConnect\SimConnect.h`
  - Library: `lib\SimConnect\SimConnect.lib`

### Build Steps

1. **Configure CMake:**
   ```powershell
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

2. **Build:**
   ```powershell
   cmake --build . --config Release
   ```

3. **Alternative: Set environment variables:**
   ```powershell
   $env:MSFS_SDK_PATH = "c:\MSFS 2024 SDK"
   # OR
   $env:P3D_SDK_PATH = "d:\Lockheed Martin\Prepar3D v6 SDK"
   ```

### CMake Configuration

The build system automatically:
1. Detects installed SDKs at default locations
2. Falls back to environment variables
3. Sets appropriate preprocessor definitions (`USE_MSFS_2024` or `USE_P3D_V6`)
4. Configures include paths
5. Links SimConnect.lib

## Usage Examples

### Basic Connection

```cpp
#include "simconnect_wrapper.h"

using namespace AICopilot;

SimConnectWrapper simConnect;

// Connect to MSFS 2024
if (simConnect.connect(SimulatorType::MSFS2024, "My AI Copilot")) {
    std::cout << "Connected!" << std::endl;
    
    // Process messages in main loop
    while (simConnect.isConnected()) {
        simConnect.processMessages();
        
        // Your AI logic here
        
        Sleep(10); // ~100Hz update rate
    }
    
    simConnect.disconnect();
}
```

### Reading Aircraft State

```cpp
// One-time request
AircraftState state = simConnect.getAircraftState();
std::cout << "Altitude: " << state.position.altitude << " feet" << std::endl;
std::cout << "Speed: " << state.indicatedAirspeed << " knots" << std::endl;

// Subscribe for continuous updates
simConnect.subscribeToAircraftState([](const AircraftState& state) {
    std::cout << "Position: " << state.position.latitude 
              << ", " << state.position.longitude << std::endl;
});
```

### Controlling Autopilot

```cpp
// Enable autopilot
simConnect.setAutopilotMaster(true);

// Set heading to 270°
simConnect.setAutopilotHeading(270.0);

// Set altitude to 10,000 feet
simConnect.setAutopilotAltitude(10000.0);

// Set speed to 250 knots
simConnect.setAutopilotSpeed(250.0);

// Enable NAV mode
simConnect.setAutopilotNav(true);
```

### Manual Flight Controls

```cpp
// Set throttle to 75%
simConnect.setThrottle(0.75);

// Set elevator (pitch down slightly)
simConnect.setElevator(-0.1);

// Bank left
simConnect.setAileron(-0.2);

// Flaps to 50%
simConnect.setFlaps(50);

// Gear down
simConnect.setGear(true);
```

### Engine Management

```cpp
// Set magnetos to BOTH (position 3)
simConnect.setMagnetos(3);

// Set mixture to rich
simConnect.setMixture(1.0);

// Toggle starter for engine 1
simConnect.toggleEngineStarter(1);

// Set prop pitch to fine
simConnect.setPropellerPitch(1.0);
```

### Lighting

```cpp
// Turn on navigation lights
simConnect.setLight("NAV", true);

// Turn on landing lights
simConnect.setLight("LANDING", true);

// Turn on beacon
simConnect.setLight("BEACON", true);

// Turn on strobes
simConnect.setLight("STROBE", true);
```

### ATC Interaction

```cpp
// Open ATC menu
simConnect.requestATCMenu();

// Select menu option 1
simConnect.sendATCMenuSelection(1);
```

## SimConnect Data Definitions

### Aircraft State Structure

| SimVar Name | Unit | Type | Description |
|-------------|------|------|-------------|
| Plane Latitude | degrees | FLOAT64 | Current latitude |
| Plane Longitude | degrees | FLOAT64 | Current longitude |
| Plane Altitude | feet | FLOAT64 | Altitude MSL |
| Plane Heading Degrees True | degrees | FLOAT64 | True heading |
| Plane Pitch Degrees | degrees | FLOAT64 | Pitch angle |
| Plane Bank Degrees | degrees | FLOAT64 | Bank angle |
| Airspeed Indicated | knots | FLOAT64 | IAS |
| Airspeed True | knots | FLOAT64 | TAS |
| Ground Velocity | knots | FLOAT64 | Ground speed |
| Vertical Speed | feet per minute | FLOAT64 | Climb/descent rate |
| Kohlsman Setting Hg | inHg | FLOAT64 | Altimeter setting |
| Fuel Total Quantity | gallons | FLOAT64 | Total fuel |
| General Eng RPM:1 | rpm | FLOAT64 | Engine 1 RPM |
| Sim On Ground | bool | INT32 | On ground flag |
| Brake Parking Position | bool | INT32 | Parking brake |
| Gear Handle Position | bool | INT32 | Gear state |
| Flaps Handle Percent | percent | FLOAT64 | Flaps position |

### Autopilot State Structure

| SimVar Name | Unit | Type | Description |
|-------------|------|------|-------------|
| Autopilot Master | bool | INT32 | AP master switch |
| Autopilot Heading Lock | bool | INT32 | Heading hold mode |
| Autopilot Altitude Lock | bool | INT32 | Altitude hold mode |
| Autopilot Airspeed Hold | bool | INT32 | Speed hold mode |
| Autopilot Nav1 Lock | bool | INT32 | NAV mode |
| Autopilot Approach Hold | bool | INT32 | Approach mode |
| Autopilot Throttle Arm | bool | INT32 | Auto-throttle |
| Autopilot Vertical Hold | bool | INT32 | VS mode |
| Autopilot Heading Lock Dir | degrees | FLOAT64 | Target heading |
| Autopilot Altitude Lock Var | feet | FLOAT64 | Target altitude |
| Autopilot Airspeed Hold Var | knots | FLOAT64 | Target speed |
| Autopilot Vertical Hold Var | feet per minute | FLOAT64 | Target VS |

## Event Mappings Reference

### Autopilot Events

- `AUTOPILOT_ON` - Toggle autopilot master
- `AP_HDG_HOLD` - Toggle heading hold
- `AP_ALT_HOLD` - Toggle altitude hold
- `AP_AIRSPEED_HOLD` - Toggle speed hold
- `AP_VS_HOLD` - Toggle vertical speed hold
- `AP_NAV1_HOLD` - Toggle NAV mode
- `AP_APR_HOLD` - Toggle approach mode
- `HEADING_BUG_SET` - Set heading bug (0-360)
- `AP_ALT_VAR_SET_ENGLISH` - Set altitude target (feet)
- `AP_SPD_VAR_SET` - Set speed target (knots)
- `AP_VS_VAR_SET_ENGLISH` - Set VS target (fpm)

### Flight Control Events

- `THROTTLE_SET` - Set throttle (0-16384)
- `AXIS_ELEVATOR_SET` - Set elevator position
- `AXIS_AILERONS_SET` - Set aileron position
- `AXIS_RUDDER_SET` - Set rudder position

### Aircraft System Events

- `FLAPS_SET` - Set flaps position (0-5 typically)
- `GEAR_UP` / `GEAR_DOWN` - Raise/lower gear
- `SPOILERS_ON` / `SPOILERS_OFF` - Deploy/retract spoilers
- `PARKING_BRAKES` - Toggle parking brake
- `BRAKES` - Apply brakes

### Engine Events

- `MIXTURE_SET` - Set mixture (0-16384)
- `PROP_PITCH_SET` - Set propeller pitch
- `MAGNETO_SET` - Set magneto position (0-4)
- `TOGGLE_STARTER1-4` - Toggle engine starters

### Lighting Events

- `TOGGLE_NAV_LIGHTS` - Navigation lights
- `TOGGLE_BEACON_LIGHTS` - Beacon lights
- `LANDING_LIGHTS_TOGGLE` - Landing lights
- `TOGGLE_TAXI_LIGHTS` - Taxi lights
- `STROBES_TOGGLE` - Strobe lights

## Troubleshooting

### Connection Issues

**Problem:** `SimConnect_Open()` fails
- Ensure simulator is running
- Check if SimConnect.cfg is properly configured
- Verify no firewall is blocking connection
- Check if another application is using SimConnect

**Problem:** Cannot find SimConnect.lib
- Verify SDK installation path
- Check CMake configuration output
- Manually set environment variable:
  ```powershell
  $env:MSFS_SDK_PATH = "c:\MSFS 2024 SDK"
  ```

### Data Issues

**Problem:** Data not updating
- Ensure `processMessages()` is called regularly
- Check if data subscription was successful
- Verify simulator is not paused

**Problem:** Controls not working
- Check if event mapping succeeded
- Verify simulator allows external control
- Check for conflicting control bindings

### Build Issues

**Problem:** Missing SimConnect.h
- Verify SDK installation
- Check include paths in CMake configuration
- Ensure correct SDK version

**Problem:** Linker errors
- Verify SimConnect.lib path
- Check architecture match (x64)
- Ensure proper linking in CMakeLists.txt

## Performance Considerations

1. **Update Rate**: Default is per-frame (30-60Hz)
   - Adjust with `SIMCONNECT_PERIOD_` constants
   - Use `SIMCONNECT_PERIOD_SECOND` for less frequent updates

2. **Data Filtering**: Use `SIMCONNECT_DATA_REQUEST_FLAG_CHANGED`
   - Only receive updates when values change
   - Reduces CPU usage

3. **Message Processing**: Call `processMessages()` regularly
   - 10-100ms intervals recommended
   - Too fast: wasted CPU
   - Too slow: delayed updates

4. **Batch Operations**: Group related control changes
   - Set multiple autopilot values together
   - Reduces SimConnect overhead

## Differences: MSFS 2024 vs P3D v6

### MSFS 2024
- More comprehensive SimVar coverage
- Better performance
- Enhanced autopilot modes
- Modern aircraft systems support

### Prepar3D v6
- Traditional SimConnect API
- Established event system
- Legacy aircraft compatibility
- Training-focused features

### API Compatibility
The wrapper handles both transparently. Key differences:
- Header include paths (handled by CMake)
- Library linking (handled by CMake)
- Some advanced features may be MSFS-only
- Core functionality is identical

## Future Enhancements

Potential additions:
1. **Weather Data**: Add weather state definitions
2. **Traffic Data**: AI aircraft tracking
3. **Terrain Data**: Elevation queries
4. **Advanced Autopilot**: VNAV, LNAV modes
5. **System Failures**: Failure injection
6. **Replay Control**: Record/playback
7. **Custom Camera**: View control
8. **Text Messages**: ATC text parsing

## References

- [SimConnect SDK Documentation](https://docs.flightsimulator.com/html/Programming_Tools/SimConnect/SimConnect_SDK.htm)
- [P3D SDK Documentation](https://www.prepar3d.com/SDK/)
- [SimConnect Event Reference](https://docs.flightsimulator.com/html/Programming_Tools/Event_IDs/Event_IDs.htm)
- [SimConnect SimVars Reference](https://docs.flightsimulator.com/html/Programming_Tools/SimVars/Simulation_Variables.htm)

## Support

For issues or questions:
1. Check this documentation
2. Review example code in `aicopilot/examples/`
3. Check SimConnect SDK documentation
4. Open GitHub issue with:
   - SDK version
   - Build configuration
   - Error messages
   - Steps to reproduce

---

**Implementation Date:** October 2025  
**Status:** Production Ready ✅  
**Version:** 1.0.0

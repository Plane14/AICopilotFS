# AI Copilot FS

A fully-featured autonomous AI pilot for Microsoft Flight Simulator 2024 (MSFS2024) and Lockheed Martin Prepar3D V6, capable of handling all aircraft systems, reading configuration files, and interacting with simulator ATC.

## Features

### Core Capabilities
- **Full Aircraft Systems Management**
  - Autopilot control (heading, altitude, speed, vertical speed)
  - Manual flight controls (pitch, roll, yaw)
  - Engine management (throttle, mixture, propeller pitch, magnetos)
  - Flight surfaces (flaps, gear, spoilers)
  - Lighting systems (nav, beacon, strobe, landing, taxi)
  - Ground operations (parking brake, brakes)

- **Configuration File Support**
  - Parse aircraft.cfg files to load aircraft specifications
  - Parse .FLT flight files to load scenarios and missions
  - Extract performance data, weight & balance, fuel capacity, etc.

- **ATC Interaction**
  - Monitor ATC messages and instructions
  - Context-aware menu selection based on flight phase
  - **Ollama LLM integration** for intelligent AI-powered menu selection (optional)
  - Rule-based fallback for reliability
  - Automatic response to clearances and instructions
  - Follow ATC guidance for taxi, takeoff, cruise, approach, and landing

- **Autonomous Flight Management**
  - Complete flight phase automation (preflight → taxi → takeoff → climb → cruise → descent → approach → landing → shutdown)
  - Flight plan following with waypoint navigation
  - Great circle navigation calculations
  - Cross-track error monitoring
  - Automatic phase transitions

- **Safety Systems**
  - Continuous system health monitoring
  - Fuel level warnings
  - Engine status checks
  - Emergency handling procedures

## Architecture

### Components

```
aicopilot/
├── include/              # Header files
│   ├── aicopilot_types.h      # Common data structures
│   ├── simconnect_wrapper.h   # SimConnect API wrapper
│   ├── config_parser.h        # INI/CFG file parser
│   ├── aircraft_config.h      # Aircraft configuration parser
│   ├── aircraft_systems.h     # Aircraft systems controller
│   ├── navigation.h           # Navigation and flight planning
│   ├── atc_controller.h       # ATC interaction handler
│   └── ai_pilot.h             # Main AI pilot controller
├── src/                  # Implementation files
│   ├── simconnect/       # SimConnect integration
│   ├── parsers/          # File parsers
│   ├── systems/          # Aircraft systems
│   ├── navigation/       # Navigation logic
│   ├── atc/              # ATC handling
│   └── ai/               # AI pilot logic
├── examples/             # Example applications
└── tests/                # Unit tests
```

## Requirements

### Development Requirements
- C++17 or later compiler
- CMake 3.10 or later
- MSFS2024 SDK or Prepar3D V6 SDK with SimConnect libraries

### Runtime Requirements
- Microsoft Flight Simulator 2024 or Lockheed Martin Prepar3D V6
- SimConnect properly installed and configured

## Building

### With CMake

```bash
mkdir build
cd build
cmake ..
make
```

### SDK Setup

1. Download and install the appropriate SDK:
   - **MSFS2024**: https://docs.flightsimulator.com/msfs2024/html/1_Introduction/Introduction.htm
   - **Prepar3D V6**: https://www.prepar3d.com/SDKv6/sdk/sdk_overview.html

2. Set environment variables for SDK paths:
   ```bash
   export MSFS_SDK_PATH=/path/to/MSFS2024/SDK
   # or
   export P3D_SDK_PATH=/path/to/Prepar3D/SDK
   ```

3. Update CMakeLists.txt with correct SimConnect library paths

## Usage

### Basic Example

```cpp
#include "ai_pilot.h"

using namespace AICopilot;

int main() {
    // Create AI Pilot
    AIPilot pilot;
    
    // Initialize for MSFS2024
    pilot.initialize(SimulatorType::MSFS2024);
    
    // Optional: Load aircraft configuration
    pilot.loadAircraftConfig("path/to/aircraft.cfg");
    
    // Optional: Load flight plan
    pilot.loadFlightPlan("path/to/flightplan.pln");
    
    // Start autonomous flight
    pilot.startAutonomousFlight();
    
    // Update loop (call at 10-20 Hz)
    while (pilot.isActive()) {
        pilot.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    return 0;
}
```

### Command Line Example

```bash
./aicopilot_example [aircraft.cfg] [flightplan.pln]
```

## Configuration

### Aircraft Configuration (.cfg)
The system reads standard MSFS/P3D aircraft.cfg files to extract:
- Aircraft type and specifications
- Performance data (cruise speed, stall speed, climb rate)
- Weight and balance
- Fuel capacity
- Engine configuration

### Flight Plans
Supports standard flight plan formats:
- .PLN (MSFS/P3D native format)
- .FMS (FMS format)
- Direct waypoint navigation

## ATC Integration

The AI Copilot monitors ATC communications and automatically selects appropriate responses based on:
- Current flight phase
- Flight plan
- Context keywords
- Priority scoring

### ATC Response Selection
The system uses an intelligent scoring algorithm to select the best ATC menu option:
- High priority for phase-relevant options (e.g., "request takeoff clearance" during takeoff phase)
- Keyword analysis (request, ready, roger, wilco, etc.)
- Negative keyword avoidance (unable, standby, cancel)

## Flight Phases

The AI Pilot automatically manages these flight phases:

1. **PREFLIGHT** - System checks, lights, parking brake
2. **TAXI_OUT** - Taxi to runway with lights
3. **TAKEOFF** - Full power, rotation, gear/flap retraction
4. **CLIMB** - Autopilot climb to cruise altitude
5. **CRUISE** - Maintain altitude and follow flight plan
6. **DESCENT** - Controlled descent to approach
7. **APPROACH** - Configure for landing, ILS/approach mode
8. **LANDING** - Final approach, flare, touchdown
9. **TAXI_IN** - Taxi to gate/parking
10. **SHUTDOWN** - Parking brake, engine shutdown

## Safety Features

- **Fuel Monitoring**: Warns at 20% and 10% fuel remaining
- **Engine Monitoring**: Detects engine failures
- **System Health Checks**: Continuous monitoring of all systems
- **Emergency Procedures**: Handles emergencies (low fuel, engine failure)
- **Manual Override**: Pilot can take control at any time

## API Documentation

### Main Classes

#### AIPilot
Main controller for autonomous flight operations.

**Key Methods:**
- `initialize(SimulatorType)` - Connect to simulator
- `loadAircraftConfig(path)` - Load aircraft configuration
- `loadFlightPlan(path)` - Load flight plan
- `startAutonomousFlight()` - Begin autonomous operation
- `stopAutonomousFlight()` - Stop autonomous operation
- `update()` - Main update loop (call regularly)
- `getStatusReport()` - Get current status string
- `setManualOverride(bool)` - Enable/disable manual control

#### SimConnectWrapper
Low-level interface to SimConnect API.

**Key Methods:**
- `connect()` - Establish SimConnect connection
- `getAircraftState()` - Query current aircraft state
- `setAutopilotHeading/Altitude/Speed()` - Control autopilot
- `setThrottle/Flaps/Gear()` - Control aircraft systems

#### Navigation
Flight planning and navigation calculations.

**Key Methods:**
- `loadFlightPlan()` - Load flight plan file
- `createDirectPlan()` - Create direct A-to-B plan
- `distanceToWaypoint()` - Calculate distance
- `bearingToWaypoint()` - Calculate bearing
- `crossTrackError()` - Get cross-track deviation

#### ATCController
ATC communication and response selection.

**Key Methods:**
- `processATCMessage()` - Handle incoming ATC message
- `setFlightPhase()` - Set current flight phase for context
- `getPendingInstructions()` - Get pending ATC instructions

## Limitations and TODO

### Current Limitations
- SimConnect integration is stubbed (requires actual SDK integration)
- ATC menu reading is limited by SimConnect API capabilities
- Weather assessment not yet implemented
- Emergency procedures are basic
- No terrain avoidance

### Future Enhancements
- [ ] Complete SimConnect integration with actual SDK
- [ ] Advanced weather analysis and avoidance
- [ ] Terrain awareness and collision avoidance
- [ ] Multiple aircraft type profiles
- [ ] Voice recognition for ATC (integration with voice tools)
- [ ] Machine learning for improved decision making
- [ ] Real-time traffic avoidance
- [ ] Advanced approach procedures (ILS, RNAV, etc.)
- [ ] Helicopter-specific operations

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes with tests
4. Submit a pull request

## License

This software is licensed under GPL3 or any later version.

Copyright 2025 AI Copilot FS Project

## References

- [MSFS2024 SDK Documentation](https://docs.flightsimulator.com/msfs2024/html/1_Introduction/Introduction.htm)
- [Prepar3D V6 SDK](https://www.prepar3d.com/SDKv6/sdk/sdk_overview.html)
- [SimConnect API Reference](https://docs.flightsimulator.com/msfs2024/html/6_Programming_APIs/SimConnect/SimConnect_API_Reference.htm)
- [Aircraft.cfg Documentation](https://docs.flightsimulator.com/html/Content_Configuration/SimObjects/Aircraft_SimO/aircraft_cfg.htm)
- [FLT File Properties](https://docs.flightsimulator.com/msfs2024/html/5_Content_Configuration/FLT_Files/FLT_Properties.htm)

## Support

For issues, questions, or suggestions, please open an issue on GitHub.

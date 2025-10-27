# AICopilotFS

A fully-featured autonomous AI pilot system for Microsoft Flight Simulator 2024 (MSFS2024) and Lockheed Martin Prepar3D V6.

## Overview

This project provides a comprehensive autonomous pilot that can:
- Control all aircraft systems (autopilot, engines, flight surfaces, etc.)
- Parse and utilize aircraft.cfg and .FLT configuration files
- Interact with simulator ATC, following instructions and selecting appropriate responses
- Execute complete autonomous flights from preflight to shutdown
- Perform intelligent decision-making based on flight phase and context

## Quick Start

See the [aicopilot/README.md](aicopilot/README.md) for detailed documentation.

### Building

```bash
mkdir build
cd build
cmake ..
make
```

### Running

```bash
./bin/aicopilot_example [aircraft.cfg] [flightplan.pln]
```

## Project Structure

- `aicopilot/` - Main AI Copilot system
  - `include/` - Header files
  - `src/` - Implementation files
  - `examples/` - Example applications
  - `tests/` - Unit tests
- `navdatareader-master/` - Navigation database reader (existing component)

## Features

- **SimConnect Integration**: Interfaces with MSFS2024 and Prepar3D V6 via SimConnect API
- **Aircraft Systems Control**: Full control over autopilot, engines, flight controls, and all aircraft systems
- **Configuration Parsers**: Read aircraft.cfg and .FLT files to extract aircraft specifications and flight parameters
- **ATC Interaction**: Context-aware ATC communication with intelligent menu selection
- **Autonomous Flight**: Complete flight automation through all phases (preflight → taxi → takeoff → climb → cruise → descent → approach → landing → shutdown)
- **Navigation**: Great circle navigation, waypoint following, cross-track error monitoring
- **Safety Systems**: Fuel monitoring, system health checks, emergency handling
- **Weather Analysis**: Advanced weather hazard detection, avoidance routing, and severity assessment
- **Terrain Awareness**: TAWS/EGPWS system with multi-level warnings and collision avoidance
- **Traffic Avoidance**: TCAS implementation with traffic advisories and resolution advisories
- **Advanced Approaches**: ILS, RNAV, and RNP approach procedures with precision guidance
- **Aircraft Profiles**: Multiple aircraft type profiles with performance-based procedures
- **Voice Recognition**: Integration framework for external speech-to-text/text-to-speech engines
- **Machine Learning**: Pattern-based decision optimization and learning system
- **Helicopter Operations**: Full support for rotorcraft including hover, VTOL, and autorotation

## Documentation

Full documentation is available in [aicopilot/README.md](aicopilot/README.md).

## Requirements

- C++17 compiler
- CMake 3.10 or later
- MSFS2024 SDK or Prepar3D V6 SDK with SimConnect
- Microsoft Flight Simulator 2024 or Prepar3D V6

## License

This software is licensed under GPL3 or any later version.

Copyright 2025 AI Copilot FS Project
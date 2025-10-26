# AI Copilot FS Examples

## Example 1: Basic Autonomous Flight

```cpp
#include "ai_pilot.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    using namespace AICopilot;
    
    // Create and initialize AI Pilot
    AIPilot pilot;
    
    if (!pilot.initialize(SimulatorType::MSFS2024)) {
        std::cerr << "Failed to initialize" << std::endl;
        return 1;
    }
    
    // Start autonomous operation
    pilot.startAutonomousFlight();
    
    // Main update loop
    while (pilot.isActive()) {
        pilot.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    return 0;
}
```

## Example 2: Loading Aircraft Configuration

```cpp
#include "ai_pilot.h"
#include <iostream>

int main() {
    using namespace AICopilot;
    
    AIPilot pilot;
    pilot.initialize(SimulatorType::P3D_V6);
    
    // Load Cessna 172 configuration
    if (pilot.loadAircraftConfig("C:/MSFS2024/SimObjects/Airplanes/Asobo_C172SP/aircraft.cfg")) {
        std::cout << "Aircraft configuration loaded successfully" << std::endl;
    }
    
    pilot.startAutonomousFlight();
    
    while (pilot.isActive()) {
        pilot.update();
        
        // Print status every 5 seconds
        static int counter = 0;
        if (counter++ % 100 == 0) {
            std::cout << pilot.getStatusReport() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    return 0;
}
```

## Example 3: Flight Plan Execution

```cpp
#include "ai_pilot.h"
#include <iostream>

int main() {
    using namespace AICopilot;
    
    AIPilot pilot;
    pilot.initialize(SimulatorType::MSFS2024);
    
    // Load aircraft and flight plan
    pilot.loadAircraftConfig("path/to/aircraft.cfg");
    pilot.loadFlightPlan("path/to/flightplan.pln");
    
    // Start autonomous flight
    pilot.startAutonomousFlight();
    
    while (pilot.isActive()) {
        pilot.update();
        
        // Check current phase
        FlightPhase phase = pilot.getCurrentPhase();
        
        // You can add custom logic based on phase
        if (phase == FlightPhase::CRUISE) {
            // Do something during cruise
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    return 0;
}
```

## Example 4: Manual Override

```cpp
#include "ai_pilot.h"
#include <iostream>

int main() {
    using namespace AICopilot;
    
    AIPilot pilot;
    pilot.initialize(SimulatorType::MSFS2024);
    pilot.startAutonomousFlight();
    
    bool manualControl = false;
    
    while (pilot.isActive()) {
        pilot.update();
        
        // Check for user input (simplified example)
        // In real code, use proper input handling
        if (/* user pressed override key */) {
            manualControl = !manualControl;
            pilot.setManualOverride(manualControl);
            
            if (manualControl) {
                std::cout << "Manual control enabled" << std::endl;
            } else {
                std::cout << "AI control resumed" << std::endl;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    return 0;
}
```

## Example 5: Direct Aircraft Systems Control

```cpp
#include "aircraft_systems.h"
#include "simconnect_wrapper.h"
#include "aircraft_config.h"
#include <memory>

int main() {
    using namespace AICopilot;
    
    // Create SimConnect connection
    auto simConnect = std::make_shared<SimConnectWrapper>();
    simConnect->connect(SimulatorType::MSFS2024);
    
    // Create default aircraft config
    AircraftConfig config;
    config.cruiseSpeed = 120.0;
    config.stallSpeed = 50.0;
    
    // Create systems controller
    AircraftSystems systems(simConnect, config);
    
    // Control autopilot
    systems.enableAutopilot(true);
    systems.setHeading(270.0);    // Heading west
    systems.setAltitude(10000.0); // 10,000 feet
    systems.setSpeed(120.0);      // 120 knots
    
    // Control lights
    systems.setNavigationLights(true);
    systems.setBeaconLights(true);
    
    // Main loop
    for (int i = 0; i < 1000; i++) {
        systems.update();
        
        AircraftState state = systems.getCurrentState();
        std::cout << "Alt: " << state.position.altitude 
                  << " HDG: " << state.heading << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return 0;
}
```

## Example 6: Navigation System

```cpp
#include "navigation.h"
#include <iostream>

int main() {
    using namespace AICopilot;
    
    Navigation nav;
    
    // Create simple direct flight plan
    FlightPlan plan = nav.createDirectPlan("KSEA", "KPDX", 8000.0);
    
    // Add some waypoints manually
    Waypoint wp1;
    wp1.id = "CHINS";
    wp1.position.latitude = 47.4502;
    wp1.position.longitude = -122.3088;
    wp1.altitude = 8000.0;
    plan.waypoints.push_back(wp1);
    
    Waypoint wp2;
    wp2.id = "BTGSN";
    wp2.position.latitude = 46.5;
    wp2.position.longitude = -122.5;
    wp2.altitude = 8000.0;
    plan.waypoints.push_back(wp2);
    
    // Current position
    Position currentPos;
    currentPos.latitude = 47.4502;
    currentPos.longitude = -122.3088;
    
    // Calculate navigation data
    Waypoint active = nav.getActiveWaypoint();
    double distance = nav.distanceToWaypoint(currentPos, active);
    double bearing = nav.bearingToWaypoint(currentPos, active);
    double xte = nav.crossTrackError(currentPos);
    
    std::cout << "Distance to waypoint: " << distance << " nm" << std::endl;
    std::cout << "Bearing: " << bearing << " deg" << std::endl;
    std::cout << "Cross-track error: " << xte << " nm" << std::endl;
    
    return 0;
}
```

## Example 7: Configuration File Parsing

```cpp
#include "aircraft_config.h"
#include <iostream>

int main() {
    using namespace AICopilot;
    
    // Parse aircraft configuration
    AircraftConfigParser parser;
    
    if (parser.parse("path/to/aircraft.cfg")) {
        AircraftConfig config = parser.getConfig();
        
        std::cout << "Aircraft: " << config.title << std::endl;
        std::cout << "Type: " << config.icaoTypeDesignator << std::endl;
        std::cout << "Cruise speed: " << config.cruiseSpeed << " kts" << std::endl;
        std::cout << "Fuel capacity: " << config.fuelCapacity << " gal" << std::endl;
        std::cout << "Max weight: " << config.maxGrossWeight << " lbs" << std::endl;
        
        // Get available variations (liveries)
        auto variations = parser.getVariations();
        std::cout << "\nAvailable variations:" << std::endl;
        for (const auto& var : variations) {
            std::cout << "  - " << var << std::endl;
        }
    }
    
    return 0;
}
```

## Example 8: ATC Interaction

```cpp
#include "atc_controller.h"
#include "simconnect_wrapper.h"
#include <memory>

int main() {
    using namespace AICopilot;
    
    auto simConnect = std::make_shared<SimConnectWrapper>();
    simConnect->connect(SimulatorType::MSFS2024);
    
    ATCController atc(simConnect);
    
    // Set flight plan for ATC coordination
    FlightPlan plan;
    plan.departure = "KSEA";
    plan.arrival = "KPDX";
    plan.cruiseAltitude = 8000.0;
    atc.setFlightPlan(plan);
    
    // Set current phase
    atc.setFlightPhase(FlightPhase::TAXI_OUT);
    
    // Request ATC menu
    atc.requestMenu();
    
    // Main loop
    while (true) {
        atc.update();
        
        // Check for pending instructions
        auto instructions = atc.getPendingInstructions();
        for (const auto& instruction : instructions) {
            std::cout << "ATC Instruction: " << instruction << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return 0;
}
```

## Example Aircraft Configuration File

```ini
[VERSION]
major = 1
minor = 0

[GENERAL]
atc_model = "C172"
icao_type_designator = "C172"
icao_manufacturer = "CESSNA"
icao_model = "172SP"

[WEIGHT_AND_BALANCE]
empty_weight = 1642
max_gross_weight = 2450

[FUEL]
fuel_capacity = 53.0

[REFERENCE SPEEDS]
cruise_speed = 122
stall_speed = 47
max_indicated_speed = 163

[GENERALENGINEDATA]
engine_type = 0
Engine.0 = "Lycoming IO-360-L2A"
```

## Example Flight File (.FLT)

```ini
[Main]
Title=Flight from KSEA to KPDX
Description=VFR flight Seattle to Portland

[Sim.0]
Sim.Latitude=N47° 26' 57.79"
Sim.Longitude=W122° 18' 32.48"
Sim.Altitude=433
Sim.Heading=160
Sim.OnGround=TRUE

[GPS_Engine]
Filename=Seattle_to_Portland.pln

[Weather]
WeatherPreset=Clear Skies
```

## Compilation

```bash
# Compile any example
g++ -std=c++17 example.cpp -I../aicopilot/include -L../build/lib -laicopilot -o example

# Run
./example path/to/aircraft.cfg path/to/flightplan.pln
```

## Tips

1. **Update Frequency**: Call `update()` at 10-20 Hz (50-100ms intervals) for smooth operation
2. **Error Handling**: Always check return values from initialization and loading methods
3. **Manual Override**: Use `setManualOverride(true)` to temporarily pause AI control
4. **Status Monitoring**: Call `getStatusReport()` periodically to monitor flight progress
5. **Phase Detection**: Check `getCurrentPhase()` to implement custom logic for each phase

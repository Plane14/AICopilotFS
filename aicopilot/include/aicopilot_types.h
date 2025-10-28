/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef AICOPILOT_TYPES_H
#define AICOPILOT_TYPES_H

#include <string>
#include <vector>
#include <map>

namespace AICopilot {

// Simulator types
enum class SimulatorType {
    MSFS2024,
    MSFS2020,
    P3D_V6,
    P3D_V5,
    P3D_V4,
    UNKNOWN
};

// Flight phase
enum class FlightPhase {
    PREFLIGHT,
    TAXI_OUT,
    TAKEOFF,
    CLIMB,
    CRUISE,
    DESCENT,
    APPROACH,
    LANDING,
    TAXI_IN,
    SHUTDOWN,
    UNKNOWN
};

// Aircraft type
enum class AircraftType {
    SINGLE_ENGINE_PROP,
    MULTI_ENGINE_PROP,
    TURBOPROP,
    JET,
    HELICOPTER,
    UNKNOWN
};

// Position structure
struct Position {
    double latitude;
    double longitude;
    double altitude;  // feet MSL
    double heading;   // degrees
};

// Aircraft state
struct AircraftState {
    Position position;
    double indicatedAirspeed;  // knots
    double trueAirspeed;       // knots
    double groundSpeed;        // knots
    double verticalSpeed;      // feet per minute
    double pitch;              // degrees
    double bank;               // degrees
    double heading;            // degrees
    double altimeter;          // inHg
    bool onGround;
    double fuelQuantity;       // gallons
    double engineRPM;
    bool parkingBrakeSet;
    bool gearDown;
    int flapsPosition;         // 0-100%
    
    // Electrical system data
    bool masterBattery;        // Battery master switch
    bool masterAlternator;     // Alternator master switch
    double batteryVoltage;     // Volts
    double batteryLoad;        // Amperes
    double generatorVoltage;   // Volts (alternator/generator output)
    double generatorLoad;      // Amperes
};

// Autopilot state
struct AutopilotState {
    bool masterEnabled;
    bool headingHold;
    bool altitudeHold;
    bool speedHold;
    bool verticalSpeedHold;
    bool navMode;
    bool approachMode;
    bool autoThrottle;
    double targetHeading;
    double targetAltitude;
    double targetSpeed;
    double targetVerticalSpeed;
};

// ATC communication
enum class ATCMessageType {
    CLEARANCE,
    TAXI,
    TAKEOFF,
    DEPARTURE,
    CRUISE,
    APPROACH,
    LANDING,
    OTHER
};

struct ATCMessage {
    ATCMessageType type;
    std::string message;
    std::vector<std::string> menuOptions;
    int selectedOption;
};

// Waypoint
struct Waypoint {
    Position position;
    std::string id;
    double altitude = 0.0;  // feet MSL
    std::string type;  // VOR, NDB, FIX, etc.
};

// Flight plan
struct FlightPlan {
    std::string departure;
    std::string arrival;
    std::vector<Waypoint> waypoints;
    double cruiseAltitude;
    double cruiseSpeed;
};

// Terrain and obstacle awareness
struct TerrainPoint {
    Position position;
    double elevation;  // feet MSL
    std::string type;  // terrain, obstacle, etc.
};

// Weather information
struct WeatherConditions {
    double windSpeed = 0.0;      // knots
    double windDirection = 0.0;  // degrees
    double visibility = 10.0;    // statute miles
    double cloudBase = 5000.0;   // feet AGL
    double ceiling = 5000.0;     // feet AGL
    double temperature = 15.0;   // Celsius
    double dewpoint = 10.0;      // Celsius
    bool icing = false;
    bool turbulence = false;
    bool precipitation = false;
};

} // namespace AICopilot

#endif // AICOPILOT_TYPES_H

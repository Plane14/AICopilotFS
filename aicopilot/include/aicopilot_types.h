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
};

// Autopilot state
struct AutopilotState {
    bool masterEnabled;
    bool headingHold;
    bool altitudeHold;
    bool speedHold;
    bool navMode;
    bool approachMode;
    bool autoThrottle;
    double targetHeading;
    double targetAltitude;
    double targetSpeed;
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
    std::string id;
    Position position;
    double altitude;  // feet MSL
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
    double windSpeed;      // knots
    double windDirection;  // degrees
    double visibility;     // statute miles
    double cloudBase;      // feet AGL
    double temperature;    // Celsius
    bool icing;
    bool turbulence;
    bool precipitation;
};

} // namespace AICopilot

#endif // AICOPILOT_TYPES_H

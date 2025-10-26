/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef AIRCRAFT_CONFIG_H
#define AIRCRAFT_CONFIG_H

#include "aicopilot_types.h"
#include "config_parser.h"
#include <string>

namespace AICopilot {

/**
 * Aircraft configuration data parsed from aircraft.cfg files
 */
struct AircraftConfig {
    std::string title;
    std::string icaoTypeDesignator;
    std::string icaoManufacturer;
    std::string icaoModel;
    AircraftType aircraftType;
    
    // Performance data
    double emptyWeight;       // lbs
    double maxGrossWeight;    // lbs
    double fuelCapacity;      // gallons
    double cruiseSpeed;       // knots
    double stallSpeed;        // knots
    double maxSpeed;          // knots
    double climbRate;         // feet per minute
    
    // Engine data
    int numberOfEngines;
    std::string engineType;
    double maxThrust;         // lbs
    
    // Flight envelope
    double maxAltitude;       // feet
    double serviceceiling;   // feet
    
    // Dimensions
    double wingspan;          // feet
    double length;            // feet
    double height;            // feet
};

/**
 * Parser for aircraft.cfg files
 */
class AircraftConfigParser {
public:
    AircraftConfigParser() = default;
    
    // Parse aircraft configuration file
    bool parse(const std::string& filePath);
    
    // Get parsed configuration
    AircraftConfig getConfig() const { return config_; }
    
    // Get available aircraft variations
    std::vector<std::string> getVariations() const;
    
private:
    ConfigParser parser_;
    AircraftConfig config_;
    
    AircraftType determineAircraftType(const std::string& engineType, int numEngines);
};

/**
 * Flight state data parsed from .FLT files
 */
struct FlightState {
    std::string title;
    std::string description;
    std::string missionType;
    Position startPosition;
    AircraftState aircraftState;
    std::string weatherPreset;
    std::string flightPlanFile;
};

/**
 * Parser for .FLT flight files
 */
class FlightFileParser {
public:
    FlightFileParser() = default;
    
    // Parse flight file
    bool parse(const std::string& filePath);
    
    // Get parsed flight state
    FlightState getFlightState() const { return flightState_; }
    
private:
    ConfigParser parser_;
    FlightState flightState_;
    
    Position parsePosition(const ConfigParser& parser);
};

} // namespace AICopilot

#endif // AIRCRAFT_CONFIG_H

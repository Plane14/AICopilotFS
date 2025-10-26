/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "../include/aircraft_config.h"
#include <algorithm>

namespace AICopilot {

bool AircraftConfigParser::parse(const std::string& filePath) {
    if (!parser_.parse(filePath)) {
        return false;
    }
    
    // Parse GENERAL section
    config_.title = parser_.getValue("GENERAL", "atc_model");
    config_.icaoTypeDesignator = parser_.getValue("GENERAL", "icao_type_designator");
    config_.icaoManufacturer = parser_.getValue("GENERAL", "icao_manufacturer");
    config_.icaoModel = parser_.getValue("GENERAL", "icao_model");
    
    // Parse WEIGHT_AND_BALANCE section
    config_.emptyWeight = parser_.getDoubleValue("WEIGHT_AND_BALANCE", "empty_weight", 0.0);
    config_.maxGrossWeight = parser_.getDoubleValue("WEIGHT_AND_BALANCE", "max_gross_weight", 0.0);
    
    // Parse FUEL section
    config_.fuelCapacity = parser_.getDoubleValue("FUEL", "fuel_capacity", 0.0);
    
    // Parse REFERENCE SPEEDS section
    config_.cruiseSpeed = parser_.getDoubleValue("REFERENCE SPEEDS", "cruise_speed", 0.0);
    config_.stallSpeed = parser_.getDoubleValue("REFERENCE SPEEDS", "stall_speed", 0.0);
    config_.maxSpeed = parser_.getDoubleValue("REFERENCE SPEEDS", "max_indicated_speed", 0.0);
    
    // Parse ENGINE section
    config_.numberOfEngines = parser_.getIntValue("GENERALENGINEDATA", "engine_type", 0);
    config_.engineType = parser_.getValue("GENERALENGINEDATA", "Engine.0");
    
    // Parse performance
    config_.climbRate = parser_.getDoubleValue("AIRPLANE_GEOMETRY", "max_climb_rate", 0.0);
    config_.maxAltitude = parser_.getDoubleValue("REFERENCE SPEEDS", "max_altitude", 0.0);
    
    // Determine aircraft type
    config_.aircraftType = determineAircraftType(config_.engineType, config_.numberOfEngines);
    
    return true;
}

std::vector<std::string> AircraftConfigParser::getVariations() const {
    std::vector<std::string> variations;
    auto sections = parser_.getSections();
    
    for (const auto& section : sections) {
        if (section.find("FLTSIM") == 0) {
            std::string title = parser_.getValue(section, "title");
            if (!title.empty()) {
                variations.push_back(title);
            }
        }
    }
    
    return variations;
}

AircraftType AircraftConfigParser::determineAircraftType(const std::string& engineType, int numEngines) {
    std::string lowerEngineType = engineType;
    std::transform(lowerEngineType.begin(), lowerEngineType.end(), lowerEngineType.begin(), ::tolower);
    
    if (lowerEngineType.find("turbine") != std::string::npos || 
        lowerEngineType.find("jet") != std::string::npos) {
        return AircraftType::JET;
    } else if (lowerEngineType.find("turboprop") != std::string::npos) {
        return AircraftType::TURBOPROP;
    } else if (lowerEngineType.find("heli") != std::string::npos) {
        return AircraftType::HELICOPTER;
    } else if (numEngines == 1) {
        return AircraftType::SINGLE_ENGINE_PROP;
    } else if (numEngines > 1) {
        return AircraftType::MULTI_ENGINE_PROP;
    }
    
    return AircraftType::UNKNOWN;
}

bool FlightFileParser::parse(const std::string& filePath) {
    if (!parser_.parse(filePath)) {
        return false;
    }
    
    // Parse Main section
    flightState_.title = parser_.getValue("Main", "Title");
    flightState_.description = parser_.getValue("Main", "Description");
    flightState_.missionType = parser_.getValue("Main", "MissionType");
    
    // Parse position
    flightState_.startPosition = parsePosition(parser_);
    
    // Parse weather
    flightState_.weatherPreset = parser_.getValue("Weather", "WeatherPreset");
    
    // Parse flight plan
    flightState_.flightPlanFile = parser_.getValue("GPS_Engine", "Filename");
    
    // Parse aircraft state
    flightState_.aircraftState.onGround = parser_.getBoolValue("Sim.0", "Sim.OnGround", true);
    flightState_.aircraftState.indicatedAirspeed = parser_.getDoubleValue("Sim.0", "Sim.IndicatedAirspeed", 0.0);
    flightState_.aircraftState.altimeter = parser_.getDoubleValue("Sim.0", "Sim.Altimeter", 29.92);
    
    return true;
}

Position FlightFileParser::parsePosition(const ConfigParser& parser) {
    Position pos;
    pos.latitude = parser.getDoubleValue("Sim.0", "Sim.Latitude", 0.0);
    pos.longitude = parser.getDoubleValue("Sim.0", "Sim.Longitude", 0.0);
    pos.altitude = parser.getDoubleValue("Sim.0", "Sim.Altitude", 0.0);
    pos.heading = parser.getDoubleValue("Sim.0", "Sim.Heading", 0.0);
    return pos;
}

} // namespace AICopilot

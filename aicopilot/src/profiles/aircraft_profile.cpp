/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*****************************************************************************/

#include "aircraft_profile.h"
#include "config_parser.h"

namespace AICopilot {

double AircraftProfile::getSpeedForPhase(FlightPhase phase) const {
    switch (phase) {
        case FlightPhase::TAKEOFF:
            return procedures.rotationSpeed;
        case FlightPhase::CLIMB:
            return procedures.climbSpeed;
        case FlightPhase::CRUISE:
            return performance.cruiseSpeed;
        case FlightPhase::DESCENT:
            return procedures.descentSpeed;
        case FlightPhase::APPROACH:
            return procedures.approachSpeed;
        case FlightPhase::LANDING:
            return procedures.landingSpeed;
        default:
            return performance.cruiseSpeed;
    }
}

double AircraftProfile::getPowerForPhase(FlightPhase phase) const {
    switch (phase) {
        case FlightPhase::TAKEOFF:
            return procedures.takeoffPower;
        case FlightPhase::CLIMB:
            return procedures.climbPower;
        case FlightPhase::CRUISE:
            return procedures.cruisePower;
        case FlightPhase::DESCENT:
            return procedures.descentPower;
        case FlightPhase::APPROACH:
            return procedures.approachPower;
        default:
            return procedures.cruisePower;
    }
}

double AircraftProfile::getFlapsForPhase(FlightPhase phase) const {
    switch (phase) {
        case FlightPhase::TAKEOFF:
            return procedures.takeoffFlaps;
        case FlightPhase::CLIMB:
            return procedures.climbFlaps;
        case FlightPhase::APPROACH:
            return procedures.approachFlaps;
        case FlightPhase::LANDING:
            return procedures.landingFlaps;
        default:
            return 0.0;
    }
}

bool AircraftProfileManager::loadProfile(const std::string& configPath) {
    ConfigParser parser;
    if (!parser.parse(configPath)) {
        return false;
    }
    
    AircraftProfile profile;
    
    // Parse basic info
    profile.icaoDesignator = parser.getValue("GENERAL", "icao_type_designator", "");
    profile.manufacturer = parser.getValue("GENERAL", "manufacturer", "");
    profile.model = parser.getValue("GENERAL", "model", "");
    
    // Parse performance
    profile.performance = parsePerformanceData(configPath);
    profile.procedures = parseProcedureData(configPath);
    
    // Determine category
    profile.category = determineCategory(profile.type, profile.numberOfEngines, profile.engineType);
    
    currentProfile_ = profile;
    profiles_[profile.icaoDesignator] = profile;
    
    return true;
}

PerformanceProfile AircraftProfileManager::parsePerformanceData(const std::string& configPath) const {
    PerformanceProfile perf;
    ConfigParser parser;
    parser.parse(configPath);
    
    perf.cruiseSpeed = parser.getDoubleValue("PERFORMANCE", "cruise_speed", 120.0);
    perf.vs1 = parser.getDoubleValue("PERFORMANCE", "stall_speed", 45.0);
    perf.climbRate = parser.getDoubleValue("PERFORMANCE", "climb_rate", 700.0);
    perf.serviceCeiling = parser.getDoubleValue("PERFORMANCE", "service_ceiling", 14000.0);
    
    return perf;
}

FlightProcedures AircraftProfileManager::parseProcedureData(const std::string& configPath) const {
    FlightProcedures proc;
    
    // Set reasonable defaults
    proc.takeoffFlaps = 10.0;
    proc.takeoffPower = 100.0;
    proc.rotationSpeed = 60.0;
    proc.climbSpeed = 80.0;
    proc.cruisePower = 75.0;
    proc.approachSpeed = 70.0;
    proc.landingFlaps = 30.0;
    proc.landingSpeed = 65.0;
    
    return proc;
}

AircraftCategory AircraftProfileManager::determineCategory(
    AircraftType type,
    int numEngines,
    const std::string& engineType) const {
    
    if (type == AircraftType::HELICOPTER) {
        return AircraftCategory::HELICOPTER;
    } else if (engineType.find("jet") != std::string::npos) {
        return AircraftCategory::JET_TRANSPORT;
    } else if (engineType.find("turboprop") != std::string::npos) {
        return AircraftCategory::TURBOPROP;
    } else if (numEngines == 1) {
        return AircraftCategory::SINGLE_ENGINE_PISTON;
    } else if (numEngines > 1) {
        return AircraftCategory::MULTI_ENGINE_PISTON;
    }
    
    return AircraftCategory::UNKNOWN;
}

} // namespace AICopilot

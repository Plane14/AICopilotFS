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

#ifndef AIRCRAFT_PROFILE_H
#define AIRCRAFT_PROFILE_H

#include "aicopilot_types.h"
#include <string>
#include <map>

namespace AICopilot {

// Aircraft category
enum class AircraftCategory {
    SINGLE_ENGINE_PISTON,
    MULTI_ENGINE_PISTON,
    TURBOPROP,
    JET_TRANSPORT,
    BUSINESS_JET,
    HELICOPTER,
    GLIDER,
    UNKNOWN
};

// Performance profile
struct PerformanceProfile {
    // Speeds (knots IAS)
    double v1;              // Takeoff decision speed (jets)
    double vr;              // Rotation speed
    double v2;              // Takeoff safety speed
    double vx;              // Best angle of climb
    double vy;              // Best rate of climb
    double vfe;             // Max flap extended speed
    double vno;             // Max structural cruise speed
    double vne;             // Never exceed speed
    double vso;             // Stall speed landing config
    double vs1;             // Stall speed clean
    double vref;            // Reference landing speed
    double vapp;            // Approach speed
    
    // Performance
    double climbRate;       // feet per minute
    double descentRate;     // feet per minute
    double cruiseSpeed;     // knots TAS
    double maxSpeed;        // knots TAS
    double serviceCeiling;  // feet MSL
    
    // Fuel
    double fuelCapacity;    // gallons
    double fuelFlow;        // gallons per hour
    double range;           // nautical miles
    double endurance;       // hours
    
    // Weights (lbs)
    double emptyWeight;
    double maxGrossWeight;
    double maxLandingWeight;
    double maxZeroFuelWeight;
};

// Flight procedures
struct FlightProcedures {
    // Takeoff
    double takeoffFlaps;        // degrees
    double takeoffPower;        // percent
    double rotationSpeed;       // knots
    double climboutSpeed;       // knots
    
    // Climb
    double climbPower;          // percent
    double climbSpeed;          // knots
    double climbFlaps;          // degrees
    
    // Cruise
    double cruisePower;         // percent
    double cruiseAltitude;      // feet MSL
    
    // Descent
    double descentPower;        // percent
    double descentSpeed;        // knots
    
    // Approach
    double approachFlaps;       // degrees
    double approachSpeed;       // knots
    double approachPower;       // percent
    
    // Landing
    double landingFlaps;        // degrees
    double landingSpeed;        // knots
    double touchdownSpeed;      // knots
};

// Helicopter-specific data
struct HelicopterData {
    double hoverCeiling;        // feet MSL (in ground effect)
    double hoverCeilingOGE;     // feet MSL (out of ground effect)
    double maxVerticalSpeed;    // feet per minute
    double cruiseSpeed;         // knots
    double vne;                 // never exceed speed (knots)
    double autorotationSpeed;   // knots
    double normalRotorRPM;      // normal operating rotor RPM
    bool hasAutopilot;
    bool hasSAS;               // Stability Augmentation System
};

/**
 * Complete Aircraft Profile from Configuration Files
 */
struct AircraftProfile {
    // Identification
    std::string icaoDesignator;
    std::string manufacturer;
    std::string model;
    std::string variant;
    AircraftType type;
    AircraftCategory category;
    
    // Configuration
    int numberOfEngines;
    std::string engineType;
    bool retractableGear;
    bool pressurized;
    bool hasSpoilers;
    
    // Performance
    PerformanceProfile performance;
    
    // Procedures
    FlightProcedures procedures;
    
    // Helicopter-specific (if applicable)
    HelicopterData helicopterData;
    bool isHelicopter() const { return type == AircraftType::HELICOPTER; }
    
    // Get recommended speed for phase
    double getSpeedForPhase(FlightPhase phase) const;
    
    // Get recommended power for phase
    double getPowerForPhase(FlightPhase phase) const;
    
    // Get recommended flaps for phase
    double getFlapsForPhase(FlightPhase phase) const;
};

/**
 * Aircraft Profile Manager
 * Loads and manages multiple aircraft type profiles
 */
class AircraftProfileManager {
public:
    AircraftProfileManager() = default;
    
    // Load profile from aircraft.cfg
    bool loadProfile(const std::string& configPath);
    
    // Get current profile
    AircraftProfile getCurrentProfile() const { return currentProfile_; }
    
    // Load profile by aircraft type
    bool loadProfileByType(AircraftType type);
    
    // Get all loaded profiles
    std::map<std::string, AircraftProfile> getAllProfiles() const { 
        return profiles_; 
    }
    
    // Check if profile is loaded
    bool hasProfile(const std::string& icaoDesignator) const;
    
    // Get profile by ICAO designator
    AircraftProfile getProfile(const std::string& icaoDesignator) const;
    
    // Parse performance data from aircraft.cfg
    PerformanceProfile parsePerformanceData(const std::string& configPath) const;
    
    // Parse flight procedures from aircraft.cfg
    FlightProcedures parseProcedureData(const std::string& configPath) const;
    
    // Determine aircraft category
    AircraftCategory determineCategory(
        AircraftType type,
        int numEngines,
        const std::string& engineType) const;
    
private:
    AircraftProfile currentProfile_;
    std::map<std::string, AircraftProfile> profiles_;
    
    // Helper methods
    void setDefaultPerformance(AircraftType type, PerformanceProfile& perf) const;
    void setDefaultProcedures(AircraftType type, FlightProcedures& proc) const;
    void setHelicopterDefaults(HelicopterData& heli) const;
};

} // namespace AICopilot

#endif // AIRCRAFT_PROFILE_H

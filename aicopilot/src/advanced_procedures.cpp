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

#include "advanced_procedures.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace AICopilot {

AdvancedProcedures::AdvancedProcedures()
    : initialized_(false),
      currentStatus_(ProcedureStatus::NOT_STARTED) {
}

AdvancedProcedures::~AdvancedProcedures() {
    shutdown();
}

bool AdvancedProcedures::initialize(const PerformanceProfile& profile,
                                     const AircraftCategory category) {
    profile_ = profile;
    category_ = category;
    initialized_ = true;
    currentStatus_ = ProcedureStatus::NOT_STARTED;
    executionHistory_.clear();
    return true;
}

void AdvancedProcedures::shutdown() {
    if (initialized_) {
        initialized_ = false;
        executionHistory_.clear();
    }
}

// ============================================================
// EMERGENCY PROCEDURES - ENGINE FAILURE
// ============================================================

EngineFailureProcedure AdvancedProcedures::handleEngineFailure(
    int failedEngineNumber,
    const AircraftState& currentState,
    double currentAltitude) {
    
    EngineFailureProcedure result;
    result.failedEngineNumber = failedEngineNumber;
    
    // Determine if single or multi-engine
    bool isMultiEngine = (category_ == AircraftCategory::MULTI_ENGINE_PISTON ||
                         category_ == AircraftCategory::TURBOPROP ||
                         category_ == AircraftCategory::JET_TRANSPORT ||
                         category_ == AircraftCategory::BUSINESS_JET);
    
    if (category_ == AircraftCategory::SINGLE_ENGINE_PISTON ||
        category_ == AircraftCategory::HELICOPTER) {
        result.engineType = EngineType::PISTON_SINGLE;
        result.minimumControlSpeed = profile_.vs1 + 5.0;  // 5 knots above stall
        result.immediateActions = 
            "1. Declare emergency\n"
            "2. Best glide speed: " + std::to_string(static_cast<int>(profile_.vy)) + " knots\n"
            "3. Find landing site\n"
            "4. Restart procedures (if applicable)\n"
            "5. Prepare for emergency landing";
        result.singleEngineClimbPerformance = 0.0;  // No climb capability
        result.estimatedGlideRange = 2.0 * currentAltitude / 1000.0;  // Typical 2:1 glide ratio
    } else {
        result.engineType = EngineType::PISTON_MULTI;
        result.minimumControlSpeed = profile_.vs1 + 10.0;  // Vmc considerations
        result.immediateActions =
            "1. Declare emergency\n"
            "2. Apply full rudder trim\n"
            "3. Reduce dead engine propeller pitch\n"
            "4. Feather failed engine propeller (if equipped)\n"
            "5. Reduce drag (retract gear, flaps)\n"
            "6. Establish single-engine climb";
        result.singleEngineClimbPerformance = std::max(0.0, profile_.climbRate * 0.3);  // ~30% climb performance
    }
    
    // Subsequent actions
    result.subsequentActions = {
        "Check fuel quantity and distribution",
        "Verify engine instruments on operating engine(s)",
        "Plan route to nearest suitable airport",
        "Contact ATC and declare emergency",
        "Brief crew on procedures",
        "Prepare for approach and landing",
        "Declare minimum fuel if needed"
    };
    
    // Find nearest airports
    result.preferredAirports = {
        Waypoint{{currentState.position.latitude, currentState.position.longitude, currentAltitude, 0}, "DIVERT_1"},
        Waypoint{{currentState.position.latitude + 0.1, currentState.position.longitude + 0.1, currentAltitude, 0}, "DIVERT_2"},
        Waypoint{{currentState.position.latitude - 0.1, currentState.position.longitude - 0.1, currentAltitude, 0}, "DIVERT_3"}
    };
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

// ============================================================
// EMERGENCY PROCEDURES - ELECTRICAL FAILURE
// ============================================================

ProcedureResult AdvancedProcedures::handleElectricalFailure(
    const AircraftState& currentState) {
    
    ProcedureResult result;
    result.status = ProcedureStatus::IN_PROGRESS;
    result.success = true;
    
    std::stringstream checklist;
    checklist << "ELECTRICAL SYSTEM FAILURE CHECKLIST\n"
              << "===================================\n\n"
              << "IMMEDIATE ACTIONS:\n"
              << "1. Check alternator/generator output\n"
              << "2. Verify battery switch is ON\n"
              << "3. Check for overload condition\n"
              << "4. Reduce electrical load (non-essential systems OFF)\n"
              << "5. Switch to essential bus (if equipped)\n\n"
              << "SYSTEMS TO DISABLE (non-essential):\n"
              << "  - Autopilot\n"
              << "  - De-ice systems\n"
              << "  - Cabin lights (reduce load)\n"
              << "  - Non-essential avionics\n\n"
              << "ESSENTIAL SYSTEMS TO MAINTAIN:\n"
              << "  - Radio/COM (critical)\n"
              << "  - Navigation (GPS/VOR)\n"
              << "  - Flight instruments\n"
              << "  - Engine instruments\n"
              << "  - Transponder (if possible)\n\n"
              << "FLIGHT PLAN:\n"
              << "  - Declare emergency\n"
              << "  - Plan direct routing to nearest airport\n"
              << "  - Estimate 45 minutes endurance (battery power)\n"
              << "  - Brief crew on battery limitations\n"
              << "  - Prepare manual flight techniques\n\n"
              << "LANDING CHECKLIST:\n"
              << "  - Plan for manual gear extension (if applicable)\n"
              << "  - Verify flap operation\n"
              << "  - Position for visual approach\n"
              << "  - Notify ATC of electrical failure\n";
    
    result.checklist = checklist.str();
    result.message = "Electrical failure procedure initiated";
    result.actionsPerformed = {
        "Assessed electrical system status",
        "Generated emergency checklist",
        "Identified essential systems",
        "Planned for battery-powered flight"
    };
    result.executionTime = 0.5;
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

// ============================================================
// EMERGENCY PROCEDURES - HYDRAULIC FAILURE
// ============================================================

ProcedureResult AdvancedProcedures::handleHydraulicFailure(
    const AircraftState& currentState) {
    
    ProcedureResult result;
    result.status = ProcedureStatus::IN_PROGRESS;
    result.success = true;
    
    std::stringstream checklist;
    checklist << "HYDRAULIC SYSTEM FAILURE CHECKLIST\n"
              << "==================================\n\n"
              << "IMMEDIATE ACTIONS:\n"
              << "1. Check hydraulic pressure gauges\n"
              << "2. Switch to backup hydraulic system (if equipped)\n"
              << "3. Assess affected systems (flaps, gear, brakes)\n"
              << "4. Reduce speed to avoid control forces\n"
              << "5. Brief crew on manual procedures\n\n"
              << "LIKELY AFFECTED SYSTEMS:\n"
              << "  - Flight control surfaces (reduced authority)\n"
              << "  - Landing gear (manual extension if equipped)\n"
              << "  - Wheel brakes (manual backup if equipped)\n"
              << "  - Nose wheel steering (loss of power steering)\n\n"
              << "FLIGHT TECHNIQUES:\n"
              << "  - Reduce speed gradually to minimize control forces\n"
              << "  - Make shallow turns (reduced roll control authority)\n"
              << "  - Plan for wide landing area (braking may be limited)\n"
              << "  - Avoid rapid maneuvers\n\n"
              << "APPROACH AND LANDING:\n"
              << "  - Request longest available runway\n"
              << "  - Plan early descent (reduce descent rate)\n"
              << "  - Use manual landing procedures\n"
              << "  - Declare emergency\n"
              << "  - Ensure emergency services standing by\n";
    
    result.checklist = checklist.str();
    result.message = "Hydraulic failure procedure initiated";
    result.actionsPerformed = {
        "Assessed hydraulic system status",
        "Identified affected systems",
        "Generated emergency procedures",
        "Planned manual-flight approach"
    };
    result.executionTime = 0.75;
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

// ============================================================
// EMERGENCY PROCEDURES - FUEL SYSTEM FAILURE
// ============================================================

FuelEmergencyProcedure AdvancedProcedures::handleFuelSystemFailure(
    const AircraftState& currentState,
    double estimatedLeakRate) {
    
    FuelEmergencyProcedure result;
    result.totalFuelRemaining = currentState.fuelQuantity;
    result.fuelConsumptionRate = profile_.fuelFlow;
    
    // Add leak rate to consumption
    result.fuelConsumptionRate += estimatedLeakRate;
    
    // Standard reserve (45 minutes for Day VFR, 30 for IFR in some regions)
    result.requiredReserve = (result.fuelConsumptionRate * 45.0) / 60.0;
    
    // Calculate usable fuel
    double usableFuel = result.totalFuelRemaining - result.requiredReserve;
    result.estimatedTimeOfUsable = (usableFuel / result.fuelConsumptionRate) * 60.0;  // minutes
    
    // Determine if immediate divert needed
    result.requiresImmediateDivert = result.estimatedTimeOfUsable < 30.0;
    
    if (result.requiresImmediateDivert) {
        result.recommendedAction = "IMMEDIATE DIVERT - Declare emergency and proceed to nearest suitable airport NOW";
    } else if (result.estimatedTimeOfUsable < 60.0) {
        result.recommendedAction = "URGENT DIVERT - Begin descent and divert to nearest suitable airport immediately";
    } else {
        result.recommendedAction = "Plan divert to nearest airport - isolate fuel leak if possible";
    }
    
    // Populate nearest airports
    result.nearestAirports = {
        Waypoint{{currentState.position.latitude, currentState.position.longitude, currentState.position.altitude, 0}, "DIVERT_1"},
        Waypoint{{currentState.position.latitude + 0.05, currentState.position.longitude + 0.05, currentState.position.altitude, 0}, "DIVERT_2"},
        Waypoint{{currentState.position.latitude - 0.05, currentState.position.longitude - 0.05, currentState.position.altitude, 0}, "DIVERT_3"}
    };
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

// ============================================================
// EMERGENCY PROCEDURES - PRESSURIZATION FAILURE
// ============================================================

EmergencyDescentParameters AdvancedProcedures::handlePressurizationFailure(
    const AircraftState& currentState,
    const Position& position) {
    
    EmergencyDescentParameters result;
    result.pressurizedDescent = false;
    result.descentRate = 4000.0;  // aggressive descent
    result.targetAltitude = 10000.0;  // below RVSM airspace, get oxygen
    result.speed = profile_.vno;  // Max structural cruise speed
    result.maxSpeedLimit = profile_.vne;
    result.estimeatedTimeToSafeAltitude = (currentState.position.altitude - result.targetAltitude) / result.descentRate;
    
    // Populate safe alternates
    result.safeAlternates = {
        Waypoint{{position.latitude, position.longitude, result.targetAltitude, 0}, "DIVERT_1"},
        Waypoint{{position.latitude + 0.1, position.longitude + 0.1, result.targetAltitude, 0}, "DIVERT_2"},
        Waypoint{{position.latitude - 0.1, position.longitude - 0.1, result.targetAltitude, 0}, "DIVERT_3"}
    };
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

// ============================================================
// EMERGENCY PROCEDURES - FIRE DETECTION
// ============================================================

ProcedureResult AdvancedProcedures::handleFireDetection(
    const std::string& location,
    const AircraftState& currentState) {
    
    ProcedureResult result;
    result.status = ProcedureStatus::IN_PROGRESS;
    result.success = true;
    
    std::stringstream checklist;
    checklist << "FIRE DETECTION EMERGENCY - " << location << "\n"
              << "============================================\n\n"
              << "IMMEDIATE ACTIONS:\n"
              << "1. Declare MAYDAY emergency\n"
              << "2. Identify fire location and confirm\n"
              << "3. Shut down affected systems (engine, APU, etc.)\n"
              << "4. Activate fire suppression system\n"
              << "5. Prepare for emergency landing\n\n";
    
    if (location == "Engine") {
        checklist << "ENGINE FIRE PROCEDURES:\n"
                  << "  - MIXTURE: Cut to IDLE (piston) or FUEL SHUTOFF (turbine)\n"
                  << "  - FUEL PUMP: OFF\n"
                  << "  - ENGINE BLEED: OFF\n"
                  << "  - ENGINE ANTI-ICE: OFF\n"
                  << "  - FIRE EXTINGUISHER: Select engine and discharge\n"
                  << "  - If fire continues: SHUT DOWN completely\n"
                  << "  - Prepare single-engine procedures if multi-engine\n";
    } else if (location == "APU") {
        checklist << "APU FIRE PROCEDURES:\n"
                  << "  - APU MASTER: OFF\n"
                  << "  - APU BLEED: OFF (if available)\n"
                  << "  - APU FUEL SHUTOFF: OFF\n"
                  << "  - FIRE EXTINGUISHER: Select APU and discharge\n"
                  << "  - Leave APU compartment ventilation open\n";
    } else if (location == "Cargo") {
        checklist << "CARGO/CABIN FIRE PROCEDURES:\n"
                  << "  - Identify fire location precisely\n"
                  << "  - Activate fire suppression (if equipped)\n"
                  << "  - Increase ventilation if safe\n"
                  << "  - Do NOT open cargo door in flight\n"
                  << "  - Prepare for immediate landing\n";
    }
    
    checklist << "\nLANDING PLAN:\n"
              << "  - Declare EMERGENCY\n"
              << "  - Request immediate landing clearance\n"
              << "  - Avoid steep descents (could disrupt fire)\n"
              << "  - Request foam/emergency equipment standing by\n"
              << "  - Brief crew on evacuation procedures\n";
    
    result.checklist = checklist.str();
    result.message = "Fire emergency procedure initiated";
    result.actionsPerformed = {
        "Received fire detection alarm",
        "Generated emergency checklist",
        "Identified fire location: " + location,
        "Prepared for emergency landing"
    };
    result.executionTime = 1.0;
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

// ============================================================
// EMERGENCY PROCEDURES - EMERGENCY DESCENT
// ============================================================

EmergencyDescentParameters AdvancedProcedures::executeEmergencyDescent(
    const AircraftState& currentState,
    const Position& currentPos,
    double targetAltitude) {
    
    EmergencyDescentParameters result;
    result.descentRate = 4000.0;
    result.targetAltitude = targetAltitude;
    result.speed = profile_.vno * 0.85;  // 85% Vno
    result.maxSpeedLimit = profile_.vne;
    result.pressurizedDescent = false;
    result.estimeatedTimeToSafeAltitude = 
        (currentState.position.altitude - result.targetAltitude) / result.descentRate;
    
    // Find safe landing sites
    result.safeAlternates = {
        Waypoint{{currentPos.latitude, currentPos.longitude, result.targetAltitude, 0}, "DIVERT_1"},
        Waypoint{{currentPos.latitude + 0.05, currentPos.longitude, result.targetAltitude, 0}, "DIVERT_2"},
        Waypoint{{currentPos.latitude - 0.05, currentPos.longitude, result.targetAltitude, 0}, "DIVERT_3"},
        Waypoint{{currentPos.latitude, currentPos.longitude + 0.05, result.targetAltitude, 0}, "DIVERT_4"},
        Waypoint{{currentPos.latitude, currentPos.longitude - 0.05, result.targetAltitude, 0}, "DIVERT_5"}
    };
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

// ============================================================
// EMERGENCY PROCEDURES - STRUCTURAL DAMAGE
// ============================================================

ProcedureResult AdvancedProcedures::handleStructuralDamage(
    const std::string& damageLocation,
    const AircraftState& currentState) {
    
    ProcedureResult result;
    result.status = ProcedureStatus::IN_PROGRESS;
    result.success = true;
    
    std::stringstream checklist;
    checklist << "STRUCTURAL DAMAGE PROCEDURES - " << damageLocation << "\n"
              << "==========================================\n\n"
              << "ASSESSMENT:\n"
              << "  - Verify actual damage (not false indication)\n"
              << "  - Assess structural integrity\n"
              << "  - Check flight control effectiveness\n"
              << "  - Evaluate continued flight safety\n\n"
              << "FLYING LIMITATIONS:\n"
              << "  - Reduce airspeed to well below normal limits\n"
              << "  - Avoid high-G maneuvers\n"
              << "  - Minimize turbulence exposure\n"
              << "  - Plan gentle descent to landing\n\n"
              << "FLIGHT CONTROL CHECKS:\n"
              << "  - Test pitch control (small inputs)\n"
              << "  - Test roll control (small inputs)\n"
              << "  - Test yaw control (small inputs)\n"
              << "  - Verify all controls respond\n"
              << "  - Brief crew on control limitations\n\n"
              << "LANDING PROCEDURES:\n"
              << "  - Declare emergency\n"
              << "  - Request longest runway available\n"
              << "  - Reduce speed gradually\n"
              << "  - Plan wide approach\n"
              << "  - Position rescue services\n";
    
    result.checklist = checklist.str();
    result.message = "Structural damage procedure initiated";
    result.actionsPerformed = {
        "Assessed structural damage: " + damageLocation,
        "Verified flight control effectiveness",
        "Generated emergency landing procedures",
        "Planned conservative approach"
    };
    result.executionTime = 1.2;
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

// ============================================================
// GO-AROUND PROCEDURES
// ============================================================

GoAroundParameters AdvancedProcedures::executeGoAround(
    const AircraftState& currentState,
    const std::string& reason) {
    
    GoAroundParameters result;
    result.targetAltitude = currentState.position.altitude + 1500.0;
    result.targetSpeed = profile_.vy;  // Best rate of climb
    result.climbRate = profile_.climbRate;
    result.bankAngle = 15.0;
    result.turnDirection = 45.0;  // degrees heading change
    result.flapsRetract = true;
    result.gearUp = true;
    result.reason = reason;
    result.estimatedFuelRequired = 15.0;  // typical go-around fuel
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

GoAroundParameters AdvancedProcedures::executeGoAroundWithConditions(
    const AircraftState& currentState,
    double windSpeed,
    double windDirection,
    const WeatherConditions& weather) {
    
    GoAroundParameters result;
    result.targetAltitude = currentState.position.altitude + 2000.0;
    
    // Adjust for wind conditions
    if (windSpeed > 25.0) {  // Strong wind
        result.targetSpeed = profile_.vy + 10.0;  // Climb faster in strong wind
        result.bankAngle = 10.0;  // Shallow bank in wind
    } else {
        result.targetSpeed = profile_.vy;
        result.bankAngle = 15.0;
    }
    
    result.climbRate = profile_.climbRate * 0.9;  // Slightly reduced
    result.flapsRetract = true;
    result.gearUp = true;
    result.reason = "Go-around with wind consideration";
    result.estimatedFuelRequired = 20.0;  // More fuel due to altitude climb
    
    // Adjust turn direction based on wind
    if (windDirection > 180.0) {
        result.turnDirection = 90.0;  // Turn left
    } else {
        result.turnDirection = -90.0;  // Turn right
    }
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

GoAroundParameters AdvancedProcedures::executeBalkedLandingGoAround(
    const AircraftState& currentState) {
    
    GoAroundParameters result;
    result.targetAltitude = currentState.position.altitude + 2500.0;
    result.targetSpeed = profile_.vy + 5.0;  // Climb faster from low altitude
    result.climbRate = profile_.climbRate;
    result.bankAngle = 10.0;  // Shallow banks for low-altitude safety
    result.flapsRetract = true;
    result.gearUp = true;
    result.reason = "Balked landing go-around";
    result.estimatedFuelRequired = 25.0;  // Higher fuel for climb from low altitude
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

AdvancedProcedures::GoAroundPerformance AdvancedProcedures::calculateGoAroundPerformance(
    double currentAltitude,
    double currentSpeed,
    double fuelRemaining) {
    
    GoAroundPerformance result;
    result.requiredFuel = 20.0;  // Standard go-around fuel
    result.climbPerformance = profile_.climbRate;
    result.timeToReachClimb = 45.0;  // seconds to establish climb
    result.sufficientFuel = fuelRemaining > (result.requiredFuel + 30.0);  // 30gal reserve
    
    // Generate holding points
    result.holdingPoints = {
        Waypoint{{0, 0, currentAltitude + 1000, 0}, "HOLDING_1"},
        Waypoint{{0, 0.05, currentAltitude + 1000, 0}, "HOLDING_2"}
    };
    
    return result;
}

// ============================================================
// CROSSWIND LANDING PROCEDURES
// ============================================================

AdvancedProcedures::CrosswindComponents AdvancedProcedures::calculateCrosswindComponents(
    double windSpeed,
    double windDirection,
    double runwayHeading) {
    
    CrosswindComponents result;
    
    // Convert to radians
    double windDir = windDirection * M_PI / 180.0;
    double runwayDir = runwayHeading * M_PI / 180.0;
    
    // Calculate relative wind angle
    double relativeAngle = windDir - runwayDir;
    
    // Normalize angle to -180 to 180
    while (relativeAngle > M_PI) relativeAngle -= 2 * M_PI;
    while (relativeAngle < -M_PI) relativeAngle += 2 * M_PI;
    
    // Calculate headwind and crosswind
    result.headwind = windSpeed * std::cos(relativeAngle);
    result.crosswind = windSpeed * std::sin(relativeAngle);
    
    return result;
}

CrosswindAssessment AdvancedProcedures::assessRunwayForCrosswind(
    const Waypoint& runway,
    double windSpeed,
    double windDirection,
    double aircraftMaxCrosswind) {
    
    CrosswindAssessment result;
    
    CrosswindComponents components = calculateCrosswindComponents(
        windSpeed, windDirection, runway.position.heading);
    
    result.crosswindComponent = std::abs(components.crosswind);
    result.headwindComponent = components.headwind;
    result.tailwindComponent = -components.headwind;
    
    result.isAcceptable = (result.crosswindComponent <= aircraftMaxCrosswind &&
                          result.tailwindComponent < 5.0);  // Avoid tailwind
    
    result.suitableRunways = result.isAcceptable ? std::vector<int>{1} : std::vector<int>{};
    
    return result;
}

Waypoint AdvancedProcedures::selectRunwayForCrosswind(
    const std::vector<Waypoint>& availableRunways,
    double windSpeed,
    double windDirection,
    double aircraftMaxCrosswind,
    double aircraftMaxHeadwind) {
    
    Waypoint bestRunway;
    double bestScore = 1e9;
    
    for (const auto& runway : availableRunways) {
        CrosswindComponents components = calculateCrosswindComponents(
            windSpeed, windDirection, runway.position.heading);
        
        double crosswind = std::abs(components.crosswind);
        double headwind = components.headwind;
        double tailwind = -components.headwind;
        
        // Check constraints
        if (crosswind > aircraftMaxCrosswind) continue;
        if (headwind > aircraftMaxHeadwind) continue;
        if (tailwind > 5.0) continue;
        
        // Score: prioritize headwind (best) over crosswind
        double score = crosswind * 2.0 - headwind;
        
        if (score < bestScore) {
            bestScore = score;
            bestRunway = runway;
        }
    }
    
    return bestRunway;
}

AdvancedProcedures::CrosswindLandingTechnique AdvancedProcedures::getCrosswindLandingTechnique(
    double crosswindComponent,
    double headwindComponent,
    double aircraftMaxCrosswind) {
    
    CrosswindLandingTechnique result;
    result.crabAngle = std::atan(crosswindComponent / headwindComponent) * 180.0 / M_PI;
    result.touchdownSpeed = profile_.vref + 2.0;  // Slightly above Vref
    result.requiresMaxBraking = (crosswindComponent > aircraftMaxCrosswind * 0.9);
    
    if (crosswindComponent > aircraftMaxCrosswind * 0.75) {
        result.slipConfiguration = "Forward Slip (after touchdown)";
    } else {
        result.slipConfiguration = "Crab method or side slip";
    }
    
    result.procedure = {
        "1. Establish crab angle to maintain runway alignment",
        "2. Maintain descent rate with crosswind consideration",
        "3. Near touchdown: transition to forward slip if needed",
        "4. Touch down on main gear first (upwind side)",
        "5. Lower nose gear gently",
        "6. Apply moderate crosswind braking as needed"
    };
    
    return result;
}

// ============================================================
// WEATHER DIVERT PROCEDURES
// ============================================================

bool AdvancedProcedures::assessWeatherDivertNecessity(
    const WeatherConditions& currentWeather,
    const WeatherConditions& destinationWeather,
    const std::vector<WeatherHazard>& hazardsAhead) {
    
    // Check destination weather
    if (destinationWeather.visibility < 1.0 && 
        destinationWeather.ceiling < 500.0) {
        return true;  // Destination not usable
    }
    
    // Check for severe hazards ahead
    for (const auto& hazard : hazardsAhead) {
        if (hazard.severity == HazardSeverity::SEVERE ||
            hazard.severity == HazardSeverity::EXTREME) {
            return true;
        }
    }
    
    return false;
}

WeatherDivertParameters AdvancedProcedures::planWeatherDivert(
    const Position& currentPosition,
    const Waypoint& originalDestination,
    const std::vector<Waypoint>& alternateAirports,
    double currentFuel,
    const WeatherConditions& currentWeather) {
    
    WeatherDivertParameters result;
    
    // Select best alternate (typically nearest with good weather)
    if (!alternateAirports.empty()) {
        result.targetAirport = alternateAirports[0];
    }
    
    // Calculate distances and fuel
    double latDiff = result.targetAirport.position.latitude - currentPosition.latitude;
    double lonDiff = result.targetAirport.position.longitude - currentPosition.longitude;
    result.routeDistance = std::sqrt(latDiff * latDiff + lonDiff * lonDiff) * 60.0;  // Rough conversion to NM
    
    result.estimatedFlightTime = (result.routeDistance / profile_.cruiseSpeed) * 60.0;  // minutes
    result.requiredFuel = (result.routeDistance / profile_.range) * profile_.fuelCapacity + 30.0;  // 30 gal reserve
    
    result.IFRCapableRequired = (currentWeather.ceiling < 3000 || 
                                currentWeather.visibility < 5.0);
    
    result.weatherReasoning = "Weather deterioration detected at destination. "
                             "Diverting to alternate airport for safety.";
    
    // Primary hazard
    result.primaryHazard.type = WeatherHazardType::THUNDERSTORM;
    result.primaryHazard.severity = HazardSeverity::SEVERE;
    result.primaryHazard.position = originalDestination.position;
    
    currentStatus_ = ProcedureStatus::IN_PROGRESS;
    
    return result;
}

double AdvancedProcedures::calculateFuelForAlternate(
    const Position& currentPos,
    const Waypoint& alternate,
    double fuelReserve) {
    
    // Simple distance calculation
    double latDiff = alternate.position.latitude - currentPos.latitude;
    double lonDiff = alternate.position.longitude - currentPos.longitude;
    double distance = std::sqrt(latDiff * latDiff + lonDiff * lonDiff) * 60.0;  // NM
    
    // Fuel required
    double flightTime = (distance / profile_.cruiseSpeed);  // hours
    double fuelRequired = (flightTime * profile_.fuelFlow) + 
                         ((fuelReserve / 60.0) * profile_.fuelFlow);
    
    return fuelRequired;
}

// ============================================================
// PROCEDURE UTILITIES
// ============================================================

std::string AdvancedProcedures::getEmergencyChecklist(EmergencyType type) {
    std::stringstream ss;
    
    switch (type) {
        case EmergencyType::ENGINE_FAILURE:
            ss << "ENGINE FAILURE EMERGENCY\n"
               << "=======================\n"
               << "1. Declare emergency\n"
               << "2. Establish best glide speed\n"
               << "3. Locate landing site\n"
               << "4. Configure aircraft for landing\n";
            break;
            
        case EmergencyType::FUEL_SYSTEM_FAILURE:
            ss << "FUEL SYSTEM FAILURE\n"
               << "===================\n"
               << "1. Verify fuel quantity\n"
               << "2. Calculate remaining time\n"
               << "3. Isolate fuel leak if possible\n"
               << "4. Plan divert to nearest airport\n";
            break;
            
        case EmergencyType::ELECTRICAL_FAILURE:
            ss << "ELECTRICAL SYSTEM FAILURE\n"
               << "=========================\n"
               << "1. Check alternator/generator\n"
               << "2. Reduce electrical load\n"
               << "3. Maintain essential systems\n"
               << "4. Plan for limited endurance\n";
            break;
            
        case EmergencyType::PRESSURIZATION_FAILURE:
            ss << "PRESSURIZATION FAILURE\n"
               << "======================\n"
               << "1. Descent to 10,000 feet MSL\n"
               << "2. Don oxygen mask if equipped\n"
               << "3. Notify ATC immediately\n"
               << "4. Plan descent to safe altitude\n";
            break;
            
        case EmergencyType::FIRE_DETECTION:
            ss << "FIRE DETECTION\n"
               << "===============\n"
               << "1. MAYDAY - Declare emergency\n"
               << "2. Identify fire location\n"
               << "3. Shutdown affected systems\n"
               << "4. Activate fire suppression\n"
               << "5. Prepare emergency landing\n";
            break;
            
        default:
            ss << "GENERAL EMERGENCY PROCEDURES\n"
               << "============================\n"
               << "1. Aviate (control aircraft)\n"
               << "2. Navigate (plan route)\n"
               << "3. Communicate (declare emergency)\n";
    }
    
    return ss.str();
}

AdvancedProcedures::ProcedureLimits AdvancedProcedures::getProcedureLimits() const {
    ProcedureLimits limits;
    
    // Determine limits based on aircraft category
    if (category_ == AircraftCategory::SINGLE_ENGINE_PISTON) {
        limits.maxCrosswindComponent = 12.0;  // knots
        limits.maxDemonstrationSpeed = profile_.vne;
        limits.engineOutPerformance = 0.0;
        limits.singleEngineControlSpeed = 0.0;
    } else if (category_ == AircraftCategory::MULTI_ENGINE_PISTON) {
        limits.maxCrosswindComponent = 15.0;
        limits.maxDemonstrationSpeed = profile_.vne;
        limits.engineOutPerformance = profile_.climbRate * 0.3;
        limits.singleEngineControlSpeed = profile_.vs1 + 10.0;
    } else if (category_ == AircraftCategory::TURBOPROP) {
        limits.maxCrosswindComponent = 20.0;
        limits.maxDemonstrationSpeed = profile_.vne;
        limits.engineOutPerformance = profile_.climbRate * 0.4;
        limits.singleEngineControlSpeed = profile_.vs1 + 8.0;
    } else {
        limits.maxCrosswindComponent = 25.0;
        limits.maxDemonstrationSpeed = profile_.vne;
        limits.engineOutPerformance = profile_.climbRate * 0.5;
        limits.singleEngineControlSpeed = profile_.vs1 + 5.0;
    }
    
    return limits;
}

bool AdvancedProcedures::validateProcedureExecution(
    const ProcedureResult& result,
    const std::vector<std::string>& requiredSteps) {
    
    if (result.status != ProcedureStatus::COMPLETED &&
        result.status != ProcedureStatus::IN_PROGRESS) {
        return false;
    }
    
    for (const auto& step : requiredSteps) {
        auto found = std::find(result.actionsPerformed.begin(),
                              result.actionsPerformed.end(),
                              step);
        if (found == result.actionsPerformed.end()) {
            return false;
        }
    }
    
    return true;
}

void AdvancedProcedures::registerProcedureCallback(ProcedureCallback callback) {
    procedureCallback_ = callback;
}

std::vector<ProcedureResult> AdvancedProcedures::getProcedureHistory() const {
    return executionHistory_;
}

void AdvancedProcedures::clearProcedureHistory() {
    executionHistory_.clear();
}

ProcedureStatus AdvancedProcedures::getCurrentProcedureStatus() const {
    return currentStatus_;
}

double AdvancedProcedures::calculateMinimumControlSpeed() const {
    if (category_ == AircraftCategory::MULTI_ENGINE_PISTON ||
        category_ == AircraftCategory::MULTI_ENGINE_PISTON) {
        return profile_.vs1 + 10.0;  // Vmc considerations
    }
    return profile_.vs1 + 5.0;
}

double AdvancedProcedures::calculateSingleEnginePerformance(double altitude) const {
    if (category_ != AircraftCategory::MULTI_ENGINE_PISTON &&
        category_ != AircraftCategory::TURBOPROP &&
        category_ != AircraftCategory::JET_TRANSPORT) {
        return 0.0;  // Single engine - no climb capability
    }
    
    // Multi-engine: estimate 30% of normal climb performance
    // Performance decreases with altitude
    double altitudeMultiplier = 1.0 - (altitude / profile_.serviceCeiling);
    return profile_.climbRate * 0.3 * std::max(0.0, altitudeMultiplier);
}

std::vector<Waypoint> AdvancedProcedures::findNearestAirports(
    const Position& pos,
    size_t count,
    double maxDistance) {
    
    std::vector<Waypoint> result;
    
    // Simulate finding nearest airports (in production, query airport database)
    for (size_t i = 0; i < count; ++i) {
        Waypoint airport;
        airport.position.latitude = pos.latitude + (i - count/2) * 0.05;
        airport.position.longitude = pos.longitude + (i - count/2) * 0.05;
        airport.position.altitude = 0;
        airport.id = "AIRPORT_" + std::to_string(i);
        result.push_back(airport);
    }
    
    return result;
}

} // namespace AICopilot

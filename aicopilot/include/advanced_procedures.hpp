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

#ifndef ADVANCED_PROCEDURES_HPP
#define ADVANCED_PROCEDURES_HPP

#include "aicopilot_types.h"
#include "aircraft_profile.h"
#include "weather_system.h"
#include "terrain_awareness.h"
#include "navigation.h"
#include <vector>
#include <memory>
#include <chrono>
#include <functional>

namespace AICopilot {

/**
 * Advanced Flight Procedures System
 * 
 * Implements complex emergency procedures, go-around procedures, crosswind
 * landing procedures, and weather divert procedures based on FAA/ICAO standards.
 */

// Emergency procedure types
enum class EmergencyType {
    ENGINE_FAILURE,
    HYDRAULIC_FAILURE,
    ELECTRICAL_FAILURE,
    FUEL_SYSTEM_FAILURE,
    PRESSURIZATION_FAILURE,
    FIRE_DETECTION,
    EMERGENCY_DESCENT,
    STRUCTURAL_DAMAGE,
    SYSTEMS_FAILURE
};

// Procedure status
enum class ProcedureStatus {
    NOT_STARTED,
    IN_PROGRESS,
    COMPLETED,
    ABORTED,
    FAILED
};

// Engine type for emergency procedures
enum class EngineType {
    PISTON_SINGLE,
    PISTON_MULTI,
    TURBOPROP,
    TURBOFAN,
    OTHER
};

// Crosswind assessment result
struct CrosswindAssessment {
    double crosswindComponent;  // knots
    double headwindComponent;   // knots
    double tailwindComponent;   // knots
    bool isAcceptable;          // Within aircraft limits
    std::vector<int> suitableRunways;  // Runway IDs suitable for landing
};

// Go-around parameters
struct GoAroundParameters {
    double targetAltitude;           // feet MSL
    double targetSpeed;              // knots IAS
    double climbRate;                // feet per minute
    double turnDirection;            // Left or right
    double bankAngle;                // degrees
    bool flapsRetract;               // Retract flaps immediately
    bool gearUp;                     // Retract gear
    std::string reason;              // Reason for go-around
    double estimatedFuelRequired;    // gallons
};

// Emergency descent parameters
struct EmergencyDescentParameters {
    double descentRate;          // feet per minute (typically 4000+ fpm)
    double targetAltitude;       // feet MSL
    double speed;                // knots IAS
    bool pressurizedDescent;     // For pressurization failure
    double maxSpeedLimit;        // Never exceed limit during descent
    std::vector<Waypoint> safeAlternates;  // Nearby airports for emergency landing
    double estimeatedTimeToSafeAltitude;  // minutes
};

// Engine failure procedure parameters
struct EngineFailureProcedure {
    EngineType engineType;
    int failedEngineNumber;              // 1 for single, 1-2+ for multi
    double minimumControlSpeed;          // knots IAS
    double singleEngineClimbPerformance; // feet per minute
    std::string immediateActions;        // First steps
    std::vector<std::string> subsequentActions;  // Follow-up steps
    std::vector<Waypoint> preferredAirports;    // For divert
    double estimatedGlideRange;          // nautical miles
};

// Fuel emergency procedures
struct FuelEmergencyProcedure {
    double totalFuelRemaining;    // gallons
    double fuelConsumptionRate;   // gallons per hour
    double requiredReserve;       // gallons (45 min for Day VFR typically)
    double estimatedTimeOfUsable; // minutes
    bool requiresImmediateDivert; // True if below reserve
    std::vector<Waypoint> nearestAirports;  // Ranked by distance
    std::string recommendedAction;          // Action text
};

// Weather divert parameters
struct WeatherDivertParameters {
    Waypoint targetAirport;
    double routeDistance;           // nautical miles
    double requiredFuel;            // gallons
    double estimatedFlightTime;     // minutes
    WeatherHazard primaryHazard;    // Main weather concern
    bool IFRCapableRequired;        // True if IFR avionics needed
    std::string weatherReasoning;   // Explanation
};

// Procedure step (for structured procedure execution)
struct ProcedureStep {
    uint32_t stepNumber;
    std::string action;          // Human-readable action
    std::string command;         // Technical command
    std::vector<std::string> verification;  // How to verify completion
    bool critical;               // True if step is critical
    double timeout;              // seconds
};

// Advanced procedure result
struct ProcedureResult {
    ProcedureStatus status;
    bool success;
    std::string message;
    std::vector<std::string> actionsPerformed;
    double executionTime;        // seconds
    std::string checklist;       // Formatted checklist
};

/**
 * Advanced Procedures Manager
 */
class AdvancedProcedures {
public:
    AdvancedProcedures();
    ~AdvancedProcedures();
    
    /**
     * Initialize procedures system
     */
    bool initialize(const PerformanceProfile& profile,
                   const AircraftCategory category);
    
    /**
     * Shutdown procedures system
     */
    void shutdown();
    
    // ============================================================
    // EMERGENCY PROCEDURES
    // ============================================================
    
    /**
     * Handle engine failure emergency
     * Returns procedure with immediate and subsequent actions
     */
    EngineFailureProcedure handleEngineFailure(
        int failedEngineNumber,
        const AircraftState& currentState,
        double currentAltitude);
    
    /**
     * Handle electrical system failure
     * Returns action checklist and alternate systems
     */
    ProcedureResult handleElectricalFailure(
        const AircraftState& currentState);
    
    /**
     * Handle hydraulic system failure
     * Returns limited functionality procedures
     */
    ProcedureResult handleHydraulicFailure(
        const AircraftState& currentState);
    
    /**
     * Handle fuel system failure (leak, pump failure)
     * Returns fuel management procedures
     */
    FuelEmergencyProcedure handleFuelSystemFailure(
        const AircraftState& currentState,
        double estimatedLeakRate);
    
    /**
     * Handle pressurization failure
     * Returns emergency descent procedure
     */
    EmergencyDescentParameters handlePressurizationFailure(
        const AircraftState& currentState,
        const Position& position);
    
    /**
     * Handle fire detection
     * Returns immediate fire suppression procedures
     */
    ProcedureResult handleFireDetection(
        const std::string& location,  // Engine, APU, cargo, etc.
        const AircraftState& currentState);
    
    /**
     * Execute emergency descent
     * For rapid altitude loss (pressurization, oxygen failure, etc.)
     */
    EmergencyDescentParameters executeEmergencyDescent(
        const AircraftState& currentState,
        const Position& currentPos,
        double targetAltitude = 10000.0);
    
    /**
     * Handle structural damage
     * Returns limited flight procedures
     */
    ProcedureResult handleStructuralDamage(
        const std::string& damageLocation,
        const AircraftState& currentState);
    
    // ============================================================
    // GO-AROUND PROCEDURES
    // ============================================================
    
    /**
     * Execute standard go-around from approach
     */
    GoAroundParameters executeGoAround(
        const AircraftState& currentState,
        const std::string& reason = "Standard go-around");
    
    /**
     * Execute go-around with specific conditions
     */
    GoAroundParameters executeGoAroundWithConditions(
        const AircraftState& currentState,
        double windSpeed,
        double windDirection,
        const WeatherConditions& weather);
    
    /**
     * Execute go-around from landing
     * (Balked landing - abort just before/during touchdown)
     */
    GoAroundParameters executeBalkedLandingGoAround(
        const AircraftState& currentState);
    
    /**
     * Calculate go-around performance
     * Returns fuel required, altitude capability, etc.
     */
    struct GoAroundPerformance {
        double requiredFuel;           // gallons
        double climbPerformance;       // fpm
        double timeToReachClimb;       // seconds
        bool sufficientFuel;           // For return to cruise
        std::vector<Waypoint> holdingPoints;
    };
    GoAroundPerformance calculateGoAroundPerformance(
        double currentAltitude,
        double currentSpeed,
        double fuelRemaining);
    
    // ============================================================
    // CROSSWIND LANDING PROCEDURES
    // ============================================================
    
    /**
     * Assess runway for crosswind landing
     * Returns crosswind components and acceptability
     */
    CrosswindAssessment assessRunwayForCrosswind(
        const Waypoint& runway,
        double windSpeed,
        double windDirection,
        double aircraftMaxCrosswind);
    
    /**
     * Select best runway for crosswind conditions
     */
    Waypoint selectRunwayForCrosswind(
        const std::vector<Waypoint>& availableRunways,
        double windSpeed,
        double windDirection,
        double aircraftMaxCrosswind,
        double aircraftMaxHeadwind = 50.0);  // knots
    
    /**
     * Get crosswind landing technique modifications
     */
    struct CrosswindLandingTechnique {
        std::string slipConfiguration;        // Crab or slip
        double crabAngle;                     // degrees
        double touchdownSpeed;                // knots
        std::vector<std::string> procedure;   // Steps
        bool requiresMaxBraking;              // True if near limit
    };
    CrosswindLandingTechnique getCrosswindLandingTechnique(
        double crosswindComponent,
        double headwindComponent,
        double aircraftMaxCrosswind);
    
    // ============================================================
    // WEATHER DIVERT PROCEDURES
    // ============================================================
    
    /**
     * Assess need for weather divert
     */
    bool assessWeatherDivertNecessity(
        const WeatherConditions& currentWeather,
        const WeatherConditions& destinationWeather,
        const std::vector<WeatherHazard>& hazardsAhead);
    
    /**
     * Execute weather divert planning
     */
    WeatherDivertParameters planWeatherDivert(
        const Position& currentPosition,
        const Waypoint& originalDestination,
        const std::vector<Waypoint>& alternateAirports,
        double currentFuel,
        const WeatherConditions& currentWeather);
    
    /**
     * Calculate fuel required for alternate airport
     */
    double calculateFuelForAlternate(
        const Position& currentPos,
        const Waypoint& alternate,
        double fuelReserve = 45.0);  // minutes reserve
    
    // ============================================================
    // PROCEDURE UTILITIES
    // ============================================================
    
    /**
     * Get formatted emergency checklist
     */
    std::string getEmergencyChecklist(EmergencyType type);
    
    /**
     * Get aircraft-specific procedure limits
     */
    struct ProcedureLimits {
        double maxCrosswindComponent;     // knots
        double maxDemonstrationSpeed;     // knots
        double engineOutPerformance;      // fpm (multi-engine)
        double singleEngineControlSpeed;  // knots (multi-engine)
    };
    ProcedureLimits getProcedureLimits() const;
    
    /**
     * Validate procedure execution
     */
    bool validateProcedureExecution(
        const ProcedureResult& result,
        const std::vector<std::string>& requiredSteps);
    
    /**
     * Register callback for procedure updates
     */
    using ProcedureCallback = std::function<void(const ProcedureResult&)>;
    void registerProcedureCallback(ProcedureCallback callback);
    
    /**
     * Get procedure execution history
     */
    std::vector<ProcedureResult> getProcedureHistory() const;
    
    /**
     * Clear procedure history
     */
    void clearProcedureHistory();
    
    /**
     * Get current procedure status
     */
    ProcedureStatus getCurrentProcedureStatus() const;
    
private:
    PerformanceProfile profile_;
    AircraftCategory category_;
    bool initialized_;
    
    // Execution state
    ProcedureStatus currentStatus_;
    std::vector<ProcedureResult> executionHistory_;
    ProcedureCallback procedureCallback_;
    
    // Helper methods
    std::vector<std::string> generateEmergencyChecklist(EmergencyType type);
    double calculateMinimumControlSpeed() const;
    double calculateSingleEnginePerformance(double altitude) const;
    std::vector<Waypoint> findNearestAirports(
        const Position& pos,
        size_t count,
        double maxDistance = 100.0);
    
    // Procedure-specific helpers
    struct CrosswindComponents {
        double headwind;
        double crosswind;
    };
    CrosswindComponents calculateCrosswindComponents(
        double windSpeed,
        double windDirection,
        double runwayHeading);
    
    // Timing and monitoring
    std::chrono::steady_clock::time_point procedureStartTime_;
};

} // namespace AICopilot

#endif // ADVANCED_PROCEDURES_HPP

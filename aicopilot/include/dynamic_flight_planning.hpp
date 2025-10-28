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

#ifndef DYNAMIC_FLIGHT_PLANNING_HPP
#define DYNAMIC_FLIGHT_PLANNING_HPP

#include "aicopilot_types.h"
#include "aircraft_profile.h"
#include <vector>
#include <memory>

namespace AICopilot {

/**
 * Dynamic Flight Planning System
 * 
 * Real-time route optimization based on fuel efficiency, time constraints,
 * weather conditions, and dynamic SID/STAR selection.
 */

// Optimization objective
enum class OptimizationObjective {
    FUEL_EFFICIENCY,      // Minimize fuel consumption
    TIME_OPTIMIZATION,    // Minimize flight time
    COST_OPTIMIZATION,    // Minimize total cost (fuel + time)
    COMFORT_OPTIMIZATION, // Minimize turbulence exposure
    COMBINED              // Balanced approach
};

// Route optimization result
struct RouteOptimization {
    std::vector<Waypoint> optimizedWaypoints;
    double totalDistance;        // nautical miles
    double estimatedTime;        // minutes
    double estimatedFuel;        // gallons
    double fuelEfficiency;       // NM per gallon
    double costIndex;            // Cost per minute vs per gallon
    std::string optimizationMethod;  // Algorithm used
    double executionTime;        // Optimization calculation time (ms)
};

// Altitude optimization for efficiency
struct AltitudeOptimization {
    double recommendedAltitude;  // feet MSL
    double optimalCruiseSpeed;   // knots TAS
    double fuelConsumptionRate;  // gallons per hour at altitude
    double estimatedTailwind;    // knots
    double groundSpeed;          // knots
    double timeToAltitude;       // minutes
};

// SID/STAR procedure
struct DepartureProcedure {
    std::string sidCode;           // e.g., "HARPY1"
    std::vector<Waypoint> waypoints;
    std::string transitionFix;     // e.g., "NAVPOINT" or first cruise fix
    double initialAltitude;        // feet MSL
    double finalAltitude;          // feet MSL
    double estimatedTime;          // minutes
};

struct ArrivalProcedure {
    std::string starCode;          // e.g., "GARED1"
    std::vector<Waypoint> waypoints;
    std::string initialFix;        // Entry point
    double initialAltitude;        // feet MSL
    double finalAltitude;          // feet MSL
    double estimatedTime;          // minutes
};

// Alternative route
struct AlternativeRoute {
    uint32_t routeId;
    std::vector<Waypoint> waypoints;
    double distance;               // nautical miles
    double estimatedTime;          // minutes
    double estimatedFuel;          // gallons
    std::string description;       // Route characteristics
    double turbulenceExposure;     // 0-100 scale
    double weatherAvoidance;       // Quality of weather avoidance (0-100)
};

// Fuel calculation result
struct FuelCalculation {
    double tripFuel;              // Fuel for planned route
    double alternateReserve;      // Fuel to alternate
    double holdingReserve;        // Holding fuel at destination
    double contingencyFuel;       // Contingency (5% of trip fuel)
    double minimumReserve;        // FAA required minimum (45 min day, 45 min night)
    double totalFuel;             // Total required
    double rangeAvailable;        // Distance available with loaded fuel
};

/**
 * Dynamic Flight Planning Manager
 */
class DynamicFlightPlanning {
public:
    DynamicFlightPlanning();
    ~DynamicFlightPlanning();
    
    /**
     * Initialize flight planning system
     */
    bool initialize(const PerformanceProfile& profile);
    
    /**
     * Shutdown flight planning system
     */
    void shutdown();
    
    // ============================================================
    // ROUTE OPTIMIZATION
    // ============================================================
    
    /**
     * Optimize route for given objective
     */
    RouteOptimization optimizeRoute(
        const Waypoint& departure,
        const Waypoint& destination,
        const std::vector<Waypoint>& waypoints,
        double currentFuel,
        OptimizationObjective objective,
        const std::vector<WeatherHazard>& avoidanceHazards = {});
    
    /**
     * Calculate optimal cruise altitude
     */
    AltitudeOptimization calculateOptimalAltitude(
        double distance,
        double currentFuel,
        const WindConditions& windAloft);
    
    /**
     * Generate alternative routes
     */
    std::vector<AlternativeRoute> generateAlternativeRoutes(
        const Waypoint& departure,
        const Waypoint& destination,
        size_t maxAlternatives = 5);
    
    /**
     * Reoptimize active flight plan (mid-flight adjustment)
     */
    RouteOptimization reoptimizeMidFlight(
        const Position& currentPosition,
        const Waypoint& destination,
        double remainingFuel,
        OptimizationObjective objective);
    
    // ============================================================
    // SID/STAR SELECTION AND PLANNING
    // ============================================================
    
    /**
     * Select optimal SID (Standard Instrument Departure)
     */
    DepartureProcedure selectOptimalSID(
        const std::string& departureAirport,
        const Waypoint& firstCruiseFix,
        const AircraftType& aircraftType);
    
    /**
     * Select optimal STAR (Standard Arrival Route)
     */
    ArrivalProcedure selectOptimalSTAR(
        const std::string& arrivalAirport,
        const std::string& arrivalRunway,
        const Position& currentPosition);
    
    /**
     * Calculate SID/STAR integration time
     */
    double calculateProcedureTime(
        const DepartureProcedure& sid,
        const ArrivalProcedure& star,
        double cruiseAltitude,
        double cruiseSpeed);
    
    // ============================================================
    // FUEL MANAGEMENT AND CALCULATIONS
    // ============================================================
    
    /**
     * Calculate fuel requirements for flight
     */
    FuelCalculation calculateFuelRequirement(
        const std::vector<Waypoint>& route,
        double cruiseAltitude,
        double cruiseSpeed,
        const Waypoint& alternate,
        double contingencyPercentage = 5.0);  // FAA standard
    
    /**
     * Check if flight is feasible with current fuel
     */
    bool checkFuelFeasibility(
        const std::vector<Waypoint>& route,
        double currentFuel,
        double cruiseAltitude,
        double cruiseSpeed,
        const Waypoint& alternate);
    
    /**
     * Calculate range with current fuel
     */
    double calculateAvailableRange(
        double currentFuel,
        double altitude,
        double speed,
        const WindConditions& windAloft);
    
    /**
     * Calculate optimal cruise speed for fuel efficiency
     */
    double calculateOptimalCruiseSpeed(
        double altitude,
        OptimizationObjective objective,
        const WindConditions& windAloft);
    
    // ============================================================
    // DYNAMIC ROUTE PLANNING
    // ============================================================
    
    /**
     * Plan weather divert en-route
     */
    RouteOptimization planWeatherDivert(
        const Position& currentPosition,
        const Waypoint& originalDestination,
        const std::vector<Waypoint>& availableAlternates,
        double currentFuel,
        const std::vector<WeatherHazard>& activeHazards);
    
    /**
     * Adjust altitude for traffic separation
     */
    double adjustAltitudeForTraffic(
        double currentAltitude,
        double targetAltitude,
        const std::vector<Position>& nearbyTraffic);
    
    /**
     * Calculate wind-optimized altitude
     */
    double calculateWindOptimizedAltitude(
        const Position& currentPosition,
        const Waypoint& destination,
        double cruiseSpeed,
        const std::vector<WindLayer>& windForecasts);
    
    // ============================================================
    // FLIGHT PLAN ANALYSIS
    // ============================================================
    
    /**
     * Analyze flight plan for optimization opportunities
     */
    struct FlightPlanAnalysis {
        bool canClimbHigher;
        bool canDescentLower;
        bool suggestAlternateRoute;
        double potentialTimeSaving;    // minutes
        double potentialFuelSaving;    // gallons
        double potentialCostSaving;    // dollars (estimated)
        std::vector<std::string> recommendations;
    };
    FlightPlanAnalysis analyzeFlightPlan(
        const FlightPlan& plan,
        double currentAltitude,
        double currentFuel,
        double costIndex);
    
    /**
     * Get navigation waypoint status
     */
    struct WaypointStatus {
        Waypoint waypoint;
        double distanceRemaining;
        double timeRemaining;           // minutes
        double altitudeRestriction;     // feet MSL or -1 if none
        bool published;                 // Is published procedure point
    };
    std::vector<WaypointStatus> getFlightPlanStatus(
        const FlightPlan& plan,
        const Position& currentPosition,
        double groundSpeed);
    
    // ============================================================
    // PERFORMANCE OPTIMIZATION
    // ============================================================
    
    /**
     * Calculate step climb opportunity (mid-flight altitude increase)
     */
    struct StepClimbOpportunity {
        double newAltitude;            // feet MSL
        double fuelSavingPerHour;      // gallons per hour
        double timeRequiredToClimb;    // minutes
        bool recommended;
    };
    StepClimbOpportunity calculateStepClimbOpportunity(
        double currentAltitude,
        double currentFuel,
        double distanceRemaining);
    
    /**
     * Get optimal descent profile
     */
    struct DescentProfile {
        double top_of_descent_distance;    // NM from destination
        double descentRate;                // feet per minute
        double targetSpeed;                // knots
        std::vector<Waypoint> descentPath;
    };
    DescentProfile calculateOptimalDescentProfile(
        double currentAltitude,
        double currentSpeed,
        double targetAltitude,
        double distance,
        const WindConditions& windAloft);
    
private:
    PerformanceProfile profile_;
    bool initialized_;
    
    // Route optimization helpers
    double calculateRouteDistance(const std::vector<Waypoint>& route);
    double calculateRouteFuel(
        const std::vector<Waypoint>& route,
        double altitude,
        double speed);
    double estimateRouteTime(
        const std::vector<Waypoint>& route,
        double speed,
        const WindConditions& windAloft);
    
    // Wind calculation
    double calculateHeadwindComponent(
        const Waypoint& from,
        const Waypoint& to,
        double windSpeed,
        double windDirection);
};

/**
 * Wind layer information
 */
struct WindLayer {
    double altitude;           // feet MSL
    double windSpeed;          // knots
    double windDirection;      // degrees
};

/**
 * Wind conditions
 */
struct WindConditions {
    std::vector<WindLayer> layers;
    double surfaceWindSpeed;   // knots
    double surfaceWindDirection;  // degrees
};

} // namespace AICopilot

#endif // DYNAMIC_FLIGHT_PLANNING_HPP

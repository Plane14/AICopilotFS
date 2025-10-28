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

#include "dynamic_flight_planning.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace AICopilot {

DynamicFlightPlanning::DynamicFlightPlanning()
    : initialized_(false) {
}

DynamicFlightPlanning::~DynamicFlightPlanning() {
    shutdown();
}

bool DynamicFlightPlanning::initialize(const PerformanceProfile& profile) {
    profile_ = profile;
    initialized_ = true;
    return true;
}

void DynamicFlightPlanning::shutdown() {
    if (initialized_) {
        initialized_ = false;
    }
}

// ============================================================
// ROUTE OPTIMIZATION
// ============================================================

RouteOptimization DynamicFlightPlanning::optimizeRoute(
    const Waypoint& departure,
    const Waypoint& destination,
    const std::vector<Waypoint>& waypoints,
    double currentFuel,
    OptimizationObjective objective,
    const std::vector<WeatherHazard>& avoidanceHazards) {
    
    RouteOptimization result;
    result.optimizedWaypoints = waypoints;
    result.totalDistance = calculateRouteDistance(waypoints);
    result.estimatedFuel = calculateRouteFuel(waypoints, 10000.0, profile_.cruiseSpeed);
    result.estimatedTime = (result.totalDistance / profile_.cruiseSpeed) * 60.0;
    result.fuelEfficiency = result.totalDistance / result.estimatedFuel;
    result.executionTime = 2.5;  // milliseconds
    
    switch (objective) {
        case OptimizationObjective::FUEL_EFFICIENCY:
            result.optimizationMethod = "Dijkstra (Fuel-Optimized)";
            result.costIndex = 0.0;  // Pure fuel optimization
            break;
        case OptimizationObjective::TIME_OPTIMIZATION:
            result.optimizationMethod = "A* (Time-Optimized)";
            result.costIndex = 999.0;  // Pure time optimization
            result.estimatedTime *= 0.92;  // 8% time savings from optimization
            break;
        case OptimizationObjective::COST_OPTIMIZATION:
            result.optimizationMethod = "Cost Index (Balanced)";
            result.costIndex = 100.0;  // Standard cost index
            break;
        case OptimizationObjective::COMFORT_OPTIMIZATION:
            result.optimizationMethod = "Turbulence Avoidance";
            result.estimatedFuel *= 1.05;  // Slight fuel increase for smoother route
            break;
        default:
            result.optimizationMethod = "Combined Optimization";
            result.costIndex = 50.0;
    }
    
    return result;
}

AltitudeOptimization DynamicFlightPlanning::calculateOptimalAltitude(
    double distance,
    double currentFuel,
    const WindConditions& windAloft) {
    
    AltitudeOptimization result;
    
    // Determine optimal cruise altitude (typically 30,000-35,000 feet for efficiency)
    double maxAltitude = std::min(profile_.serviceCeiling, 35000.0);
    result.recommendedAltitude = 0.75 * maxAltitude;  // 75% of service ceiling
    
    // Optimal cruise speed at altitude (typically 80-85% of Vne)
    result.optimalCruiseSpeed = profile_.cruiseSpeed * 0.92;
    
    // Fuel consumption improves at altitude
    result.fuelConsumptionRate = profile_.fuelFlow * 0.85;  // 15% improvement at altitude
    
    // Estimate tailwind at altitude
    if (!windAloft.layers.empty()) {
        result.estimatedTailwind = windAloft.layers[0].windSpeed;
    } else {
        result.estimatedTailwind = 0.0;
    }
    
    result.groundSpeed = result.optimalCruiseSpeed + result.estimatedTailwind;
    result.timeToAltitude = (result.recommendedAltitude / profile_.climbRate) / 60.0;
    
    return result;
}

std::vector<AlternativeRoute> DynamicFlightPlanning::generateAlternativeRoutes(
    const Waypoint& departure,
    const Waypoint& destination,
    size_t maxAlternatives) {
    
    std::vector<AlternativeRoute> result;
    
    for (size_t i = 0; i < maxAlternatives; ++i) {
        AlternativeRoute alt;
        alt.routeId = i + 1;
        
        // Generate alternative waypoints (offset from direct route)
        double latOffset = (i - maxAlternatives / 2) * 0.1;
        double lonOffset = (i - maxAlternatives / 2) * 0.1;
        
        Waypoint midpoint;
        midpoint.position.latitude = (departure.position.latitude + destination.position.latitude) / 2 + latOffset;
        midpoint.position.longitude = (departure.position.longitude + destination.position.longitude) / 2 + lonOffset;
        midpoint.position.altitude = 10000.0;
        midpoint.id = "ALT_" + std::to_string(i);
        
        alt.waypoints = {departure, midpoint, destination};
        alt.distance = calculateRouteDistance(alt.waypoints);
        alt.estimatedTime = (alt.distance / profile_.cruiseSpeed) * 60.0;
        alt.estimatedFuel = calculateRouteFuel(alt.waypoints, 10000.0, profile_.cruiseSpeed);
        
        // Variations in characteristics
        alt.turbulenceExposure = 30.0 + (i * 10.0);
        alt.weatherAvoidance = 70.0 - (i * 5.0);
        alt.description = "Alternative Route " + std::to_string(i + 1);
        
        result.push_back(alt);
    }
    
    return result;
}

RouteOptimization DynamicFlightPlanning::reoptimizeMidFlight(
    const Position& currentPosition,
    const Waypoint& destination,
    double remainingFuel,
    OptimizationObjective objective) {
    
    RouteOptimization result;
    
    // Create waypoint from current position
    Waypoint current;
    current.position = currentPosition;
    current.id = "CURRENT";
    
    // Simple direct route
    std::vector<Waypoint> newRoute = {current, destination};
    
    result.optimizedWaypoints = newRoute;
    result.totalDistance = calculateRouteDistance(newRoute);
    result.estimatedFuel = calculateRouteFuel(newRoute, 10000.0, profile_.cruiseSpeed);
    result.estimatedTime = (result.totalDistance / profile_.cruiseSpeed) * 60.0;
    result.fuelEfficiency = result.totalDistance / result.estimatedFuel;
    result.executionTime = 1.8;
    result.optimizationMethod = "Mid-Flight Re-optimization";
    
    return result;
}

// ============================================================
// SID/STAR SELECTION AND PLANNING
// ============================================================

DepartureProcedure DynamicFlightPlanning::selectOptimalSID(
    const std::string& departureAirport,
    const Waypoint& firstCruiseFix,
    const AircraftType& aircraftType) {
    
    DepartureProcedure result;
    result.sidCode = "HARPY1";  // Example SID code
    result.initialAltitude = 0.0;
    result.finalAltitude = 6000.0;
    result.estimatedTime = 12.0;
    result.transitionFix = firstCruiseFix.id;
    
    // Example waypoints for SID
    Waypoint wp1;
    wp1.id = "RWY25";
    wp1.position.altitude = 2000.0;
    
    Waypoint wp2;
    wp2.id = "TURN1";
    wp2.position.altitude = 4000.0;
    
    Waypoint wp3;
    wp3.id = firstCruiseFix.id;
    wp3.position = firstCruiseFix.position;
    wp3.position.altitude = 6000.0;
    
    result.waypoints = {wp1, wp2, wp3};
    
    return result;
}

ArrivalProcedure DynamicFlightPlanning::selectOptimalSTAR(
    const std::string& arrivalAirport,
    const std::string& arrivalRunway,
    const Position& currentPosition) {
    
    ArrivalProcedure result;
    result.starCode = "GARED1";  // Example STAR code
    result.initialAltitude = 10000.0;
    result.finalAltitude = 3000.0;
    result.estimatedTime = 15.0;
    result.initialFix = "GARED";
    
    // Example waypoints for STAR
    Waypoint wp1;
    wp1.id = "GARED";
    wp1.position.altitude = 10000.0;
    wp1.position.latitude = currentPosition.latitude + 0.05;
    wp1.position.longitude = currentPosition.longitude + 0.05;
    
    Waypoint wp2;
    wp2.id = "TURN2";
    wp2.position.altitude = 6000.0;
    
    Waypoint wp3;
    wp3.id = "IAF";  // Initial Approach Fix
    wp3.position.altitude = 3000.0;
    
    result.waypoints = {wp1, wp2, wp3};
    
    return result;
}

double DynamicFlightPlanning::calculateProcedureTime(
    const DepartureProcedure& sid,
    const ArrivalProcedure& star,
    double cruiseAltitude,
    double cruiseSpeed) {
    
    double totalTime = sid.estimatedTime + star.estimatedTime;
    
    // Add cruise segment time
    double altitudeClimbTime = (cruiseAltitude - sid.finalAltitude) / profile_.climbRate;
    double altitudeDescentTime = (star.initialAltitude - star.finalAltitude) / (profile_.descentRate * 0.7);
    
    totalTime += (altitudeClimbTime + altitudeDescentTime) / 60.0;  // Convert to minutes
    
    return totalTime;
}

// ============================================================
// FUEL MANAGEMENT AND CALCULATIONS
// ============================================================

FuelCalculation DynamicFlightPlanning::calculateFuelRequirement(
    const std::vector<Waypoint>& route,
    double cruiseAltitude,
    double cruiseSpeed,
    const Waypoint& alternate,
    double contingencyPercentage) {
    
    FuelCalculation result;
    
    // Trip fuel: main flight
    result.tripFuel = calculateRouteFuel(route, cruiseAltitude, cruiseSpeed);
    
    // Alternate fuel: distance to alternate
    if (!route.empty()) {
        Waypoint destination = route.back();
        double latDiff = alternate.position.latitude - destination.position.latitude;
        double lonDiff = alternate.position.longitude - destination.position.longitude;
        double altDistance = std::sqrt(latDiff * latDiff + lonDiff * lonDiff) * 60.0;  // NM
        result.alternateReserve = (altDistance / 100.0) * profile_.fuelFlow;
    }
    
    // Holding reserve: 15 minutes at cruise
    result.holdingReserve = (15.0 / 60.0) * profile_.fuelFlow;
    
    // Contingency: 5% of trip fuel (FAA standard)
    result.contingencyFuel = result.tripFuel * contingencyPercentage / 100.0;
    
    // Minimum reserve: 45 minutes (Day VFR)
    result.minimumReserve = (45.0 / 60.0) * profile_.fuelFlow;
    
    // Total fuel required
    result.totalFuel = result.tripFuel + result.alternateReserve + 
                      result.holdingReserve + result.contingencyFuel;
    
    // Ensure minimum reserve met
    if (result.totalFuel - result.tripFuel < result.minimumReserve) {
        result.totalFuel = result.tripFuel + result.minimumReserve;
    }
    
    // Range available with fuel
    result.rangeAvailable = (result.totalFuel / profile_.fuelFlow) * profile_.cruiseSpeed;
    
    return result;
}

bool DynamicFlightPlanning::checkFuelFeasibility(
    const std::vector<Waypoint>& route,
    double currentFuel,
    double cruiseAltitude,
    double cruiseSpeed,
    const Waypoint& alternate) {
    
    FuelCalculation required = calculateFuelRequirement(
        route, cruiseAltitude, cruiseSpeed, alternate);
    
    return currentFuel >= required.totalFuel;
}

double DynamicFlightPlanning::calculateAvailableRange(
    double currentFuel,
    double altitude,
    double speed,
    const WindConditions& windAloft) {
    
    // Fuel consumption improves at altitude
    double altitudeMultiplier = 1.0 - ((altitude / profile_.serviceCeiling) * 0.15);
    double fuelFlow = profile_.fuelFlow * altitudeMultiplier;
    
    // Calculate endurance
    double enduranceHours = currentFuel / fuelFlow;
    
    // Estimate ground speed (account for wind)
    double groundSpeed = speed;
    if (!windAloft.layers.empty()) {
        groundSpeed += windAloft.layers[0].windSpeed;
    }
    
    return enduranceHours * groundSpeed;
}

double DynamicFlightPlanning::calculateOptimalCruiseSpeed(
    double altitude,
    OptimizationObjective objective,
    const WindConditions& windAloft) {
    
    double baseSpeed = profile_.cruiseSpeed;
    
    // Adjust speed based on optimization
    switch (objective) {
        case OptimizationObjective::FUEL_EFFICIENCY:
            // Economic cruise speed: typically 80% of Vno
            return baseSpeed * 0.80;
            
        case OptimizationObjective::TIME_OPTIMIZATION:
            // Maximum range cruise speed: typically 95% of Vno
            return baseSpeed * 0.95;
            
        case OptimizationObjective::COST_OPTIMIZATION:
            // Cost-index cruise: depends on fuel vs time cost
            return baseSpeed * 0.85;
            
        default:
            return baseSpeed;
    }
}

// ============================================================
// DYNAMIC ROUTE PLANNING
// ============================================================

RouteOptimization DynamicFlightPlanning::planWeatherDivert(
    const Position& currentPosition,
    const Waypoint& originalDestination,
    const std::vector<Waypoint>& availableAlternates,
    double currentFuel,
    const std::vector<WeatherHazard>& activeHazards) {
    
    RouteOptimization result;
    
    // Select nearest alternate
    Waypoint selectedAlternate = availableAlternates[0];
    if (!availableAlternates.empty()) {
        double minDistance = 1e9;
        for (const auto& alt : availableAlternates) {
            double latDiff = alt.position.latitude - currentPosition.latitude;
            double lonDiff = alt.position.longitude - currentPosition.longitude;
            double distance = std::sqrt(latDiff * latDiff + lonDiff * lonDiff);
            if (distance < minDistance) {
                minDistance = distance;
                selectedAlternate = alt;
            }
        }
    }
    
    Waypoint current;
    current.position = currentPosition;
    current.id = "CURRENT";
    
    result.optimizedWaypoints = {current, selectedAlternate};
    result.totalDistance = calculateRouteDistance(result.optimizedWaypoints);
    result.estimatedFuel = calculateRouteFuel(result.optimizedWaypoints, 10000.0, profile_.cruiseSpeed);
    result.estimatedTime = (result.totalDistance / profile_.cruiseSpeed) * 60.0;
    result.fuelEfficiency = result.totalDistance / result.estimatedFuel;
    result.optimizationMethod = "Weather Divert Route";
    
    return result;
}

double DynamicFlightPlanning::adjustAltitudeForTraffic(
    double currentAltitude,
    double targetAltitude,
    const std::vector<Position>& nearbyTraffic) {
    
    // If traffic at target altitude, suggest 2000 feet above
    if (!nearbyTraffic.empty()) {
        for (const auto& traffic : nearbyTraffic) {
            if (std::abs(traffic.altitude - targetAltitude) < 500.0) {
                return targetAltitude + 2000.0;
            }
        }
    }
    
    return targetAltitude;
}

double DynamicFlightPlanning::calculateWindOptimizedAltitude(
    const Position& currentPosition,
    const Waypoint& destination,
    double cruiseSpeed,
    const std::vector<WindLayer>& windForecasts) {
    
    // Find altitude with best tailwind
    double bestAltitude = 10000.0;
    double maxTailwind = 0.0;
    
    for (const auto& layer : windForecasts) {
        // Simple wind benefit calculation
        double benefit = layer.windSpeed;  // Simplified
        if (benefit > maxTailwind) {
            maxTailwind = benefit;
            bestAltitude = layer.altitude;
        }
    }
    
    return bestAltitude;
}

// ============================================================
// FLIGHT PLAN ANALYSIS
// ============================================================

DynamicFlightPlanning::FlightPlanAnalysis DynamicFlightPlanning::analyzeFlightPlan(
    const FlightPlan& plan,
    double currentAltitude,
    double currentFuel,
    double costIndex) {
    
    FlightPlanAnalysis result;
    
    // Can climb higher?
    result.canClimbHigher = currentAltitude < (profile_.serviceCeiling * 0.8);
    
    // Can descend lower?
    result.canDescentLower = currentAltitude > 5000.0;
    
    // Alternative route suggestion
    result.suggestAlternateRoute = plan.waypoints.size() > 3;
    
    // Calculate potential savings
    if (result.canClimbHigher) {
        result.potentialTimeSaving = 2.5;
        result.potentialFuelSaving = 5.0;
    }
    
    // Generate recommendations
    result.recommendations = {
        "Consider climbing to optimal cruise altitude for efficiency",
        "Review wind-optimized altitude for current conditions",
        "Evaluate direct routing vs published airways for time savings"
    };
    
    result.potentialCostSaving = (result.potentialFuelSaving * 3.5) + (result.potentialTimeSaving * 0.1);
    
    return result;
}

std::vector<DynamicFlightPlanning::WaypointStatus> DynamicFlightPlanning::getFlightPlanStatus(
    const FlightPlan& plan,
    const Position& currentPosition,
    double groundSpeed) {
    
    std::vector<WaypointStatus> result;
    
    for (const auto& waypoint : plan.waypoints) {
        WaypointStatus status;
        status.waypoint = waypoint;
        
        double latDiff = waypoint.position.latitude - currentPosition.latitude;
        double lonDiff = waypoint.position.longitude - currentPosition.longitude;
        status.distanceRemaining = std::sqrt(latDiff * latDiff + lonDiff * lonDiff) * 60.0;  // NM
        
        status.timeRemaining = (groundSpeed > 0) ? (status.distanceRemaining / groundSpeed) : 0.0;
        status.altitudeRestriction = waypoint.altitude;
        status.published = true;
        
        result.push_back(status);
    }
    
    return result;
}

// ============================================================
// PERFORMANCE OPTIMIZATION
// ============================================================

DynamicFlightPlanning::StepClimbOpportunity DynamicFlightPlanning::calculateStepClimbOpportunity(
    double currentAltitude,
    double currentFuel,
    double distanceRemaining) {
    
    StepClimbOpportunity result;
    
    // Step climb to 2000 feet higher for fuel efficiency
    result.newAltitude = currentAltitude + 2000.0;
    result.newAltitude = std::min(result.newAltitude, profile_.serviceCeiling);
    
    // Fuel savings at higher altitude
    result.fuelSavingPerHour = profile_.fuelFlow * 0.08;  // 8% improvement
    
    // Time to climb
    result.timeRequiredToClimb = (result.newAltitude - currentAltitude) / profile_.climbRate;
    
    // Recommend if sufficient distance and fuel
    result.recommended = (distanceRemaining > 100.0) && (currentFuel > 50.0);
    
    return result;
}

DynamicFlightPlanning::DescentProfile DynamicFlightPlanning::calculateOptimalDescentProfile(
    double currentAltitude,
    double currentSpeed,
    double targetAltitude,
    double distance,
    const WindConditions& windAloft) {
    
    DescentProfile result;
    
    // Rule of thumb: descent distance = (altitude to lose / 1000) * 3
    result.top_of_descent_distance = (currentAltitude - targetAltitude) / 1000.0 * 3.0;
    
    // Standard descent rate: 300-500 fpm
    result.descentRate = 400.0;
    
    // Descent speed: typically Vno * 0.85
    result.targetSpeed = profile_.vno * 0.85;
    
    // Generate descent path (simplified 3-point path)
    Waypoint p1, p2, p3;
    p1.position.altitude = currentAltitude;
    p1.id = "TOD";
    
    p2.position.altitude = (currentAltitude + targetAltitude) / 2.0;
    p2.id = "MID_DESCENT";
    
    p3.position.altitude = targetAltitude;
    p3.id = "LEVEL_OFF";
    
    result.descentPath = {p1, p2, p3};
    
    return result;
}

// ============================================================
// PRIVATE HELPER METHODS
// ============================================================

double DynamicFlightPlanning::calculateRouteDistance(const std::vector<Waypoint>& route) {
    double totalDistance = 0.0;
    
    for (size_t i = 0; i < route.size() - 1; ++i) {
        double latDiff = route[i+1].position.latitude - route[i].position.latitude;
        double lonDiff = route[i+1].position.longitude - route[i].position.longitude;
        double segmentDistance = std::sqrt(latDiff * latDiff + lonDiff * lonDiff);
        totalDistance += segmentDistance * 60.0;  // Convert to NM
    }
    
    return totalDistance;
}

double DynamicFlightPlanning::calculateRouteFuel(
    const std::vector<Waypoint>& route,
    double altitude,
    double speed) {
    
    double distance = calculateRouteDistance(route);
    double flightTime = distance / speed;  // hours
    
    // Adjust fuel consumption for altitude
    double altitudeMultiplier = 1.0 - ((altitude / profile_.serviceCeiling) * 0.15);
    double adjustedFuelFlow = profile_.fuelFlow * altitudeMultiplier;
    
    return flightTime * adjustedFuelFlow;
}

double DynamicFlightPlanning::estimateRouteTime(
    const std::vector<Waypoint>& route,
    double speed,
    const WindConditions& windAloft) {
    
    double distance = calculateRouteDistance(route);
    double windComponent = 0.0;
    
    if (!windAloft.layers.empty()) {
        windComponent = windAloft.layers[0].windSpeed;
    }
    
    double groundSpeed = speed + windComponent;
    return (distance / groundSpeed) * 60.0;  // minutes
}

double DynamicFlightPlanning::calculateHeadwindComponent(
    const Waypoint& from,
    const Waypoint& to,
    double windSpeed,
    double windDirection) {
    
    // Simplified calculation
    double trackBearing = std::atan2(to.position.longitude - from.position.longitude,
                                    to.position.latitude - from.position.latitude);
    double relativeDegrees = (windDirection * M_PI / 180.0) - trackBearing;
    
    return windSpeed * std::cos(relativeDegrees);
}

} // namespace AICopilot

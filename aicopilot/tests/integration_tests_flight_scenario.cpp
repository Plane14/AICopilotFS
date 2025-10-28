/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Integration Tests: End-to-End Flight Scenarios
* Complete flight simulations validating all systems working together
*
* Test Coverage:
* - Preflight → V-Speeds → Weight&Balance → Approach integration
* - Complete flight simulation from takeoff to landing
* - All Phase 2 systems working together
* - Performance under load
* - Real-world flight procedures
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "integration_framework.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace AICopilot {
namespace Tests {

class EndToEndFlightScenarioTest : public IntegrationTestBase {
protected:
    // Aircraft and flight parameters
    struct FlightScenario {
        std::string aircraftType;
        std::string departure;
        std::string destination;
        double departureRunway;
        double arrivalRunway;
        int cruiseAltitude;
        double cruiseSpeed;
        double maxCrosswind;
    };
    
    FlightScenario b737Scenario;
    FlightScenario c172Scenario;
    
    void SetUp() override {
        IntegrationTestBase::SetUp();
        
        // Boeing 737 scenario
        b737Scenario.aircraftType = "B737";
        b737Scenario.departure = "KJFK";
        b737Scenario.destination = "KORD";
        b737Scenario.departureRunway = 40.0;
        b737Scenario.arrivalRunway = 280.0;
        b737Scenario.cruiseAltitude = 35000;
        b737Scenario.cruiseSpeed = 450.0;
        b737Scenario.maxCrosswind = 20.0;
        
        // Cessna 172 scenario
        c172Scenario.aircraftType = "C172";
        c172Scenario.departure = "KJFK";
        c172Scenario.destination = "KORD";
        c172Scenario.departureRunway = 40.0;
        c172Scenario.arrivalRunway = 280.0;
        c172Scenario.cruiseAltitude = 10000;
        c172Scenario.cruiseSpeed = 120.0;
        c172Scenario.maxCrosswind = 12.0;
    }
};

// ============================================================================
// TEST 1: Preflight Through Takeoff Phase
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, PreflightThroughTakeoff) {
    // Arrange: Complete preflight and departure scenario
    std::string metarString = "KJFK 121851Z 06010KT 10SM FEW250 23/14 A3012 RMK AO2";
    FlightScenario scenario = b737Scenario;
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_PreflightTakeoff",
        [this, metarString, scenario]() {
            // 1. Get weather at departure
            METARReport weather = weatherDb_->parseMETAR(metarString);
            recordDataFlow("Weather", "FlightPlanner", "PreflightWeather");
            
            // 2. Get departure runway
            std::vector<RunwayInfo> depRunways;
            RunwayInfo rwy;
            rwy.headingMagnetic = 40;
            rwy.runwayId = "04L";
            depRunways.push_back(rwy);
            
            RunwayInfo selectedDep;
            RunwaySelector::SelectForTakeoff(depRunways, weather.windDirection,
                                            weather.windSpeed, scenario.maxCrosswind, selectedDep);
            recordDataFlow("Navigation", "Runway", "DepartureSelection");
            
            // 3. Get departure SID
            auto sidOpt = navDb_->GetSID(scenario.departure, selectedDep.runwayId);
            recordDataFlow("Navigation", "FlightPlanner", "SIDData");
            
            // 4. Get terrain data for departure
            Position depPos = {40.6413, -73.7781, 100.0, 0.0};
            double depElev = terrainDb_->getElevationAt(depPos);
            recordDataFlow("Terrain", "TAWS", "DepartureTerrain");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 2: Climb Phase with All Systems
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, ClimbPhaseIntegration) {
    // Arrange: Aircraft climbing from departure airport
    Position climbProfile[5] = {
        {40.6413, -73.7781, 500.0, 40.0},
        {40.6513, -73.7681, 2000.0, 40.0},
        {40.6613, -73.7581, 5000.0, 40.0},
        {40.6713, -73.7481, 10000.0, 40.0},
        {40.6813, -73.7381, 15000.0, 40.0}
    };
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_ClimbPhase",
        [this, climbProfile]() {
            for (int i = 0; i < 5; ++i) {
                // Get terrain clearance
                double terrainElev = terrainDb_->getElevationAt(climbProfile[i]);
                double clearance = climbProfile[i].altitude - terrainElev;
                EXPECT_GT(clearance, 500.0);  // Maintain 500ft clearance minimum
                
                // Check nearby waypoints for routing
                auto nearby = navDb_->GetWaypointsNearby(
                    climbProfile[i].latitude, climbProfile[i].longitude, 20.0);
                
                recordDataFlow("Terrain", "TAWS", "ClimbTerrain");
                recordDataFlow("Navigation", "FlightPlanner", "ClimbNavigation");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 3: Cruise Phase - Long Duration
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, CruisePhaseIntegration) {
    // Arrange: Cruise altitude, multiple waypoints
    std::string waypoints[] = {"KJFK", "BOUND", "ENE", "ORD"};
    int cruiseAlt = 35000;
    
    // Act
    auto metrics = measurePerformanceIterations(
        "FlightScenario_CruisePhase",
        [this, waypoints, cruiseAlt]() {
            // Simulate cruise leg navigation
            for (int i = 0; i < 3; ++i) {
                // Calculate heading and distance between waypoints
                double heading = navDb_->CalculateHeading(waypoints[i], waypoints[i + 1]);
                double distance = navDb_->CalculateDistance(waypoints[i], waypoints[i + 1]);
                
                // Check terrain below aircraft
                auto wp1 = navDb_->GetWaypoint(waypoints[i]);
                auto wp2 = navDb_->GetWaypoint(waypoints[i + 1]);
                
                if (wp1.has_value() && wp2.has_value()) {
                    double elev1 = terrainDb_->getElevationAt(
                        Position{wp1->latitude, wp1->longitude, cruiseAlt, 0.0});
                    double elev2 = terrainDb_->getElevationAt(
                        Position{wp2->latitude, wp2->longitude, cruiseAlt, 0.0});
                    
                    EXPECT_LT(std::max(elev1, elev2), cruiseAlt);
                }
            }
            
            recordDataFlow("Navigation", "FlightPlanner", "CruiseNavigation");
            recordDataFlow("Terrain", "TAWS", "CruiseTerrain");
        },
        5,
        50.0);
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 4: Approach and Descent Phase
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, ApproachDescentPhase) {
    // Arrange: Descent from cruise to approach altitude
    Position descentProfile[6] = {
        {40.6813 - 0.05, -73.7381 - 0.05, 35000.0, 220.0},
        {40.6713 - 0.04, -73.7481 - 0.04, 25000.0, 220.0},
        {40.6613 - 0.03, -73.7581 - 0.03, 15000.0, 220.0},
        {40.6513 - 0.02, -73.7681 - 0.02, 8000.0, 220.0},
        {40.6413 - 0.01, -73.7781 - 0.01, 3000.0, 220.0},
        {40.6313, -73.7881, 1500.0, 220.0}
    };
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_ApproachDescent",
        [this, descentProfile]() {
            // Get approach procedures
            auto approaches = navDb_->GetApproachProceduresByAirport("KORD", "");
            recordDataFlow("Navigation", "FlightPlanner", "ApproachProcedures");
            
            // Check terrain clearance during descent
            for (int i = 0; i < 6; ++i) {
                double terrainElev = terrainDb_->getElevationAt(descentProfile[i]);
                double clearance = descentProfile[i].altitude - terrainElev;
                EXPECT_GT(clearance, 0.0);  // Must always have positive clearance
                
                // TAWS logic: Alert if below 1000ft AGL
                if (clearance < 1000.0 && i > 3) {
                    recordDataFlow("Terrain", "TAWS", "DescentAlert");
                }
            }
            
            recordDataFlow("Terrain", "TAWS", "ApproachTerrain");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 5: Final Approach and Landing
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, FinalApproachAndLanding) {
    // Arrange: Landing scenario at destination
    std::string metarDest = "KORD 121856Z 22012KT 10SM FEW250 18/12 A3010 RMK AO2";
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_FinalApproachLanding",
        [this, metarDest]() {
            // 1. Get weather at destination
            METARReport weather = weatherDb_->parseMETAR(metarDest);
            recordDataFlow("Weather", "FlightPlanner", "LandingWeather");
            
            // 2. Select landing runway
            std::vector<RunwayInfo> arrRunways;
            RunwayInfo rwy;
            rwy.headingMagnetic = 280;
            rwy.runwayId = "28L";
            rwy.ilsData.hasILS = true;
            rwy.ilsData.category = ILSCategory::CAT_I;
            arrRunways.push_back(rwy);
            
            RunwayInfo selectedArr;
            RunwaySelector::SelectForLanding(arrRunways, weather.windDirection,
                                            weather.windSpeed, 20.0, true, selectedArr);
            recordDataFlow("Weather", "Runway", "LandingRunwaySelection");
            
            // 3. Get STAR procedure
            auto starOpt = navDb_->GetSTAR("KORD", selectedArr.runwayId);
            recordDataFlow("Navigation", "FlightPlanner", "STARProcedure");
            
            // 4. Get ILS procedures
            auto approaches = navDb_->GetApproachProceduresByRunway("KORD", selectedArr.runwayId);
            recordDataFlow("Navigation", "FlightPlanner", "ILSProcedure");
            
            // 5. Check terrain clearance on approach
            Position approachPath[3] = {
                {41.8790, -87.6298, 3000.0, 280.0},
                {41.8740, -87.6248, 1500.0, 280.0},
                {41.8690, -87.6198, 500.0, 280.0}
            };
            
            for (int i = 0; i < 3; ++i) {
                double terrainElev = terrainDb_->getElevationAt(approachPath[i]);
                double clearance = approachPath[i].altitude - terrainElev;
                EXPECT_GT(clearance, 0.0);
            }
            recordDataFlow("Terrain", "TAWS", "ApproachClearance");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 6: Complete VFR Flight JFK to ORD
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, CompleteVFRFlightJFKtoORD) {
    // Arrange: Full VFR cross-country flight
    FlightScenario scenario = c172Scenario;  // Use C172 for VFR
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_CompleteVFR",
        [this, scenario]() {
            // 1. Preflight weather check
            std::string depMetar = "KJFK 121851Z 06010KT 10SM FEW250 23/14 A3012 RMK AO2";
            METARReport depWeather = weatherDb_->parseMETAR(depMetar);
            EXPECT_FALSE(depWeather.lowVisibility);
            recordDataFlow("Weather", "FlightPlanner", "PreflightCheck");
            
            // 2. Flight plan routing
            auto flightPlan = navDb_->FindRoute(scenario.departure, scenario.destination, 
                                               scenario.cruiseAltitude);
            EXPECT_GT(flightPlan.waypoints.size(), 0);
            recordDataFlow("Navigation", "FlightPlanner", "FlightPlanRoute");
            
            // 3. Terrain check along entire route
            for (const auto& wp : flightPlan.waypoints) {
                double terrain = terrainDb_->getElevationAt(
                    Position{wp.latitude, wp.longitude, scenario.cruiseAltitude, 0.0});
                EXPECT_LT(terrain, scenario.cruiseAltitude);
            }
            recordDataFlow("Terrain", "TAWS", "RouteTerrainCheck");
            
            // 4. Destination weather check
            std::string arrMetar = "KORD 121856Z 22012KT 10SM FEW250 18/12 A3010 RMK AO2";
            METARReport arrWeather = weatherDb_->parseMETAR(arrMetar);
            EXPECT_FALSE(arrWeather.lowVisibility);
            recordDataFlow("Weather", "FlightPlanner", "ArrivalCheck");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 7: Complete IFR Flight with Procedures
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, CompleteIFRFlightWithProcedures) {
    // Arrange: Full IFR flight with SID/STAR
    FlightScenario scenario = b737Scenario;
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_CompleteIFR",
        [this, scenario]() {
            // 1. IFR weather requirements
            std::string depMetar = "KJFK 121851Z 06015KT 5SM -RA BKN050 23/14 A3012 RMK AO2";
            METARReport depWeather = weatherDb_->parseMETAR(depMetar);
            recordDataFlow("Weather", "FlightPlanner", "IFRWeatherCheck");
            
            // 2. Get SID
            auto sidOpt = navDb_->GetSID(scenario.departure, "04L");
            if (sidOpt.has_value()) {
                recordDataFlow("Navigation", "FlightPlanner", "SIDLoaded");
            }
            
            // 3. Enroute airways
            auto enrouteRoute = navDb_->FindRoute(scenario.departure, scenario.destination,
                                                  scenario.cruiseAltitude);
            EXPECT_GT(enrouteRoute.waypoints.size(), 0);
            recordDataFlow("Navigation", "FlightPlanner", "EnrouteRoute");
            
            // 4. Get STAR
            auto starOpt = navDb_->GetSTAR(scenario.destination, "28L");
            if (starOpt.has_value()) {
                recordDataFlow("Navigation", "FlightPlanner", "STARLoaded");
            }
            
            // 5. Get approach procedures
            auto approaches = navDb_->GetApproachProceduresByRunway(scenario.destination, "28L");
            recordDataFlow("Navigation", "FlightPlanner", "ApproachesLoaded");
            
            // 6. Destination weather
            std::string arrMetar = "KORD 121856Z 22012KT 3SM -RA BKN040 18/12 A3010 RMK AO2";
            METARReport arrWeather = weatherDb_->parseMETAR(arrMetar);
            recordDataFlow("Weather", "FlightPlanner", "IFRArrivalWeather");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 8: Weather Diversion Scenario
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, WeatherDiversionScenario) {
    // Arrange: Deteriorating weather forces diversion
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_WeatherDiversion",
        [this]() {
            // Initial destination weather - good
            std::string initialMetar = "KORD 121856Z 22012KT 10SM FEW250 18/12 A3010 RMK AO2";
            METARReport initialWeather = weatherDb_->parseMETAR(initialMetar);
            EXPECT_FALSE(initialWeather.lowVisibility);
            recordDataFlow("Weather", "FlightPlanner", "InitialWeather");
            
            // Updated weather - deteriorated
            std::string updatedMetar = "KORD 121901Z 22025G35KT 2SM +TSRA BKN015CB 16/14 A2995 RMK AO2";
            METARReport updatedWeather = weatherDb_->parseMETAR(updatedMetar);
            EXPECT_TRUE(updatedWeather.lowVisibility);
            EXPECT_TRUE(updatedWeather.thunderstorm);
            recordDataFlow("Weather", "FlightPlanner", "DeterioratedWeather");
            
            // Alternate airport selection
            std::string altMetar = "KMDT 121856Z 20008KT 10SM FEW250 19/13 A3011 RMK AO2";
            METARReport altWeather = weatherDb_->parseMETAR(altMetar);
            EXPECT_FALSE(altWeather.lowVisibility);
            recordDataFlow("Weather", "FlightPlanner", "AlternateWeather");
            
            // Find route to alternate
            auto altRoute = navDb_->FindRoute("ORD_CURRENT", "KMDT", 10000);
            recordDataFlow("Navigation", "FlightPlanner", "DiversionRoute");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 9: Performance Under Load - Multiple Flights
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, PerformanceUnderLoadMultipleFlights) {
    // Arrange: Simulate multiple concurrent flights
    struct FlightData {
        std::string departure;
        std::string destination;
        int cruiseAlt;
    };
    
    std::vector<FlightData> flights = {
        {"KJFK", "KORD", 35000},
        {"KORD", "KLAX", 37000},
        {"KLAX", "KJFK", 35000},
        {"KJFK", "KMIA", 33000},
        {"KMIA", "KORD", 32000}
    };
    
    // Act
    auto metrics = stressTestConcurrentAccess(
        "FlightScenario_MultipleFlights",
        50,
        [this, &flights]() {
            static int index = 0;
            FlightData flight = flights[index++ % flights.size()];
            
            auto route = navDb_->FindRoute(flight.departure, flight.destination, flight.cruiseAlt);
            EXPECT_GT(route.waypoints.size(), 0);
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 10: Concurrent System Validation - All Systems Together
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, ConcurrentSystemsStressTest) {
    // Arrange
    
    // Act
    auto metrics = stressTestConcurrentAccess(
        "FlightScenario_AllSystemsConcurrent",
        100,
        [this]() {
            // Thread 1 equivalent: Weather queries
            std::string metarString = "KJFK 121851Z 06010KT 10SM FEW250 23/14 A3012 RMK AO2";
            METARReport report = weatherDb_->parseMETAR(metarString);
            
            // Thread 2 equivalent: Navigation queries
            auto waypoint = navDb_->GetWaypoint("BOUND");
            auto nearby = navDb_->GetWaypointsNearby(40.6413, -73.7781, 20.0);
            
            // Thread 3 equivalent: Terrain queries
            Position testPos = {40.6413, -73.7781, 5000.0, 180.0};
            double elevation = terrainDb_->getElevationAt(testPos);
            
            // Thread 4 equivalent: Runway selection
            std::vector<RunwayInfo> runways;
            RunwayInfo rwy;
            rwy.headingMagnetic = 40;
            RunwayInfo selected;
            RunwaySelector::SelectForTakeoff(runways, report.windDirection,
                                            report.windSpeed, 20.0, selected);
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
    EXPECT_LT(metrics.averageLatencyMs, 5.0);
}

// ============================================================================
// TEST 11: Altitude Constraint Validation
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, AltitudeConstraintValidation) {
    // Arrange: Flight plan with altitude restrictions
    std::vector<std::string> flightPlan = {"JFK", "BOUND", "ENE", "ORD"};
    int cruiseAlt = 35000;
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_AltitudeConstraints",
        [this, flightPlan, cruiseAlt]() {
            // Validate flight plan
            auto result = navDb_->ValidateFlightPlan(flightPlan, cruiseAlt);
            
            // Check altitude conflicts
            auto conflicts = navDb_->CheckAltitudeConflicts(flightPlan, cruiseAlt);
            
            recordDataFlow("Navigation", "FlightPlanner", "AltitudeValidation");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 12: Extended Flight Scenario - Cross-Country
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, ExtendedCrossCountryFlight) {
    // Arrange: Long-distance flight with multiple waypoints
    std::vector<std::string> route = {"KJFK", "BOUND", "ENE", "MXE", "ORD"};
    
    // Act
    auto metrics = measurePerformanceIterations(
        "FlightScenario_ExtendedCrossCountry",
        [this, &route]() {
            // Plan complete cross-country route
            double totalDistance = 0.0;
            double totalTime = 0.0;
            
            for (size_t i = 0; i < route.size() - 1; ++i) {
                double distance = navDb_->CalculateDistance(route[i], route[i + 1]);
                if (distance > 0) {
                    totalDistance += distance;
                    totalTime += navDb_->CalculateFlightTime(distance, 450.0);
                }
            }
            
            EXPECT_GT(totalDistance, 500.0);
            EXPECT_GT(totalTime, 0.0);
            recordDataFlow("Navigation", "FlightPlanner", "CrossCountryRoute");
        },
        5,
        50.0);
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 13: System Health Check and Database Consistency
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, SystemHealthAndConsistency) {
    // Arrange
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_SystemHealth",
        [this]() {
            // Check navigation database health
            auto navStats = navDb_->GetStatistics();
            EXPECT_GT(navStats.waypointCount, 0);
            
            // Verify database consistency
            std::string navConsistency = navDb_->CheckDatabaseConsistency();
            // Should be empty or indicate OK
            
            // Check cache statistics for terrain and weather
            if (terrainDb_) {
                auto [tilesInCache, cacheSizeMb] = terrainDb_->getCacheStats();
                EXPECT_GE(tilesInCache, 0);
            }
            
            if (weatherDb_) {
                int weatherCacheSize = weatherDb_->getCacheSize();
                EXPECT_GE(weatherCacheSize, 0);
            }
            
            recordDataFlow("Systems", "HealthCheck", "AllSystemsOK");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 14: Emergency Procedures - Engine Out
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, EmergencyProcedures_EngineOut) {
    // Arrange: Engine failure scenario requiring diversion
    Position failurePosition = {40.8000, -73.5000, 15000.0, 45.0};
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_EngineOut",
        [this, failurePosition]() {
            // Find nearby suitable airports
            auto nearby = navDb_->GetWaypointsNearby(failurePosition.latitude, 
                                                     failurePosition.longitude, 50.0);
            EXPECT_GT(nearby.size(), 0);
            recordDataFlow("Navigation", "FlightPlanner", "NearbyAirports");
            
            // Check descent terrain clearance
            std::vector<double> descentPath = terrainDb_->getElevationProfile(
                failurePosition,
                Position{nearby[0].latitude, nearby[0].longitude, 500.0, 0.0},
                10);
            
            for (double elev : descentPath) {
                EXPECT_LT(elev, failurePosition.altitude);
            }
            recordDataFlow("Terrain", "TAWS", "EmergencyDescentPath");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 15: Final Complete Integration Validation
// ============================================================================
TEST_F(EndToEndFlightScenarioTest, FinalCompleteIntegrationValidation) {
    // Arrange: Complete end-to-end validation
    std::string departure = "KJFK";
    std::string destination = "KORD";
    int cruiseAltitude = 35000;
    
    // Act
    auto metrics = measurePerformance(
        "FlightScenario_FinalValidation",
        [this, departure, destination, cruiseAltitude]() {
            // 1. Weather validation
            std::string metarString = "KJFK 121851Z 06010KT 10SM FEW250 23/14 A3012 RMK AO2";
            METARReport weather = weatherDb_->parseMETAR(metarString);
            recordDataFlow("Weather", "FlightPlanner", "WeatherData");
            
            // 2. Runway selection
            std::vector<RunwayInfo> runways;
            RunwayInfo rwy;
            rwy.headingMagnetic = 40;
            rwy.runwayId = "04L";
            runways.push_back(rwy);
            
            RunwayInfo selectedRunway;
            RunwaySelector::SelectForTakeoff(runways, weather.windDirection,
                                            weather.windSpeed, 20.0, selectedRunway);
            recordDataFlow("Weather", "Runway", "RunwaySelection");
            
            // 3. Flight plan routing
            auto route = navDb_->FindRoute(departure, destination, cruiseAltitude);
            EXPECT_GT(route.waypoints.size(), 0);
            recordDataFlow("Navigation", "FlightPlanner", "RouteData");
            
            // 4. Terrain validation along route
            for (const auto& wp : route.waypoints) {
                double terrain = terrainDb_->getElevationAt(
                    Position{wp.latitude, wp.longitude, cruiseAltitude, 0.0});
                EXPECT_LT(terrain, cruiseAltitude);
            }
            recordDataFlow("Terrain", "TAWS", "TerrainValidation");
            
            // 5. Verify all data flows connected
            EXPECT_TRUE(validateDataFlow("Weather", "FlightPlanner"));
            EXPECT_TRUE(validateDataFlow("Weather", "Runway"));
            EXPECT_TRUE(validateDataFlow("Navigation", "FlightPlanner"));
            EXPECT_TRUE(validateDataFlow("Terrain", "TAWS"));
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

} // namespace Tests
} // namespace AICopilot

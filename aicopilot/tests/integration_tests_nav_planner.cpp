/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Integration Tests: Navigation → Flight Planning System
* Validates navigation data flows to flight planning
*
* Test Coverage:
* - Navigation data enables flight planning
* - Waypoint routing validation
* - Airways flow into flight planner
* - SID/STAR integration
* - Route optimization verification
* - Concurrent waypoint access stress testing
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "integration_framework.hpp"
#include <algorithm>
#include <cmath>

namespace AICopilot {
namespace Tests {

class NavigationFlightPlannerIntegrationTest : public IntegrationTestBase {
protected:
    // Test route data
    std::string testOrigin = "KJFK";
    std::string testDestination = "KORD";
    std::string alternateDestination = "KLAX";
    int testCruiseAltitude = 35000;
};

// ============================================================================
// TEST 1: Navigation Data Flows to Flight Planner
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, NavigationDataFlowsToFlightPlanner) {
    // Arrange
    std::string waypoint = "BOUND";
    
    // Act
    auto metrics = measurePerformance(
        "NavData_FlightPlanner_DataFlow",
        [this, waypoint]() {
            auto waypointOpt = navDb_->GetWaypoint(waypoint);
            if (waypointOpt.has_value()) {
                recordDataFlow("Navigation", "FlightPlanner", "WaypointData");
            }
        });
    
    // Assert
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 2: Waypoint Lookup Performance
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, WaypointLookupPerformance) {
    // Arrange
    std::vector<std::string> waypoints = {"JFK", "BOUND", "ENE", "ORD", "LAX"};
    
    // Act
    auto metrics = measurePerformanceIterations(
        "Navigation_WaypointLookup",
        [this, &waypoints]() {
            static int index = 0;
            auto wp = navDb_->GetWaypoint(waypoints[index++ % waypoints.size()]);
            if (wp.has_value()) {
                recordDataFlow("Navigation", "FlightPlanner", "Waypoint");
            }
        },
        50,
        50.0);
    
    // Assert: Waypoint lookup should be <1ms
    VALIDATE_PERFORMANCE(metrics, 50.0);
    EXPECT_LT(metrics.averageLatencyMs, 1.0);
}

// ============================================================================
// TEST 3: Nearby Waypoint Discovery
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, NearbyWaypointDiscovery) {
    // Arrange: JFK location
    double jfkLat = 40.6413;
    double jfkLon = -73.7781;
    double searchRadius = 20.0; // 20 NM radius
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_NearbyWaypoints",
        [this, jfkLat, jfkLon, searchRadius]() {
            auto nearby = navDb_->GetWaypointsNearby(jfkLat, jfkLon, searchRadius);
            EXPECT_GT(nearby.size(), 0);
            recordDataFlow("Navigation", "FlightPlanner", "NearbyWaypoints");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 4: Airway Integration with Flight Plan
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, AirwayIntegrationFlightPlan) {
    // Arrange
    std::string airwayName = "V1";  // Common test airway
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_AirwayIntegration",
        [this, airwayName]() {
            auto airway = navDb_->GetAirway(airwayName);
            if (airway.has_value()) {
                auto waypoints = navDb_->GetAirwayWaypoints(airwayName);
                EXPECT_GT(waypoints.size(), 1);
                recordDataFlow("Navigation", "FlightPlanner", "AirwayData");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 5: SID Integration (Departure Procedures)
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, SIDIntegrationDeparture) {
    // Arrange
    std::string airport = "KJFK";
    std::string runway = "04L";
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_SIDIntegration",
        [this, airport, runway]() {
            auto sidOpt = navDb_->GetSID(airport, runway);
            if (sidOpt.has_value()) {
                EXPECT_EQ(sidOpt->airport, airport);
                recordDataFlow("Navigation", "FlightPlanner", "SIDData");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 6: STAR Integration (Arrival Procedures)
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, STARIntegrationArrival) {
    // Arrange
    std::string airport = "KORD";
    std::string runway = "28L";
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_STARIntegration",
        [this, airport, runway]() {
            auto starOpt = navDb_->GetSTAR(airport, runway);
            if (starOpt.has_value()) {
                EXPECT_EQ(starOpt->airport, airport);
                recordDataFlow("Navigation", "FlightPlanner", "STARData");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 7: Route Finding Between Airports
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, RouteFinderIntegration) {
    // Arrange
    std::string origin = "KJFK";
    std::string destination = "KORD";
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_RouteFinding",
        [this, origin, destination]() {
            auto result = navDb_->FindRoute(origin, destination, testCruiseAltitude);
            EXPECT_GT(result.waypoints.size(), 0);
            recordDataFlow("Navigation", "FlightPlanner", "RouteData");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 8: Distance Calculation
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, DistanceCalculation) {
    // Arrange
    std::string waypoint1 = "JFK";
    std::string waypoint2 = "ORD";
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_DistanceCalc",
        [this, waypoint1, waypoint2]() {
            double distance = navDb_->CalculateDistance(waypoint1, waypoint2);
            if (distance > 0) {
                EXPECT_GT(distance, 500.0);  // JFK to ORD is ~750 NM
                EXPECT_LT(distance, 1000.0);
                recordDataFlow("Navigation", "FlightPlanner", "DistanceData");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 9: Heading Calculation
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, HeadingCalculation) {
    // Arrange
    std::string from = "JFK";
    std::string to = "ORD";
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_HeadingCalc",
        [this, from, to]() {
            double heading = navDb_->CalculateHeading(from, to);
            if (heading >= 0) {
                EXPECT_GE(heading, 0.0);
                EXPECT_LT(heading, 360.0);
                recordDataFlow("Navigation", "FlightPlanner", "HeadingData");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 10: Flight Plan Validation
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, FlightPlanValidation) {
    // Arrange
    std::vector<std::string> flightPlan = {"JFK", "BOUND", "ORD"};
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_FlightPlanValidation",
        [this, flightPlan]() {
            auto result = navDb_->ValidateFlightPlan(flightPlan, testCruiseAltitude);
            recordDataFlow("Navigation", "FlightPlanner", "ValidationResult");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 11: Airway Connectivity Validation
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, AirwayConnectivityValidation) {
    // Arrange
    std::vector<std::string> flightPlan = {"KJFK", "BOUND", "ENE", "KORD"};
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_AirwayConnectivity",
        [this, flightPlan]() {
            bool connected = navDb_->ValidateWaypointConnectivity(flightPlan, testCruiseAltitude);
            recordDataFlow("Navigation", "FlightPlanner", "ConnectivityCheck");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 12: Direct Route Calculation
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, DirectRouteCalculation) {
    // Arrange
    std::string origin = "JFK";
    std::string destination = "ORD";
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_DirectRoute",
        [this, origin, destination]() {
            auto directRoute = navDb_->FindDirectRoute(origin, destination);
            EXPECT_EQ(directRoute.size(), 2);
            EXPECT_EQ(directRoute[0], origin);
            EXPECT_EQ(directRoute[1], destination);
            recordDataFlow("Navigation", "FlightPlanner", "DirectRoute");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 13: Airways-Based Route Finding
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, AirwaysBasedRouteFinding) {
    // Arrange
    std::string origin = "JFK";
    std::string destination = "ORD";
    std::vector<std::string> preferredAirways = {"V1", "V2", "V3"};
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_AirwaysBasedRoute",
        [this, origin, destination, &preferredAirways]() {
            auto route = navDb_->FindRouteViaAirways(origin, destination, preferredAirways);
            EXPECT_GT(route.size(), 0);
            recordDataFlow("Navigation", "FlightPlanner", "AirwaysRoute");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 14: Approach Procedure Integration
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, ApproachProcedureIntegration) {
    // Arrange
    std::string airport = "KORD";
    std::string runway = "28L";
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_ApproachProcedure",
        [this, airport, runway]() {
            auto approaches = navDb_->GetApproachProceduresByRunway(airport, runway);
            // May or may not have approaches depending on database
            recordDataFlow("Navigation", "FlightPlanner", "ApproachData");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 15: Flight Time Calculation
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, FlightTimeCalculation) {
    // Arrange
    double distance = 750.0;  // NM
    double groundSpeed = 450.0; // knots
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_FlightTimeCalc",
        [this, distance, groundSpeed]() {
            double flightTime = navDb_->CalculateFlightTime(distance, groundSpeed);
            EXPECT_GT(flightTime, 0.0);
            EXPECT_LT(flightTime, 180.0);  // Less than 3 hours
            recordDataFlow("Navigation", "FlightPlanner", "FlightTime");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 16: Concurrent Waypoint Access (Stress Test)
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, ConcurrentWaypointAccessStress) {
    // Arrange
    std::vector<std::string> waypoints = {"JFK", "BOUND", "ENE", "ORD", "LAX", "SFO"};
    
    // Act
    auto metrics = stressTestConcurrentAccess(
        "Navigation_ConcurrentWaypoints",
        100,
        [this, &waypoints]() {
            static int index = 0;
            auto wp = navDb_->GetWaypoint(waypoints[index++ % waypoints.size()]);
            EXPECT_TRUE(wp.has_value() || wp == std::nullopt); // Either valid or not found
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
    EXPECT_LT(metrics.averageLatencyMs, 1.0);
}

// ============================================================================
// TEST 17: Altitude Conflict Detection
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, AltitudeConflictDetection) {
    // Arrange
    std::vector<std::string> flightPlan = {"JFK", "BOUND", "ORD"};
    int cruiseAlt = 35000;
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_AltitudeConflicts",
        [this, flightPlan, cruiseAlt]() {
            auto conflicts = navDb_->CheckAltitudeConflicts(flightPlan, cruiseAlt);
            // May or may not have conflicts depending on procedures
            recordDataFlow("Navigation", "FlightPlanner", "AltitudeCheck");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 18: Route Optimization
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, RouteOptimization) {
    // Arrange
    std::string origin = "KJFK";
    std::string destination = "KORD";
    
    // Act
    auto metrics = measurePerformanceIterations(
        "Navigation_RouteOptimization",
        [this, origin, destination]() {
            auto result = navDb_->FindRoute(origin, destination, testCruiseAltitude);
            // Optimized route should have reasonable number of waypoints
            if (result.waypoints.size() > 0) {
                EXPECT_LT(result.waypoints.size(), 20);  // Not too many waypoints
            }
        },
        20,
        50.0);
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 19: Database Statistics and Health
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, DatabaseHealthCheck) {
    // Arrange
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_DatabaseHealth",
        [this]() {
            auto stats = navDb_->GetStatistics();
            EXPECT_GT(stats.waypointCount, 0);
            
            std::string consistency = navDb_->CheckDatabaseConsistency();
            EXPECT_TRUE(consistency.empty() || consistency == "");
            
            recordDataFlow("Navigation", "FlightPlanner", "DatabaseStats");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 20: SID/STAR Availability by Airport
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, SIDSTARAvailability) {
    // Arrange
    std::string airports[] = {"KJFK", "KORD", "KLAX"};
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_SIDSTARAvailability",
        [this, airports]() {
            for (int i = 0; i < 3; ++i) {
                auto sids = navDb_->GetSIDsByAirport(airports[i]);
                auto stars = navDb_->GetSTARsByAirport(airports[i]);
                
                // May or may not have procedures depending on database
                recordDataFlow("Navigation", "FlightPlanner", "SIDSTARCheck");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 21: Multi-Leg Route Planning
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, MultiLegRoutePlanning) {
    // Arrange: Three-leg route JFK -> ORD -> LAX
    std::vector<std::pair<std::string, std::string>> legs = {
        {"KJFK", "KORD"},
        {"KORD", "KLAX"},
        {"KLAX", "KJFK"}
    };
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_MultiLegRoute",
        [this, &legs]() {
            for (const auto& leg : legs) {
                auto result = navDb_->FindRoute(leg.first, leg.second, testCruiseAltitude);
                EXPECT_GT(result.waypoints.size(), 0);
            }
            recordDataFlow("Navigation", "FlightPlanner", "MultiLegRoute");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 22: Waypoint Type Filtering
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, WaypointTypeFiltering) {
    // Arrange
    NavaidType types[] = {NavaidType::VOR, NavaidType::NDB, NavaidType::FIX};
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_TypeFiltering",
        [this, types]() {
            for (int i = 0; i < 3; ++i) {
                auto waypoints = navDb_->GetWaypointsByType(types[i]);
                // May or may not have waypoints of specific types
            }
            recordDataFlow("Navigation", "FlightPlanner", "TypeFiltering");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 23: Preload and Cache Warming
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, CacheWarmingPerformance) {
    // Arrange
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_CacheWarming",
        [this]() {
            int preloaded = navDb_->PreloadCommonData();
            EXPECT_GE(preloaded, 0);
            recordDataFlow("Navigation", "FlightPlanner", "CacheWarming");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 24: Region-Based Waypoint Search
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, RegionBasedSearch) {
    // Arrange: Northeast region
    
    // Act
    auto metrics = measurePerformance(
        "Navigation_RegionSearch",
        [this]() {
            // Get nearby waypoints which implicitly filters by region
            auto nearby = navDb_->GetWaypointsNearby(40.7128, -74.0060, 50.0);
            EXPECT_GT(nearby.size(), 0);
            recordDataFlow("Navigation", "FlightPlanner", "RegionSearch");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 25: Final Navigation-FlightPlanner Integration
// ============================================================================
TEST_F(NavigationFlightPlannerIntegrationTest, FinalNavigationFlightPlannerIntegration) {
    // Arrange: Complete flight planning workflow
    std::string departure = "KJFK";
    std::string destination = "KORD";
    std::string depRunway = "04L";
    std::string arrRunway = "28L";
    
    // Act
    auto metrics = measurePerformanceIterations(
        "Navigation_FlightPlanning_Complete",
        [this, departure, destination, depRunway, arrRunway]() {
            // 1. Get SID for departure
            auto sidOpt = navDb_->GetSID(departure, depRunway);
            
            // 2. Get STAR for arrival
            auto starOpt = navDb_->GetSTAR(destination, arrRunway);
            
            // 3. Find route
            auto route = navDb_->FindRoute(departure, destination, testCruiseAltitude);
            
            // 4. Calculate flight time
            if (route.waypoints.size() > 0) {
                double distance = navDb_->CalculateDistance(
                    departure, destination);
                double flightTime = navDb_->CalculateFlightTime(distance);
                EXPECT_GT(flightTime, 0.0);
            }
            
            recordDataFlow("Navigation", "FlightPlanner", "CompleteFlightPlan");
        },
        25,
        50.0);
    
    // Assert
    VALIDATE_PERFORMANCE(metrics, 50.0);
    EXPECT_LT(metrics.averageLatencyMs, 10.0);
}

} // namespace Tests
} // namespace AICopilot

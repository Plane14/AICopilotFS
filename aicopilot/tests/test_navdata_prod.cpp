/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* COMPREHENSIVE NAVIGATION DATABASE TESTS
* 350+ lines covering 15+ test cases
* Tests waypoints, airways, SIDs, STARs, routing, and validation
*****************************************************************************/

#ifdef GTEST_AVAILABLE
#include <gtest/gtest.h>
#else
// Minimal test framework fallback
#include <cassert>
#include <iostream>
#include <vector>
#include <string>

#define TEST(suite, name) void test_##suite##_##name()
#define EXPECT_TRUE(x) assert(x)
#define EXPECT_FALSE(x) assert(!(x))
#define EXPECT_EQ(x, y) assert((x) == (y))
#define EXPECT_GT(x, y) assert((x) > (y))
#define EXPECT_LT(x, y) assert((x) < (y))
#define EXPECT_GE(x, y) assert((x) >= (y))
#define EXPECT_LE(x, y) assert((x) <= (y))
#define EXPECT_NEAR(x, y, tolerance) assert(std::abs((x) - (y)) <= (tolerance))
#define EXPECT_DOUBLE_EQ(x, y) assert(std::abs((x) - (y)) < 1e-10)
#define RUN_ALL_TESTS() 0

#endif

#include "../../include/navdata_database.hpp"
#include "../../include/airway_router.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>

using namespace AICopilot;

// Global test utilities
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

void PrintTestHeader(const std::string& testName) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "TEST: " << testName << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

#ifdef GTEST_AVAILABLE

class NavigationDatabaseTest : public ::testing::Test {
protected:
    NavigationDatabaseTest() : db_() {}
    NavigationDatabase db_;
};

class AirwayRouterTest : public ::testing::Test {
protected:
    AirwayRouterTest() : db_(), router_(db_) {}
    NavigationDatabase db_;
    AirwayRouter router_;
};

// ============================================================================
// WAYPOINT TESTS (4 tests)
// ============================================================================

TEST_F(NavigationDatabaseTest, GetWaypointValid) {
    auto wp = db_.GetWaypoint("KJFK");
    EXPECT_TRUE(wp.has_value());
    if (wp.has_value()) {
        EXPECT_EQ(wp->name, "KJFK");
        EXPECT_NEAR(wp->latitude, 40.6413, 0.01);
        EXPECT_NEAR(wp->longitude, -73.7781, 0.01);
        EXPECT_EQ(wp->type, NavaidType::AIRPORT);
    }
}

TEST_F(NavigationDatabaseTest, GetWaypointInvalid) {
    auto wp = db_.GetWaypoint("INVALID_WAYPOINT");
    EXPECT_FALSE(wp.has_value());
}

TEST_F(NavigationDatabaseTest, GetWaypointsByType) {
    auto vors = db_.GetWaypointsByType(NavaidType::VOR);
    EXPECT_GT(vors.size(), 0);
    for (const auto& wp : vors) {
        EXPECT_EQ(wp.type, NavaidType::VOR);
    }
}

TEST_F(NavigationDatabaseTest, GetWaypointsNearby) {
    // Get waypoints within 100 NM of NYC
    auto nearby = db_.GetWaypointsNearby(40.7128, -74.0060, 100.0);
    EXPECT_GT(nearby.size(), 0);
    
    // Verify all results are within radius
    for (const auto& wp : nearby) {
        double lat_rad = (wp.latitude - 40.7128) * M_PI / 180.0;
        double lon_rad = (wp.longitude - (-74.0060)) * M_PI / 180.0;
        double d = 2.0 * 3440.065 * std::asin(std::sqrt(
            std::sin(lat_rad/2)*std::sin(lat_rad/2) + 
            std::cos(40.7128*M_PI/180.0)*std::cos(wp.latitude*M_PI/180.0)*
            std::sin(lon_rad/2)*std::sin(lon_rad/2)));
        EXPECT_LE(d, 100.1); // Small tolerance for rounding
    }
}

// ============================================================================
// WAYPOINT COUNT AND STATS TESTS (2 tests)
// ============================================================================

TEST_F(NavigationDatabaseTest, WaypointCountGreaterThan500) {
    int count = db_.GetWaypointCount();
    EXPECT_GT(count, 500);
    std::cout << "Total waypoints in database: " << count << std::endl;
}

TEST_F(NavigationDatabaseTest, DatabaseStatistics) {
    auto stats = db_.GetStatistics();
    EXPECT_GT(stats.waypointCount, 500);
    EXPECT_GT(stats.airwayCount, 200);
    EXPECT_GT(stats.sidCount, 0);
    EXPECT_GT(stats.starCount, 0);
    EXPECT_TRUE(stats.isReady);
    
    std::cout << db_.GetStatisticsString() << std::endl;
}

// ============================================================================
// AIRWAY TESTS (3 tests)
// ============================================================================

TEST_F(NavigationDatabaseTest, GetAirwayValid) {
    auto airway = db_.GetAirway("V1");
    EXPECT_TRUE(airway.has_value());
    if (airway.has_value()) {
        EXPECT_EQ(airway->name, "V1");
        EXPECT_GT(airway->waypointSequence.size(), 0);
    }
}

TEST_F(NavigationDatabaseTest, GetAirwayCount) {
    int count = db_.GetAirwayCount();
    EXPECT_GT(count, 200);
    std::cout << "Total airways in database: " << count << std::endl;
}

TEST_F(NavigationDatabaseTest, GetAirwaysByAltitude) {
    auto airways = db_.GetAirwaysByAltitude(25000);
    EXPECT_GT(airways.size(), 0);
    for (const auto& airway : airways) {
        EXPECT_GE(25000, airway.minimumAltitude);
        EXPECT_LE(25000, airway.maximumAltitude);
    }
}

// ============================================================================
// SID/STAR TESTS (3 tests)
// ============================================================================

TEST_F(NavigationDatabaseTest, GetSIDsByAirport) {
    auto sids = db_.GetSIDsByAirport("KJFK");
    EXPECT_GT(sids.size(), 0);
    for (const auto& sid : sids) {
        EXPECT_EQ(sid.airport, "KJFK");
        EXPECT_GT(sid.waypointSequence.size(), 0);
    }
}

TEST_F(NavigationDatabaseTest, GetSTARsByAirport) {
    auto stars = db_.GetSTARsByAirport("KJFK");
    EXPECT_GT(stars.size(), 0);
    for (const auto& star : stars) {
        EXPECT_EQ(star.airport, "KJFK");
        EXPECT_GT(star.waypointSequence.size(), 0);
    }
}

TEST_F(NavigationDatabaseTest, SIDSTARCount) {
    int sidCount = db_.GetSIDCount();
    int starCount = db_.GetSTARCount();
    EXPECT_GT(sidCount, 0);
    EXPECT_GT(starCount, 0);
    std::cout << "Total SIDs: " << sidCount << ", Total STARs: " << starCount << std::endl;
}

// ============================================================================
// DISTANCE AND HEADING CALCULATION TESTS (2 tests)
// ============================================================================

TEST_F(NavigationDatabaseTest, CalculateDistanceJFKtoLAX) {
    double distance = db_.CalculateDistance("KJFK", "KLAX");
    EXPECT_GT(distance, 0);
    // JFK to LAX is approximately 2475 NM
    EXPECT_NEAR(distance, 2475.0, 50.0);
    std::cout << "Distance JFK to LAX: " << std::fixed << std::setprecision(1) 
              << distance << " NM" << std::endl;
}

TEST_F(NavigationDatabaseTest, CalculateHeading) {
    double heading = db_.CalculateHeading("KJFK", "KLAX");
    EXPECT_GT(heading, 0);
    EXPECT_LE(heading, 360);
    // Heading should be approximately 265-270 degrees
    EXPECT_NEAR(heading, 268.0, 10.0);
    std::cout << "Magnetic heading JFK to LAX: " << std::fixed << std::setprecision(1)
              << heading << " degrees" << std::endl;
}

// ============================================================================
// FLIGHT PLAN VALIDATION TESTS (2 tests)
// ============================================================================

TEST_F(NavigationDatabaseTest, ValidateFlightPlanValid) {
    std::vector<std::string> waypoints = {"KJFK", "KORD", "KLAX"};
    auto result = db_.ValidateFlightPlan(waypoints, 35000);
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.waypointCount, 3);
    EXPECT_GT(result.totalDistance, 0);
    EXPECT_EQ(result.maxAltitude, 35000);
    
    std::cout << "Flight plan validation passed:" << std::endl
              << "  Waypoints: " << result.waypointCount << std::endl
              << "  Total distance: " << result.totalDistance << " NM" << std::endl
              << "  Est. time: " << result.estimatedTimeMinutes << " minutes" << std::endl;
}

TEST_F(NavigationDatabaseTest, ValidateFlightPlanInvalid) {
    std::vector<std::string> waypoints = {"KJFK", "INVALID", "KLAX"};
    auto result = db_.ValidateFlightPlan(waypoints, 35000);
    
    EXPECT_FALSE(result.isValid);
    EXPECT_FALSE(result.errorMessage.empty());
}

// ============================================================================
// ROUTE FINDING TESTS (2 tests)
// ============================================================================

TEST_F(NavigationDatabaseTest, FindDirectRoute) {
    auto route = db_.FindDirectRoute("KJFK", "KLAX");
    EXPECT_EQ(route.size(), 2);
    EXPECT_EQ(route[0], "KJFK");
    EXPECT_EQ(route[1], "KLAX");
}

TEST_F(NavigationDatabaseTest, FindRoute) {
    auto result = db_.FindRoute("KJFK", "KLAX", 35000);
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.totalDistance, 0);
    EXPECT_GT(result.estimatedTimeMinutes, 0);
    EXPECT_GT(result.fuelRequired, 0);
    EXPECT_FALSE(result.routeDescription.empty());
    
    std::cout << "Route found: " << result.routeDescription << std::endl
              << "  Distance: " << result.totalDistance << " NM" << std::endl
              << "  Time: " << result.estimatedTimeMinutes << " min" << std::endl
              << "  Fuel: " << result.fuelRequired << " lbs" << std::endl;
}

// ============================================================================
// AIRWAY ROUTER TESTS (2 tests)
// ============================================================================

TEST_F(AirwayRouterTest, FindOptimalRoute) {
    auto route = router_.FindOptimalRoute("KJFK", "KORD", 35000);
    EXPECT_GT(route.size(), 0);
    
    double totalDist = AirwayRouter::CalculateRouteTotalDistance(route);
    double totalTime = AirwayRouter::CalculateEstimatedTime(route, 450.0);
    
    EXPECT_GT(totalDist, 0);
    EXPECT_GT(totalTime, 0);
    
    std::cout << "\nOptimal route found:" << std::endl
              << AirwayRouter::FormatRoute(route) << std::endl;
}

TEST_F(AirwayRouterTest, FindAlternateRoutes) {
    auto routes = router_.FindAlternateRoutes("KJFK", "KLAX", 3);
    EXPECT_GE(routes.size(), 1);
    
    for (size_t i = 0; i < routes.size(); ++i) {
        double distance = AirwayRouter::CalculateRouteTotalDistance(routes[i]);
        double time = AirwayRouter::CalculateEstimatedTime(routes[i], 450.0);
        std::cout << "\nAlternate Route " << (i+1) << ": " 
                  << static_cast<int>(distance) << " NM, "
                  << static_cast<int>(time) << " min" << std::endl;
    }
}

#else

// Manual test suite when gtest is not available
int main() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "NAVIGATION DATABASE SYSTEM - COMPREHENSIVE TEST SUITE" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    NavigationDatabase db;
    AirwayRouter router(db);
    
    // TEST 1: Database Initialization
    PrintTestHeader("Database Initialization");
    std::cout << "Checking if database is ready..." << std::endl;
    EXPECT_TRUE(db.IsReady());
    std::cout << "✓ Database initialized successfully" << std::endl;
    
    // TEST 2: Waypoint Count
    PrintTestHeader("Waypoint Count Verification");
    int wpCount = db.GetWaypointCount();
    std::cout << "Waypoints in database: " << wpCount << std::endl;
    EXPECT_GT(wpCount, 500);
    std::cout << "✓ Database contains 500+ waypoints" << std::endl;
    
    // TEST 3: Get Specific Waypoint
    PrintTestHeader("Waypoint Retrieval");
    auto kjfk = db.GetWaypoint("KJFK");
    EXPECT_TRUE(kjfk.has_value());
    if (kjfk) {
        std::cout << "Retrieved waypoint: " << kjfk->name 
                  << " at (" << kjfk->latitude << ", " << kjfk->longitude << ")" << std::endl;
        EXPECT_EQ(kjfk->type, NavaidType::AIRPORT);
        std::cout << "✓ Waypoint retrieval successful" << std::endl;
    }
    
    // TEST 4: Waypoints by Type
    PrintTestHeader("Waypoints by Type");
    auto vors = db.GetWaypointsByType(NavaidType::VOR);
    std::cout << "VOR waypoints found: " << vors.size() << std::endl;
    EXPECT_GT(vors.size(), 0);
    std::cout << "✓ Type-based waypoint search successful" << std::endl;
    
    // TEST 5: Nearby Waypoints
    PrintTestHeader("Nearby Waypoints Search");
    auto nearby = db.GetWaypointsNearby(40.7128, -74.0060, 100.0);
    std::cout << "Waypoints within 100 NM of NYC: " << nearby.size() << std::endl;
    EXPECT_GT(nearby.size(), 0);
    std::cout << "✓ Spatial search operational" << std::endl;
    
    // TEST 6: Airway Count
    PrintTestHeader("Airway Statistics");
    int airwayCount = db.GetAirwayCount();
    std::cout << "Airways in database: " << airwayCount << std::endl;
    EXPECT_GT(airwayCount, 200);
    std::cout << "✓ Database contains 200+ airways" << std::endl;
    
    // TEST 7: Get Airway
    PrintTestHeader("Airway Retrieval");
    auto v1 = db.GetAirway("V1");
    EXPECT_TRUE(v1.has_value());
    if (v1) {
        std::cout << "Airway V1: " << v1->waypointSequence.size() << " waypoints" << std::endl;
        std::cout << "✓ Airway retrieval successful" << std::endl;
    }
    
    // TEST 8: Airways by Altitude
    PrintTestHeader("Airways by Altitude");
    auto altAirways = db.GetAirwaysByAltitude(25000);
    std::cout << "Airways available at FL250: " << altAirways.size() << std::endl;
    EXPECT_GT(altAirways.size(), 0);
    std::cout << "✓ Altitude-based airway search successful" << std::endl;
    
    // TEST 9: SID/STAR Retrieval
    PrintTestHeader("SID/STAR Procedures");
    auto sids = db.GetSIDsByAirport("KJFK");
    auto stars = db.GetSTARsByAirport("KJFK");
    std::cout << "SIDs at KJFK: " << sids.size() << std::endl;
    std::cout << "STARs at KJFK: " << stars.size() << std::endl;
    EXPECT_GT(sids.size(), 0);
    EXPECT_GT(stars.size(), 0);
    std::cout << "✓ SID/STAR retrieval successful" << std::endl;
    
    // TEST 10: Distance Calculation
    PrintTestHeader("Distance Calculation");
    double distance = db.CalculateDistance("KJFK", "KLAX");
    std::cout << "Distance JFK to LAX: " << std::fixed << std::setprecision(1) 
              << distance << " NM" << std::endl;
    EXPECT_NEAR(distance, 2475.0, 50.0);
    std::cout << "✓ Distance calculation verified (~2475 NM expected)" << std::endl;
    
    // TEST 11: Heading Calculation
    PrintTestHeader("Heading Calculation");
    double heading = db.CalculateHeading("KJFK", "KLAX");
    std::cout << "Magnetic heading JFK to LAX: " << std::fixed << std::setprecision(1)
              << heading << "°" << std::endl;
    EXPECT_GT(heading, 0);
    EXPECT_LE(heading, 360);
    std::cout << "✓ Heading calculation verified" << std::endl;
    
    // TEST 12: Flight Plan Validation
    PrintTestHeader("Flight Plan Validation");
    std::vector<std::string> flightPlan = {"KJFK", "KORD", "KLAX"};
    auto validation = db.ValidateFlightPlan(flightPlan, 35000);
    std::cout << "Plan valid: " << (validation.isValid ? "YES" : "NO") << std::endl;
    std::cout << "Total distance: " << std::fixed << std::setprecision(0) 
              << validation.totalDistance << " NM" << std::endl;
    std::cout << "Est. time: " << validation.estimatedTimeMinutes << " minutes" << std::endl;
    EXPECT_TRUE(validation.isValid);
    EXPECT_GT(validation.totalDistance, 0);
    std::cout << "✓ Flight plan validation successful" << std::endl;
    
    // TEST 13: Route Finding
    PrintTestHeader("Route Finding");
    auto routeResult = db.FindRoute("KJFK", "KLAX", 35000);
    std::cout << "Route: " << routeResult.routeDescription << std::endl;
    EXPECT_TRUE(routeResult.success);
    std::cout << "✓ Route finding successful" << std::endl;
    
    // TEST 14: Airway Router - Optimal Route
    PrintTestHeader("Airway Router - Optimal Route");
    auto optimalRoute = router.FindOptimalRoute("KJFK", "KORD", 35000);
    double routeDist = AirwayRouter::CalculateRouteTotalDistance(optimalRoute);
    std::cout << "Optimal route distance: " << std::fixed << std::setprecision(0)
              << routeDist << " NM" << std::endl;
    if (!optimalRoute.empty()) {
        std::cout << "✓ Optimal route found" << std::endl;
    }
    
    // TEST 15: Database Statistics
    PrintTestHeader("Database Statistics");
    auto stats = db.GetStatistics();
    std::cout << "Waypoints: " << stats.waypointCount << std::endl
              << "Airways: " << stats.airwayCount << std::endl
              << "SIDs: " << stats.sidCount << std::endl
              << "STARs: " << stats.starCount << std::endl
              << "Approaches: " << stats.approachCount << std::endl
              << "Status: " << (stats.isReady ? "Ready" : "Not Ready") << std::endl;
    std::cout << "✓ Statistics retrieved successfully" << std::endl;
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "ALL TESTS COMPLETED SUCCESSFULLY" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    return 0;
}

#endif

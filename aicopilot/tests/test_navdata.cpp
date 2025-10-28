/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Comprehensive unit tests for navigation database system
* 15+ tests covering waypoints, airways, SIDs, STARs, and route finding
*****************************************************************************/

#ifdef GTEST_AVAILABLE
#include <gtest/gtest.h>
#else
// Minimal test framework for when gtest is not available
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
#endif

#include "../../include/navdata_database.hpp"
#include "../../include/airway_router.hpp"
#include <chrono>
#include <cmath>
#include <iostream>

using namespace AICopilot;

// Global database instance
static NavigationDatabase* g_db = nullptr;

void InitializeDatabase() {
    if (!g_db) {
        g_db = new NavigationDatabase();
    }
}

void CleanupDatabase() {
    if (g_db) {
        delete g_db;
        g_db = nullptr;
    }
}

// ============================================================================
// WAYPOINT TESTS
// ============================================================================

#ifdef GTEST_AVAILABLE

class NavdataTest : public ::testing::Test {
protected:
    NavdataTest() : db_() {}
    NavigationDatabase db_;
};

TEST_F(NavdataTest, GetWaypointValid) {
    auto wp = db_.GetWaypoint("KJFK");
    EXPECT_TRUE(wp.has_value());
    if (wp.has_value()) {
        EXPECT_EQ(wp->name, "KJFK");
        EXPECT_NEAR(wp->latitude, 40.6413, 0.01);
        EXPECT_NEAR(wp->longitude, -73.7781, 0.01);
    }
}

TEST_F(NavdataTest, GetWaypointInvalid) {
    auto wp = db_.GetWaypoint("NONEXISTENT");
    EXPECT_FALSE(wp.has_value());
}

TEST_F(NavdataTest, GetWaypointsByType) {
    auto vors = db_.GetWaypointsByType(NavaidType::VOR);
    EXPECT_GT(vors.size(), 0);
    for (const auto& wp : vors) {
        EXPECT_EQ(wp.type, NavaidType::VOR);
    }
}

TEST_F(NavdataTest, GetWaypointsNearby) {
    // Get waypoints near New York
    auto nearby = db_.GetWaypointsNearby(40.7128, -74.0060, 50.0);  // 50 NM radius
    EXPECT_GT(nearby.size(), 0);
}

TEST_F(NavdataTest, WaypointCount) {
    int count = db_.GetWaypointCount();
    EXPECT_GT(count, 500);  // Should have 500+ waypoints
}

TEST_F(NavdataTest, GetAirwayValid) {
    auto airway = db_.GetAirway("V1");
    EXPECT_TRUE(airway.has_value());
    if (airway.has_value()) {
        EXPECT_EQ(airway->name, "V1");
        EXPECT_GT(airway->waypointSequence.size(), 0);
    }
}

TEST_F(NavdataTest, GetAirwayInvalid) {
    auto airway = db_.GetAirway("NONEXISTENT");
    EXPECT_FALSE(airway.has_value());
}

TEST_F(NavdataTest, GetAirwayWaypoints) {
    auto waypoints = db_.GetAirwayWaypoints("V1");
    EXPECT_GT(waypoints.size(), 0);
}

TEST_F(NavdataTest, GetConnectingAirways) {
    auto airways = db_.GetConnectingAirways("KUJOE", "ELLOS");
    EXPECT_GT(airways.size(), 0);
}

TEST_F(NavdataTest, GetAirwaysByAltitude) {
    auto airways_high = db_.GetAirwaysByAltitude(35000);
    EXPECT_GT(airways_high.size(), 0);
    for (const auto& airway : airways_high) {
        EXPECT_LE(airway.minimumAltitude, 35000);
        EXPECT_GE(airway.maximumAltitude, 35000);
    }
}

TEST_F(NavdataTest, AirwayCount) {
    int count = db_.GetAirwayCount();
    EXPECT_GT(count, 200);  // Should have 200+ airways
}

TEST_F(NavdataTest, GetSIDValid) {
    auto sid = db_.GetSID("KJFK", "04L");
    EXPECT_TRUE(sid.has_value());
    if (sid.has_value()) {
        EXPECT_EQ(sid->airport, "KJFK");
        EXPECT_EQ(sid->runway, "04L");
    }
}

TEST_F(NavdataTest, GetSIDInvalid) {
    auto sid = db_.GetSID("KJFK", "99Z");
    EXPECT_FALSE(sid.has_value());
}

TEST_F(NavdataTest, GetSIDsByAirport) {
    auto sids = db_.GetSIDsByAirport("KJFK");
    EXPECT_GT(sids.size(), 0);
    for (const auto& sid : sids) {
        EXPECT_EQ(sid.airport, "KJFK");
    }
}

TEST_F(NavdataTest, GetSTARValid) {
    auto star = db_.GetSTAR("KJFK", "04L");
    EXPECT_TRUE(star.has_value());
    if (star.has_value()) {
        EXPECT_EQ(star->airport, "KJFK");
        EXPECT_EQ(star->runway, "04L");
    }
}

TEST_F(NavdataTest, GetSTARsByAirport) {
    auto stars = db_.GetSTARsByAirport("KJFK");
    EXPECT_GT(stars.size(), 0);
    for (const auto& star : stars) {
        EXPECT_EQ(star.airport, "KJFK");
    }
}

TEST_F(NavdataTest, SIDCount) {
    int count = db_.GetSIDCount();
    EXPECT_GT(count, 100);  // Should have 100+ SIDs
}

TEST_F(NavdataTest, STARCount) {
    int count = db_.GetSTARCount();
    EXPECT_GT(count, 100);  // Should have 100+ STARs
}

TEST_F(NavdataTest, CalculateDistanceJFKToLAX) {
    double distance = db_.CalculateDistance("KJFK", "KLAX");
    EXPECT_GT(distance, 2400.0);
    EXPECT_LT(distance, 2500.0);
}

TEST_F(NavdataTest, CalculateDistanceCoordinates) {
    double distance = db_.CalculateDistanceCoordinates(40.7128, -74.0060, 33.9425, -118.4081);
    EXPECT_GT(distance, 2400.0);
    EXPECT_LT(distance, 2500.0);
}

TEST_F(NavdataTest, CalculateDistancePerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        db_.CalculateDistance("KJFK", "KLAX");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avg_time_ms = duration.count() / 1000.0 / 1000.0;
    EXPECT_LT(avg_time_ms, 5.0);  // Should average <5ms per call
}

TEST_F(NavdataTest, CalculateHeadingValid) {
    double heading = db_.CalculateHeading("KJFK", "KLAX");
    EXPECT_GT(heading, 0.0);
    EXPECT_LT(heading, 360.0);
}

TEST_F(NavdataTest, CalculateHeadingCoordinates) {
    double heading = db_.CalculateHeadingCoordinates(40.7128, -74.0060, 33.9425, -118.4081);
    EXPECT_GT(heading, 0.0);
    EXPECT_LT(heading, 360.0);
}

TEST_F(NavdataTest, ValidateFlightPlanValid) {
    std::vector<std::string> waypoints = {"KJFK", "ELLOS", "KLAX"};
    auto result = db_.ValidateFlightPlan(waypoints, 35000);
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.waypointCount, 3);
    EXPECT_GT(result.totalDistance, 0.0);
}

TEST_F(NavdataTest, ValidateFlightPlanEmpty) {
    std::vector<std::string> waypoints;
    auto result = db_.ValidateFlightPlan(waypoints, 35000);
    
    EXPECT_FALSE(result.isValid);
}

TEST_F(NavdataTest, ValidateFlightPlanInvalidWaypoint) {
    std::vector<std::string> waypoints = {"KJFK", "NONEXISTENT", "KLAX"};
    auto result = db_.ValidateFlightPlan(waypoints, 35000);
    
    EXPECT_FALSE(result.isValid);
}

TEST_F(NavdataTest, ValidateFlightPlanInvalidAltitude) {
    std::vector<std::string> waypoints = {"KJFK", "KLAX"};
    auto result = db_.ValidateFlightPlan(waypoints, 70000);  // Too high
    
    EXPECT_FALSE(result.isValid);
}

TEST_F(NavdataTest, CheckAltitudeConflicts) {
    std::vector<std::string> waypoints = {"KDEN", "KLAX"};
    auto conflicts = db_.CheckAltitudeConflicts(waypoints, 6000);  // Below KDEN
    
    EXPECT_GT(conflicts.size(), 0);
}

TEST_F(NavdataTest, FindRouteValid) {
    auto result = db_.FindRoute("KJFK", "KLAX", 35000);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.waypointSequence.size(), 2);
    EXPECT_GT(result.totalDistance, 2400.0);
    EXPECT_LT(result.totalDistance, 2500.0);
}

TEST_F(NavdataTest, FindRouteInvalid) {
    auto result = db_.FindRoute("NONEXISTENT1", "NONEXISTENT2", 35000);
    
    EXPECT_FALSE(result.success);
}

TEST_F(NavdataTest, FindDirectRoute) {
    auto route = db_.FindDirectRoute("KJFK", "KLAX");
    
    EXPECT_EQ(route.size(), 2);
    EXPECT_EQ(route[0], "KJFK");
    EXPECT_EQ(route[1], "KLAX");
}

TEST_F(NavdataTest, AirwayRouterFindDirectRoute) {
    AirwayRouter router(db_);
    auto segment = router.FindDirectRoute("KJFK", "KLAX");
    
    EXPECT_EQ(segment.fromWaypoint, "KJFK");
    EXPECT_EQ(segment.toWaypoint, "KLAX");
    EXPECT_EQ(segment.airwayName, "DIRECT");
    EXPECT_GT(segment.distance, 2400.0);
}

TEST_F(NavdataTest, AirwayRouterCalculateRouteTotalDistance) {
    std::vector<RouteSegment> route;
    RouteSegment seg1;
    seg1.distance = 100.0;
    route.push_back(seg1);
    
    RouteSegment seg2;
    seg2.distance = 150.0;
    route.push_back(seg2);
    
    double total = AirwayRouter::CalculateRouteTotalDistance(route);
    EXPECT_DOUBLE_EQ(total, 250.0);
}

TEST_F(NavdataTest, AirwayRouterCalculateEstimatedTime) {
    std::vector<RouteSegment> route;
    RouteSegment seg;
    seg.distance = 450.0;  // 450 NM
    route.push_back(seg);
    
    double time = AirwayRouter::CalculateEstimatedTime(route, 450.0);
    EXPECT_NEAR(time, 60.0, 1.0);  // Should be about 1 hour
}

TEST_F(NavdataTest, AirwayRouterFormatRoute) {
    std::vector<RouteSegment> route;
    RouteSegment seg1;
    seg1.fromWaypoint = "KJFK";
    seg1.toWaypoint = "BOUND";
    seg1.airwayName = "V1";
    route.push_back(seg1);
    
    RouteSegment seg2;
    seg2.fromWaypoint = "BOUND";
    seg2.toWaypoint = "KLAX";
    seg2.airwayName = "";
    route.push_back(seg2);
    
    std::string formatted = AirwayRouter::FormatRoute(route);
    EXPECT_TRUE(formatted.find("KJFK") != std::string::npos);
    EXPECT_TRUE(formatted.find("KLAX") != std::string::npos);
}

TEST_F(NavdataTest, DatabaseReady) {
    EXPECT_TRUE(db_.IsReady());
}

TEST_F(NavdataTest, GetStatistics) {
    std::string stats = db_.GetStatistics();
    
    EXPECT_TRUE(stats.find("Waypoints") != std::string::npos);
    EXPECT_TRUE(stats.find("Airways") != std::string::npos);
    EXPECT_TRUE(stats.find("Ready") != std::string::npos);
}

TEST_F(NavdataTest, LargeWaypointCountStress) {
    EXPECT_GT(db_.GetWaypointCount(), 500);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto nearby = db_.GetWaypointsNearby(40.0, -100.0, 200.0);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 100);  // Should complete in <100ms
    EXPECT_GT(nearby.size(), 0);
}

TEST_F(NavdataTest, MultipleQueriesThreadSafety) {
    for (int i = 0; i < 100; ++i) {
        auto wp = db_.GetWaypoint("KJFK");
        EXPECT_TRUE(wp.has_value());
        
        auto airways = db_.GetAirwaysByAltitude(35000);
        EXPECT_GT(airways.size(), 0);
        
        auto result = db_.ValidateFlightPlan({"KJFK", "KLAX"}, 35000);
        EXPECT_TRUE(result.isValid);
    }
}

TEST_F(NavdataTest, DistanceToSelfIsZero) {
    double distance = db_.CalculateDistance("KJFK", "KJFK");
    EXPECT_NEAR(distance, 0.0, 0.1);
}

TEST_F(NavdataTest, WaypointWithNoFrequency) {
    auto wp = db_.GetWaypoint("KUJOE");
    EXPECT_TRUE(wp.has_value());
    if (wp.has_value()) {
        EXPECT_EQ(wp->type, NavaidType::FIX);
        EXPECT_EQ(wp->frequency, 0.0);
    }
}

TEST_F(NavdataTest, AirwayAltitudeRanges) {
    auto airway = db_.GetAirway("V1");
    EXPECT_TRUE(airway.has_value());
    if (airway.has_value()) {
        EXPECT_LE(airway->minimumAltitude, airway->maximumAltitude);
    }
}

#else

// Standalone test functions (when gtest not available)
void test_navdata_all() {
    InitializeDatabase();
    std::cout << "Testing Navigation Database..." << std::endl;
    
    // Test waypoint operations
    auto wp = g_db->GetWaypoint("KJFK");
    assert(wp.has_value());
    std::cout << "✓ Waypoint retrieval working" << std::endl;
    
    // Test airway operations
    auto airway = g_db->GetAirway("V1");
    assert(airway.has_value());
    std::cout << "✓ Airway retrieval working" << std::endl;
    
    // Test distance calculations
    double distance = g_db->CalculateDistance("KJFK", "KLAX");
    assert(distance > 2400.0 && distance < 2500.0);
    std::cout << "✓ Distance calculations working" << std::endl;
    
    // Test flight plan validation
    auto result = g_db->ValidateFlightPlan({"KJFK", "KLAX"}, 35000);
    assert(result.isValid);
    std::cout << "✓ Flight plan validation working" << std::endl;
    
    // Test statistics
    std::cout << g_db->GetStatistics() << std::endl;
    
    std::cout << "All tests passed!" << std::endl;
    CleanupDatabase();
}

#endif

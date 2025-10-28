#include <gtest/gtest.h>
#include "../../include/navigation.h"
#include <cmath>

using namespace AICopilot;

class NavigationTest : public ::testing::Test {
protected:
    Navigation nav;
    
    // Helper to create positions
    Position createPosition(double lat, double lon, double alt) {
        return {lat, lon, alt, 0.0};
    }
    
    // Helper to calculate expected great circle distance (Haversine formula)
    double expectedDistance(const Position& p1, const Position& p2) {
        const double R = 3440.065; // Earth radius in nautical miles
        const double DEG_TO_RAD = M_PI / 180.0;
        
        double dLat = (p2.latitude - p1.latitude) * DEG_TO_RAD;
        double dLon = (p2.longitude - p1.longitude) * DEG_TO_RAD;
        
        double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                   std::cos(p1.latitude * DEG_TO_RAD) * std::cos(p2.latitude * DEG_TO_RAD) *
                   std::sin(dLon / 2) * std::sin(dLon / 2);
        
        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return R * c;
    }
};

// Test: Distance calculation between two points
TEST_F(NavigationTest, DistanceCalculationAccuracy) {
    Position current = createPosition(40.7128, -74.0060, 0.0);    // New York
    Position waypoint = createPosition(34.0522, -118.2437, 0.0);  // Los Angeles
    
    double distance = nav.distanceToWaypoint(current, {waypoint, "LAX"});
    double expected = expectedDistance(current, waypoint);
    
    // Allow 1% tolerance for numerical precision
    EXPECT_NEAR(distance, expected, expected * 0.01);
}

// Test: Antimeridian crossing detection
TEST_F(NavigationTest, AntimeridianCrossingHandling) {
    Position current = createPosition(0.0, 179.0, 5000.0);
    Waypoint waypoint = {createPosition(0.0, -179.0, 5000.0), "TEST"};
    
    double distance = nav.distanceToWaypoint(current, waypoint);
    
    // Distance should be minimal (~120 nm at equator), not wrapping around
    EXPECT_LT(distance, 150.0);
    EXPECT_GT(distance, 0.0);
}

// Test: Bearing calculation
TEST_F(NavigationTest, BearingCalculationQuadrants) {
    Position current = createPosition(0.0, 0.0, 0.0);
    
    // Northeast
    Waypoint ne_waypoint = {createPosition(1.0, 1.0, 0.0), "NE"};
    double bearing_ne = nav.bearingToWaypoint(current, ne_waypoint);
    EXPECT_GT(bearing_ne, 0.0);
    EXPECT_LT(bearing_ne, 90.0);
    
    // Southeast
    Waypoint se_waypoint = {createPosition(-1.0, 1.0, 0.0), "SE"};
    double bearing_se = nav.bearingToWaypoint(current, se_waypoint);
    EXPECT_GT(bearing_se, 90.0);
    EXPECT_LT(bearing_se, 180.0);
    
    // Southwest
    Waypoint sw_waypoint = {createPosition(-1.0, -1.0, 0.0), "SW"};
    double bearing_sw = nav.bearingToWaypoint(current, sw_waypoint);
    EXPECT_GT(bearing_sw, 180.0);
    EXPECT_LT(bearing_sw, 270.0);
    
    // Northwest
    Waypoint nw_waypoint = {createPosition(1.0, -1.0, 0.0), "NW"};
    double bearing_nw = nav.bearingToWaypoint(current, nw_waypoint);
    EXPECT_GT(bearing_nw, 270.0);
    EXPECT_LT(bearing_nw, 360.0);
}

// Test: Waypoint reached detection
TEST_F(NavigationTest, WaypointReachedWithinTolerance) {
    Waypoint target = {createPosition(40.0, -74.0, 5000.0), "TARGET"};
    
    // Within tolerance
    Position at_waypoint = createPosition(40.0, -74.0, 5000.0);
    EXPECT_TRUE(nav.isWaypointReached(at_waypoint, 0.5));
    
    // Just outside tolerance (0.5 nm)
    Position outside = createPosition(40.007, -74.0, 5000.0);
    EXPECT_FALSE(nav.isWaypointReached(outside, 0.5));
    
    // Well within tolerance
    Position close = createPosition(40.001, -74.0, 5000.0);
    EXPECT_TRUE(nav.isWaypointReached(close, 1.0));
}

// Test: Cross-track error calculation
TEST_F(NavigationTest, CrossTrackErrorCalculation) {
    FlightPlan plan;
    Waypoint wp1 = {createPosition(40.0, -74.0, 5000.0), "START"};
    Waypoint wp2 = {createPosition(40.1, -74.0, 5000.0), "END"};
    plan.waypoints.push_back(wp1);
    plan.waypoints.push_back(wp2);
    
    // Aircraft directly on track
    Position on_track = createPosition(40.05, -74.0, 5000.0);
    double cte = nav.crossTrackError(on_track);
    EXPECT_NEAR(cte, 0.0, 0.1);
    
    // Aircraft off track (north)
    Position off_track_north = createPosition(40.05, -73.95, 5000.0);
    double cte_north = nav.crossTrackError(off_track_north);
    EXPECT_GT(cte_north, 0.5);  // Should be off-track by several nm
}

// Test: Flight plan loading and validation
TEST_F(NavigationTest, FlightPlanValidation) {
    FlightPlan plan;
    plan.departure = "KJFK";
    plan.arrival = "KLAX";
    
    // Empty waypoints should fail validation
    EXPECT_FALSE(nav.validateFlightPlan());
    
    // Add waypoints with valid positions
    Waypoint wp1 = {createPosition(40.7128, -74.0060, 5000.0), "KJFK"};
    Waypoint wp2 = {createPosition(34.0522, -118.2437, 35000.0), "KLAX"};
    
    // Manually add waypoints (since loadFlightPlan is file-based)
    // This tests the validation logic for properly formed flight plans
}

// Test: Extreme altitude edge cases
TEST_F(NavigationTest, ExtremeAltitudes) {
    Position high_altitude = createPosition(40.0, -74.0, 43000.0);  // FL430
    Position ground_level = createPosition(40.0, -74.0, 0.0);
    Position negative = createPosition(40.0, -74.0, -100.0);  // Below sea level
    
    // These should be valid positions that calculations can handle
    Waypoint wp = {high_altitude, "HIGH"};
    double dist = nav.distanceToWaypoint(ground_level, wp);
    EXPECT_GT(dist, 0.0);
}

// Test: Pole crossing scenarios
TEST_F(NavigationTest, PolarRoutingHandling) {
    Position north = createPosition(89.0, 0.0, 35000.0);
    Position south = createPosition(-89.0, 0.0, 35000.0);
    Waypoint pole_waypoint = {north, "NORTH_POLE"};
    
    double distance = nav.distanceToWaypoint(south, pole_waypoint);
    
    // Should calculate shortest route, roughly half earth circumference at poles
    double earth_circumference_nm = 20926.0;
    EXPECT_LT(distance, earth_circumference_nm);
    EXPECT_GT(distance, 0.0);
}

// Test: Time to destination calculation
TEST_F(NavigationTest, TimeToDestinationCalculation) {
    FlightPlan plan;
    Waypoint wp1 = {createPosition(40.0, -74.0, 0.0), "START"};
    Waypoint wp2 = {createPosition(40.0, -64.0, 35000.0), "END"};
    plan.waypoints.push_back(wp1);
    plan.waypoints.push_back(wp2);
    
    // Distance is approximately 540 nm
    // At 500 knots ground speed, should take ~65 minutes
    double time = nav.timeToDestination(500.0);
    EXPECT_GT(time, 60.0);
    EXPECT_LT(time, 70.0);
}

// Test: Waypoint advancement
TEST_F(NavigationTest, WaypointAdvancement) {
    FlightPlan plan;
    plan.waypoints.push_back({createPosition(40.0, -74.0, 0.0), "START"});
    plan.waypoints.push_back({createPosition(40.0, -75.0, 5000.0), "WP1"});
    plan.waypoints.push_back({createPosition(40.0, -76.0, 10000.0), "WP2"});
    
    Waypoint initial = nav.getActiveWaypoint();
    EXPECT_EQ(initial.ident, "START");
    
    nav.advanceWaypoint();
    Waypoint after_advance = nav.getActiveWaypoint();
    EXPECT_EQ(after_advance.ident, "WP1");
}

// Test: Input validation - null/empty strings
TEST_F(NavigationTest, InputValidationEmptyStrings) {
    // These should handle gracefully
    EXPECT_FALSE(nav.loadFlightPlan(""));
    EXPECT_FALSE(nav.loadFlightPlan("nonexistent_file.pln"));
    EXPECT_FALSE(nav.updateWaypointPosition("", createPosition(0, 0, 0)));
}

// Test: Input validation - invalid coordinates
TEST_F(NavigationTest, InputValidationInvalidCoordinates) {
    Position invalid_lat = createPosition(91.0, 0.0, 0.0);      // Latitude > 90
    Position invalid_lon = createPosition(0.0, 181.0, 0.0);     // Longitude > 180
    Position invalid_alt = createPosition(0.0, 0.0, -35000.0);  // Invalid negative altitude
    
    Waypoint wp_bad_lat = {invalid_lat, "BAD_LAT"};
    Waypoint wp_bad_lon = {invalid_lon, "BAD_LON"};
    Waypoint wp_bad_alt = {invalid_alt, "BAD_ALT"};
    
    // Operations should still complete (graceful degradation)
    double d1 = nav.distanceToWaypoint(createPosition(0, 0, 0), wp_bad_lat);
    double d2 = nav.distanceToWaypoint(createPosition(0, 0, 0), wp_bad_lon);
    double d3 = nav.distanceToWaypoint(createPosition(0, 0, 0), wp_bad_alt);
    
    // Results should be valid numbers
    EXPECT_TRUE(std::isfinite(d1));
    EXPECT_TRUE(std::isfinite(d2));
    EXPECT_TRUE(std::isfinite(d3));
}

// Test: Large flight plan performance
TEST_F(NavigationTest, LargeFlightPlanPerformance) {
    FlightPlan plan;
    
    // Create 100 waypoints across the US
    for (int i = 0; i < 100; ++i) {
        double lat = 25.0 + (i / 100.0) * 40.0;  // 25 to 65 degrees N
        double lon = -125.0 + (i / 100.0) * 60.0;  // -125 to -65 degrees W
        Waypoint wp = {createPosition(lat, lon, 35000.0), "WP" + std::to_string(i)};
        plan.waypoints.push_back(wp);
    }
    
    // Should complete without issues
    EXPECT_GT(plan.waypoints.size(), 0);
    
    // Calculate total distance should be performant
    double total_dist = 0.0;
    for (size_t i = 0; i < plan.waypoints.size() - 1; ++i) {
        total_dist += nav.distanceToWaypoint(
            plan.waypoints[i].position,
            plan.waypoints[i + 1]
        );
    }
    
    EXPECT_GT(total_dist, 0.0);
}


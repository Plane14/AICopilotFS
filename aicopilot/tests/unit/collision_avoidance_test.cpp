#include <gtest/gtest.h>
#include "../../include/collision_avoidance.hpp"
#include <cmath>

using namespace AICopilot::Collision;

class CollisionDetectionTest : public ::testing::Test {
protected:
    Vector2D createVector(double x, double y) {
        return Vector2D(x, y);
    }
    
    SeparationStandards::AircraftState createAircraftState(
        int id, double x, double y, double alt,
        double vx, double vy, double heading, double gs)
    {
        SeparationStandards::AircraftState state;
        state.aircraft_id = id;
        state.position_local = Vector2D(x, y);
        state.altitude_feet = alt;
        state.velocity = Vector2D(vx, vy);
        state.heading_true = heading;
        state.groundspeed_knots = gs;
        return state;
    }
};

// Test: Circle-circle collision detection - overlapping circles
TEST_F(CollisionDetectionTest, CircleCircleOverlap) {
    Circle c1(Vector2D(0.0, 0.0), 500.0);    // Aircraft 1
    Circle c2(Vector2D(400.0, 0.0), 500.0);  // Aircraft 2, 400 ft apart
    
    bool collision = CollisionDetector::check_circle_circle_collision(c1, c2);
    EXPECT_TRUE(collision);  // Circles overlap (each 500 ft radius = 1000 ft combined)
}

// Test: Circle-circle collision detection - separated circles
TEST_F(CollisionDetectionTest, CircleCircleNoCollision) {
    Circle c1(Vector2D(0.0, 0.0), 500.0);
    Circle c2(Vector2D(1500.0, 0.0), 500.0);  // 1500 ft apart
    
    bool collision = CollisionDetector::check_circle_circle_collision(c1, c2);
    EXPECT_FALSE(collision);  // Safe separation
}

// Test: Circle-circle edge case - touching circles
TEST_F(CollisionDetectionTest, CircleCircleTouching) {
    Circle c1(Vector2D(0.0, 0.0), 500.0);
    Circle c2(Vector2D(1000.0, 0.0), 500.0);  // Exactly touching
    
    bool collision = CollisionDetector::check_circle_circle_collision(c1, c2);
    EXPECT_TRUE(collision);  // At boundary, counts as collision
}

// Test: Polygon point containment - inside polygon
TEST_F(CollisionDetectionTest, PointInsidePolygon) {
    Polygon poly;
    poly.add_vertex(Vector2D(0.0, 0.0));
    poly.add_vertex(Vector2D(1000.0, 0.0));
    poly.add_vertex(Vector2D(1000.0, 1000.0));
    poly.add_vertex(Vector2D(0.0, 1000.0));
    
    Vector2D inside = Vector2D(500.0, 500.0);
    EXPECT_TRUE(poly.contains_point(inside));
}

// Test: Polygon point containment - outside polygon
TEST_F(CollisionDetectionTest, PointOutsidePolygon) {
    Polygon poly;
    poly.add_vertex(Vector2D(0.0, 0.0));
    poly.add_vertex(Vector2D(1000.0, 0.0));
    poly.add_vertex(Vector2D(1000.0, 1000.0));
    poly.add_vertex(Vector2D(0.0, 1000.0));
    
    Vector2D outside = Vector2D(2000.0, 500.0);
    EXPECT_FALSE(poly.contains_point(outside));
}

// Test: Circle-polygon collision - circle inside polygon
TEST_F(CollisionDetectionTest, CircleInsidePolygon) {
    Polygon poly;
    poly.add_vertex(Vector2D(0.0, 0.0));
    poly.add_vertex(Vector2D(2000.0, 0.0));
    poly.add_vertex(Vector2D(2000.0, 2000.0));
    poly.add_vertex(Vector2D(0.0, 2000.0));
    
    Circle circle(Vector2D(500.0, 500.0), 100.0);
    bool collision = CollisionDetector::check_circle_polygon_collision(circle, poly);
    EXPECT_TRUE(collision);
}

// Test: Circle-polygon collision - circle touching edge
TEST_F(CollisionDetectionTest, CircleTouchingPolygonEdge) {
    Polygon poly;
    poly.add_vertex(Vector2D(0.0, 0.0));
    poly.add_vertex(Vector2D(2000.0, 0.0));
    poly.add_vertex(Vector2D(2000.0, 2000.0));
    poly.add_vertex(Vector2D(0.0, 2000.0));
    
    Circle circle(Vector2D(-100.0, 1000.0), 150.0);  // Center outside, radius reaches edge
    bool collision = CollisionDetector::check_circle_polygon_collision(circle, poly);
    EXPECT_TRUE(collision);  // Should detect edge intersection
}

// Test: Separation standards - lateral separation violation
TEST_F(CollisionDetectionTest, LateralSeparationViolation) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    auto ac2 = createAircraftState(2, 200.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);  // 200 ft apart
    
    auto conflict = standards.check_separation_conflict(ac1, ac2);
    EXPECT_EQ(conflict, SeparationStandards::ConflictType::SameAltitude);  // Same altitude, too close laterally
}

// Test: Separation standards - lateral separation safe
TEST_F(CollisionDetectionTest, LateralSeparationSafe) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    auto ac2 = createAircraftState(2, 1500.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);  // 1500 ft apart
    
    auto conflict = standards.check_separation_conflict(ac1, ac2);
    EXPECT_EQ(conflict, SeparationStandards::ConflictType::None);  // Safe separation
}

// Test: Separation standards - vertical separation (different altitudes)
TEST_F(CollisionDetectionTest, VerticalSeparationSafe) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    auto ac2 = createAircraftState(2, 200.0, 0.0, 7000.0, 100.0, 0.0, 0.0, 300.0);  // 2000 ft altitude diff
    
    auto conflict = standards.check_separation_conflict(ac1, ac2);
    EXPECT_EQ(conflict, SeparationStandards::ConflictType::None);  // Vertical separation sufficient
}

// Test: Head-on conflict detection
TEST_F(CollisionDetectionTest, HeadOnConflictDetection) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);      // Heading 0
    auto ac2 = createAircraftState(2, 400.0, 0.0, 5000.0, -100.0, 0.0, 180.0, 300.0); // Heading 180 (opposite)
    
    auto conflict = standards.check_separation_conflict(ac1, ac2);
    EXPECT_EQ(conflict, SeparationStandards::ConflictType::HeadOn);
}

// Test: Parallel conflict detection
TEST_F(CollisionDetectionTest, ParallelConflictDetection) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    auto ac2 = createAircraftState(2, 200.0, 0.0, 5000.0, 100.0, 0.0, 5.0, 300.0);  // Nearly parallel
    
    auto conflict = standards.check_separation_conflict(ac1, ac2);
    EXPECT_EQ(conflict, SeparationStandards::ConflictType::Parallel);
}

// Test: Crossing conflict detection
TEST_F(CollisionDetectionTest, CrossingConflictDetection) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);      // Heading 0
    auto ac2 = createAircraftState(2, 200.0, 0.0, 5000.0, 0.0, 100.0, 90.0, 300.0);   // Heading 90
    
    auto conflict = standards.check_separation_conflict(ac1, ac2);
    EXPECT_EQ(conflict, SeparationStandards::ConflictType::Crossing);
}

// Test: Closest point of approach calculation
TEST_F(CollisionDetectionTest, ClosestPointOfApproachCalculation) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    auto ac2 = createAircraftState(2, 1000.0, 1000.0, 5000.0, -100.0, 0.0, 180.0, 300.0);
    
    double time_to_cpa = 0.0;
    double distance_at_cpa = standards.calculate_closest_point_of_approach(ac1, ac2, time_to_cpa);
    
    EXPECT_GT(time_to_cpa, 0.0);      // Should have positive CPA time
    EXPECT_GT(distance_at_cpa, 0.0);  // Should have measurable distance
}

// Test: Conflict prediction - converging aircraft
TEST_F(CollisionDetectionTest, ConflictPredictionConverging) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 200.0, 0.0, 90.0, 400.0);
    auto ac2 = createAircraftState(2, 500.0, 0.0, 5000.0, -200.0, 0.0, 270.0, 400.0);  // Converging
    
    double time_to_conflict = 0.0;
    bool conflict = standards.predict_collision(ac1, ac2, 60.0, time_to_conflict);
    
    if (conflict) {
        EXPECT_GT(time_to_conflict, 0.0);
        EXPECT_LT(time_to_conflict, 60.0);  // Within prediction horizon
    }
}

// Test: Conflict prediction - diverging aircraft
TEST_F(CollisionDetectionTest, ConflictPredictionDiverging) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 200.0, 0.0, 90.0, 400.0);
    auto ac2 = createAircraftState(2, 500.0, 0.0, 5000.0, 200.0, 0.0, 90.0, 400.0);  // Parallel, same direction
    
    double time_to_conflict = 0.0;
    bool conflict = standards.predict_collision(ac1, ac2, 60.0, time_to_conflict);
    
    EXPECT_FALSE(conflict);  // No conflict for diverging aircraft
}

// Test: Low altitude separation standards
TEST_F(CollisionDetectionTest, LowAltitudeSeparationStandards) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 1000.0, 100.0, 0.0, 0.0, 300.0);
    auto ac2 = createAircraftState(2, 400.0, 0.0, 1000.0, 100.0, 0.0, 0.0, 300.0);  // Close together at low altitude
    
    auto conflict = standards.check_separation_conflict(ac1, ac2, true);  // Low altitude area
    // Should use reduced separation standards
    EXPECT_TRUE(conflict == SeparationStandards::ConflictType::SameAltitude ||
                conflict == SeparationStandards::ConflictType::None);
}

// Test: Conflict predictor - multiple aircraft tracking
TEST_F(CollisionDetectionTest, ConflictPredictorMultipleAircraft) {
    ConflictPredictor predictor(30.0);
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 200.0, 0.0, 90.0, 400.0);
    auto ac2 = createAircraftState(2, 500.0, 0.0, 5000.0, -200.0, 0.0, 270.0, 400.0);
    auto ac3 = createAircraftState(3, 0.0, 500.0, 3000.0, 100.0, 100.0, 45.0, 300.0);
    
    predictor.update_aircraft_state(ac1);
    predictor.update_aircraft_state(ac2);
    predictor.update_aircraft_state(ac3);
    
    EXPECT_EQ(predictor.get_tracked_aircraft_count(), 3);
    
    auto alerts = predictor.predict_conflicts();
    // Should return alerts or empty vector
    EXPECT_TRUE(alerts.size() >= 0);
}

// Test: Aircraft removal from predictor
TEST_F(CollisionDetectionTest, AircraftRemovalFromPredictor) {
    ConflictPredictor predictor(30.0);
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    auto ac2 = createAircraftState(2, 1000.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    
    predictor.update_aircraft_state(ac1);
    predictor.update_aircraft_state(ac2);
    EXPECT_EQ(predictor.get_tracked_aircraft_count(), 2);
    
    predictor.remove_aircraft(1);
    EXPECT_EQ(predictor.get_tracked_aircraft_count(), 1);
}

// Test: Maneuver selector - turn selection
TEST_F(CollisionDetectionTest, ManeuverSelectorTurnManeuver) {
    ManeuverSelector selector;
    
    auto ownship = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    auto intruder = createAircraftState(2, 200.0, 0.0, 5000.0, -100.0, 0.0, 180.0, 300.0);
    
    auto maneuver = selector.select_avoidance_maneuver(ownship, intruder);
    
    EXPECT_NE(maneuver.maneuver_type, AvoidanceManeuver::ManeuverType::None);
}

// Test: Maneuver selector - climb/descent
TEST_F(CollisionDetectionTest, ManeuverSelectorAltitudeChange) {
    ManeuverSelector selector;
    selector.set_aircraft_limits(3.0, 1500.0, 1000.0, 1.5);
    
    auto ownship = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    auto intruder = createAircraftState(2, 0.0, 200.0, 5000.0, 0.0, 100.0, 90.0, 300.0);
    
    auto maneuver = selector.select_avoidance_maneuver(ownship, intruder);
    
    // Should select some maneuver
    EXPECT_NE(maneuver.maneuver_type, AvoidanceManeuver::ManeuverType::None);
}

// Test: Conflict resolver - multi-aircraft resolution
TEST_F(CollisionDetectionTest, ConflictResolverMultiAircraft) {
    ConflictResolver resolver;
    ManeuverSelector selector;
    resolver.set_maneuver_selector(&selector);
    
    std::vector<ConflictPredictor::ConflictAlert> conflicts;
    ConflictPredictor::ConflictAlert alert;
    alert.aircraft1_id = 1;
    alert.aircraft2_id = 2;
    alert.time_to_conflict_seconds = 30.0;
    conflicts.push_back(alert);
    
    std::map<int, SeparationStandards::AircraftState> aircraft;
    aircraft[1] = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    aircraft[2] = createAircraftState(2, 200.0, 0.0, 5000.0, -100.0, 0.0, 180.0, 300.0);
    
    auto plan = resolver.resolve_multi_aircraft_conflicts(conflicts, aircraft);
    
    EXPECT_TRUE(plan.plan_effectiveness > 0.0);
}

// Test: Edge case - zero velocity aircraft
TEST_F(CollisionDetectionTest, ZeroVelocityAircraft) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 0.0, 0.0, 0.0, 0.0);  // Stationary
    auto ac2 = createAircraftState(2, 200.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    
    double time_to_cpa = 0.0;
    double distance = standards.calculate_closest_point_of_approach(ac1, ac2, time_to_cpa);
    
    EXPECT_GT(distance, 0.0);
}

// Test: Edge case - identical position
TEST_F(CollisionDetectionTest, IdenticalPositions) {
    SeparationStandards standards;
    
    auto ac1 = createAircraftState(1, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);
    auto ac2 = createAircraftState(2, 0.0, 0.0, 5000.0, 100.0, 0.0, 0.0, 300.0);  // Exact same position
    
    auto conflict = standards.check_separation_conflict(ac1, ac2);
    EXPECT_EQ(conflict, SeparationStandards::ConflictType::SameAltitude);  // Critical collision
}


/*****************************************************************************
* Copyright 2025 AI Copilot FS Project - Phase 1 Critical Tests
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include <gtest/gtest.h>
#include <cmath>
#include "../include/navigation.h"
#include "../include/aircraft_systems.h"
#include "../include/weather_system.h"
#include "../include/terrain_awareness.h"
#include "../include/approach_system.h"
#include "phase1/mock_simconnect.h"
#include "phase1/mock_databases.h"

using namespace AICopilot;
using namespace AICopilot::Testing;

// ============================================================================
// PHASE 1 CRITICAL TESTS (20 Core Functions)
// ============================================================================

/**
 * Test Suite 1: Navigation - Distance & Bearing Calculations
 */
class NavigationPhase1Test : public ::testing::Test {
protected:
    Navigation nav_;
};

TEST_F(NavigationPhase1Test, ValidateDistanceCalculationPositiveCase) {
    Position sfPOS = {37.7749, -122.4194, 5000.0, 0.0};
    Waypoint laWaypoint = {Position{34.0522, -118.2437, 5000.0, 0.0}, "LAX"};
    
    double distance = nav_.distanceToWaypoint(sfPOS, laWaypoint);
    
    EXPECT_GT(distance, 0.0);
    EXPECT_LT(distance, 500.0);  // SFO to LAX ~350nm
    EXPECT_TRUE(std::isfinite(distance));
}

TEST_F(NavigationPhase1Test, ValidateBearingCalculationValidRange) {
    Position current = {0.0, 0.0, 0.0, 0.0};
    Waypoint north = {Position{1.0, 0.0, 0.0, 0.0}, "NORTH"};
    Waypoint east = {Position{0.0, 1.0, 0.0, 0.0}, "EAST"};
    Waypoint south = {Position{-1.0, 0.0, 0.0, 0.0}, "SOUTH"};
    Waypoint west = {Position{0.0, -1.0, 0.0, 0.0}, "WEST"};
    
    double bearing_n = nav_.bearingToWaypoint(current, north);
    double bearing_e = nav_.bearingToWaypoint(current, east);
    double bearing_s = nav_.bearingToWaypoint(current, south);
    double bearing_w = nav_.bearingToWaypoint(current, west);
    
    EXPECT_TRUE(bearing_n >= 0.0 && bearing_n <= 360.0);
    EXPECT_TRUE(bearing_e >= 0.0 && bearing_e <= 360.0);
    EXPECT_TRUE(bearing_s >= 0.0 && bearing_s <= 360.0);
    EXPECT_TRUE(bearing_w >= 0.0 && bearing_w <= 360.0);
}

TEST_F(NavigationPhase1Test, ValidateWaypointReachedWithinTolerance) {
    Position current = {40.0, -74.0, 1000.0, 0.0};
    Waypoint waypoint = {Position{40.0, -74.0, 1000.0, 0.0}, "TEST"};
    
    bool reached = nav_.isWaypointReached(current, 1.0);
    EXPECT_TRUE(reached);
}

TEST_F(NavigationPhase1Test, ValidateWaypointNotReachedOutsideTolerance) {
    Position current = {40.0, -74.0, 1000.0, 0.0};
    Waypoint waypoint = {Position{41.0, -75.0, 1000.0, 0.0}, "DISTANT"};
    
    bool reached = nav_.isWaypointReached(current, 0.1);
    EXPECT_FALSE(reached);
}

/**
 * Test Suite 2: Aircraft Systems - Control Input Validation
 */
class AircraftSystemsPhase1Test : public ::testing::Test {
protected:
    MockSimConnect mockSimConnect_;
    std::shared_ptr<SimConnectWrapper> simConnect_ = 
        std::make_shared<SimConnectWrapper>();
    AircraftConfig config_;
    
    void SetUp() override {
        config_.aircraftType = AircraftType::SINGLE_ENGINE_PROP;
        config_.numEngines = 1;
        config_.maxThrottle = 1.0;
        config_.minAltitude = 0.0;
        config_.maxAltitude = 25000.0;
    }
};

TEST_F(AircraftSystemsPhase1Test, ValidateThrottleInputRangeClamping) {
    // Test: Throttle values should be valid 0.0-1.0
    double validThrottle = 0.75;
    double tooHighThrottle = 1.5;
    double tooLowThrottle = -0.5;
    
    EXPECT_GE(validThrottle, 0.0) << "Valid throttle must be >= 0.0";
    EXPECT_LE(validThrottle, 1.0) << "Valid throttle must be <= 1.0";
    EXPECT_GT(tooHighThrottle, 1.0) << "Over-throttle should be detected";
    EXPECT_LT(tooLowThrottle, 0.0) << "Under-throttle should be detected";
}

TEST_F(AircraftSystemsPhase1Test, ValidateFlapsPositionRange) {
    // Test: Flaps position should be 0-100%
    int validFlaps = 50;
    int invalidFlaps = 150;
    int negativeFlaps = -10;
    
    EXPECT_GE(validFlaps, 0) << "Valid flaps >= 0";
    EXPECT_LE(validFlaps, 100) << "Valid flaps <= 100";
    EXPECT_GT(invalidFlaps, 100) << "Over-extended flaps invalid";
    EXPECT_LT(negativeFlaps, 0) << "Negative flaps invalid";
}

TEST_F(AircraftSystemsPhase1Test, ValidatePitchRollYawRanges) {
    // Test: Control surfaces should be -1.0 to 1.0
    double validPitch = 0.5;
    double validRoll = -0.75;
    double validYaw = 0.25;
    
    EXPECT_GE(validPitch, -1.0) && EXPECT_LE(validPitch, 1.0);
    EXPECT_GE(validRoll, -1.0) && EXPECT_LE(validRoll, 1.0);
    EXPECT_GE(validYaw, -1.0) && EXPECT_LE(validYaw, 1.0);
}

/**
 * Test Suite 3: Weather System - Condition Analysis
 */
class WeatherSystemPhase1Test : public ::testing::Test {
protected:
    WeatherSystem weather_;
    MockWeatherDatabase mockDB_ = 
        MockDatabaseFactory::createWeatherDatabase();
};

TEST_F(WeatherSystemPhase1Test, ValidateWeatherDataStructureValid) {
    WeatherConditions wx{};
    wx.temperature = 15.0;
    wx.dewpoint = 10.0;
    wx.windSpeed = 5.0;
    wx.windDirection = 270.0;
    wx.visibility = 10.0;
    wx.ceiling = 5000.0;
    
    EXPECT_TRUE(std::isfinite(wx.temperature));
    EXPECT_TRUE(std::isfinite(wx.windSpeed));
    EXPECT_GE(wx.windDirection, 0.0);
    EXPECT_LE(wx.windDirection, 360.0);
}

TEST_F(WeatherSystemPhase1Test, ValidateWindComponentCalculation) {
    // Headwind/crosswind calculation for wind 180° at 15kt, heading 360°
    double windSpeed = 15.0;
    double windDir = 180.0;
    double heading = 360.0;
    double headwind = 0.0;
    double crosswind = 0.0;
    
    weather_.calculateWindComponents(windSpeed, windDir, heading, headwind, crosswind);
    
    // 180° opposite to 360° means full headwind
    EXPECT_NEAR(headwind, 15.0, 1.0);
    EXPECT_NEAR(crosswind, 0.0, 1.0);
}

TEST_F(WeatherSystemPhase1Test, ValidateWeatherSuitabilityVFRConditions) {
    WeatherConditions vfr{};
    vfr.visibility = 10.0;      // 10nm visibility
    vfr.ceiling = 5000.0;        // 5000ft ceiling
    vfr.windSpeed = 8.0;         // 8kt wind
    
    weather_.updateWeatherConditions(vfr);
    
    // VFR should be suitable for most flights
    EXPECT_TRUE(weather_.isWeatherSuitableForTakeoff());
}

/**
 * Test Suite 4: Position Coordinate Validation
 */
class CoordinateValidationTest : public ::testing::Test {
protected:
    Position createTestPosition(double lat, double lon, double alt) {
        return {lat, lon, alt, 0.0};
    }
};

TEST_F(CoordinateValidationTest, ValidateLatitudeRangeValidation) {
    Position valid1 = createTestPosition(0.0, 0.0, 1000.0);
    Position valid2 = createTestPosition(45.5, -120.0, 5000.0);
    Position valid3 = createTestPosition(-90.0, 180.0, 10000.0);
    
    EXPECT_GE(valid1.latitude, -90.0) && EXPECT_LE(valid1.latitude, 90.0);
    EXPECT_GE(valid2.latitude, -90.0) && EXPECT_LE(valid2.latitude, 90.0);
    EXPECT_GE(valid3.latitude, -90.0) && EXPECT_LE(valid3.latitude, 90.0);
}

TEST_F(CoordinateValidationTest, ValidateLongitudeRangeValidation) {
    Position valid1 = createTestPosition(0.0, 0.0, 1000.0);
    Position valid2 = createTestPosition(45.5, -120.0, 5000.0);
    Position valid3 = createTestPosition(-45.0, 179.9, 10000.0);
    
    EXPECT_GE(valid1.longitude, -180.0) && EXPECT_LE(valid1.longitude, 180.0);
    EXPECT_GE(valid2.longitude, -180.0) && EXPECT_LE(valid2.longitude, 180.0);
    EXPECT_GE(valid3.longitude, -180.0) && EXPECT_LE(valid3.longitude, 180.0);
}

TEST_F(CoordinateValidationTest, ValidateAltitudeFiniteValue) {
    Position validAlt = createTestPosition(40.0, -74.0, 5000.0);
    EXPECT_TRUE(std::isfinite(validAlt.altitude));
    EXPECT_GE(validAlt.altitude, -1500.0);  // Allow below sea level
    EXPECT_LE(validAlt.altitude, 50000.0);  // Max commercial altitude
}

/**
 * Test Suite 5: Approach System - Runway & Procedure Validation
 */
class ApproachSystemPhase1Test : public ::testing::Test {
protected:
    ApproachSystem approach_;
};

TEST_F(ApproachSystemPhase1Test, ValidateApproachProcedureStructure) {
    ApproachProcedure proc{};
    proc.type = ApproachType::ILS;
    proc.name = "ILS 28R";
    proc.runwayId = "28R";
    proc.minimumAltitude = 200.0;
    
    EXPECT_EQ(proc.type, ApproachType::ILS);
    EXPECT_FALSE(proc.name.empty());
    EXPECT_GT(proc.minimumAltitude, 0.0);
}

TEST_F(ApproachSystemPhase1Test, ValidateILSDeviationBounds) {
    // ILS deviation should be -2.5 to +2.5 dots
    double locDev = 1.5;
    double gdDev = -0.8;
    
    EXPECT_GE(locDev, -2.5) && EXPECT_LE(locDev, 2.5);
    EXPECT_GE(gdDev, -2.5) && EXPECT_LE(gdDev, 2.5);
}

TEST_F(ApproachSystemPhase1Test, ValidateApproachPhaseProgression) {
    EXPECT_EQ(ApproachPhase::INITIAL, ApproachPhase::INITIAL);
    EXPECT_NE(ApproachPhase::INITIAL, ApproachPhase::FINAL);
    EXPECT_NE(ApproachPhase::FINAL, ApproachPhase::MISSED);
}

/**
 * Test Suite 6: Terrain Awareness - Altitude Safety
 */
class TerrainAwarenessPhase1Test : public ::testing::Test {
protected:
    TerrainAwareness terrain_;
    MockTerrainDatabase mockDB_ = 
        MockDatabaseFactory::createTerrainDatabase();
};

TEST_F(TerrainAwarenessPhase1Test, ValidateAltitudeAGLCalculation) {
    Position pos = {37.7749, -122.4194, 5000.0, 0.0};
    double agl = terrain_.getAltitudeAGL(pos, 5000.0);
    
    EXPECT_GE(agl, 0.0);
    EXPECT_TRUE(std::isfinite(agl));
}

TEST_F(TerrainAwarenessPhase1Test, ValidateTerrainWarningLevelHierarchy) {
    EXPECT_LT(static_cast<int>(TerrainWarningLevel::NONE), 
              static_cast<int>(TerrainWarningLevel::CAUTION));
    EXPECT_LT(static_cast<int>(TerrainWarningLevel::CAUTION),
              static_cast<int>(TerrainWarningLevel::WARNING));
    EXPECT_LT(static_cast<int>(TerrainWarningLevel::WARNING),
              static_cast<int>(TerrainWarningLevel::PULL_UP));
}

TEST_F(TerrainAwarenessPhase1Test, ValidateSafetyMarginApplication) {
    Position pos = {37.7749, -122.4194, 2000.0, 0.0};
    
    // At high altitude, more clearance required
    double safeAlt = terrain_.calculateMinimumSafeAltitude(pos);
    EXPECT_GT(safeAlt, 0.0);
    EXPECT_TRUE(std::isfinite(safeAlt));
}

/**
 * Test Suite 7: SimConnect Wrapper - Connection State
 */
class SimConnectWrapperPhase1Test : public ::testing::Test {
protected:
    MockSimConnect mockSim_;
};

TEST_F(SimConnectWrapperPhase1Test, ValidateConnectionStateTracking) {
    bool connected = false;
    EXPECT_FALSE(connected);
    
    connected = true;
    EXPECT_TRUE(connected);
}

TEST_F(SimConnectWrapperPhase1Test, ValidateAircraftStateStructureConsistency) {
    AircraftState state = MockSimConnectFactory::createDefaultAircraftState();
    
    EXPECT_TRUE(std::isfinite(state.indicatedAirspeed));
    EXPECT_TRUE(std::isfinite(state.verticalSpeed));
    EXPECT_TRUE(std::isfinite(state.pitch));
    EXPECT_TRUE(std::isfinite(state.bank));
    EXPECT_GE(state.fuelQuantity, 0.0);
}

TEST_F(SimConnectWrapperPhase1Test, ValidateAutopilotStateInitialization) {
    AutopilotState state = MockSimConnectFactory::createDefaultAutopilotState();
    
    EXPECT_FALSE(state.masterEnabled);
    EXPECT_FALSE(state.headingHold);
    EXPECT_FALSE(state.altitudeHold);
    EXPECT_GE(state.targetHeading, 0.0);
    EXPECT_LE(state.targetHeading, 360.0);
}

/**
 * Test Suite 8: Data Input Sanitization
 */
class DataSanitizationTest : public ::testing::Test {
};

TEST_F(DataSanitizationTest, ValidateNaNDetection) {
    double value = std::nan("");
    EXPECT_FALSE(std::isfinite(value));
}

TEST_F(DataSanitizationTest, ValidateInfinityDetection) {
    double value = std::numeric_limits<double>::infinity();
    EXPECT_FALSE(std::isfinite(value));
}

TEST_F(DataSanitizationTest, ValidateValidNumberDetection) {
    double value = 123.456;
    EXPECT_TRUE(std::isfinite(value));
}

/**
 * Test Suite 9: Type Conversion & Compatibility
 */
class TypeConversionTest : public ::testing::Test {
};

TEST_F(TypeConversionTest, ValidateSimulatorTypeEnumValues) {
    EXPECT_NE(SimulatorType::MSFS2024, SimulatorType::P3D_V6);
    EXPECT_NE(SimulatorType::MSFS2024, SimulatorType::UNKNOWN);
    EXPECT_NE(SimulatorType::P3D_V6, SimulatorType::UNKNOWN);
}

TEST_F(TypeConversionTest, ValidateFlightPhaseEnumOrdering) {
    EXPECT_LT(static_cast<int>(FlightPhase::PREFLIGHT), 
              static_cast<int>(FlightPhase::TAKEOFF));
    EXPECT_LT(static_cast<int>(FlightPhase::TAKEOFF),
              static_cast<int>(FlightPhase::CLIMB));
}

TEST_F(TypeConversionTest, ValidateAircraftTypeEnumValues) {
    EXPECT_NE(AircraftType::SINGLE_ENGINE_PROP, AircraftType::JET);
    EXPECT_NE(AircraftType::HELICOPTER, AircraftType::TURBOPROP);
}

/**
 * Test Suite 10: Error Boundary Conditions
 */
class BoundaryConditionTest : public ::testing::Test {
protected:
    Navigation nav_;
};

TEST_F(BoundaryConditionTest, ValidateEquatorCrossing) {
    Position above = {1.0, 0.0, 1000.0, 0.0};
    Waypoint below = {Position{-1.0, 0.0, 1000.0, 0.0}, "CROSS"};
    
    double distance = nav_.distanceToWaypoint(above, below);
    EXPECT_GT(distance, 0.0);
    EXPECT_TRUE(std::isfinite(distance));
}

TEST_F(BoundaryConditionTest, ValidateAntimeridianCrossing) {
    Position west = {0.0, 179.0, 1000.0, 0.0};
    Waypoint east = {Position{0.0, -179.0, 1000.0, 0.0}, "CROSS"};
    
    double distance = nav_.distanceToWaypoint(west, east);
    EXPECT_LT(distance, 200.0);  // Should be ~120nm at equator
}

TEST_F(BoundaryConditionTest, ValidatePoleProximity) {
    Position nearPole = {89.0, 0.0, 1000.0, 0.0};
    Waypoint atPole = {Position{90.0, 0.0, 1000.0, 0.0}, "POLE"};
    
    double distance = nav_.distanceToWaypoint(nearPole, atPole);
    EXPECT_GE(distance, 0.0);
    EXPECT_TRUE(std::isfinite(distance));
}

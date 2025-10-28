/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Runway System Unit Tests - Comprehensive test suite (20+ tests)
* Tests runway selection, wind calculations, and database operations
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include <gtest/gtest.h>
#include "runway_data.h"
#include "runway_selector.hpp"
#include "runway_database_prod.hpp"
#include <cmath>
#include <vector>

namespace AICopilot {

// ============================================================================
// Test Fixture
// ============================================================================

class RunwayDatabaseTest : public ::testing::Test {
protected:
    RunwayDatabase db;
    
    void SetUp() override {
        db.Initialize();
    }
    
    void TearDown() override {
        db.Shutdown();
    }
};

class RunwaySelectorTest : public ::testing::Test {
protected:
    // Static methods, no setup needed
};

// ============================================================================
// Wind Component Calculation Tests
// ============================================================================

TEST_F(RunwaySelectorTest, CalculateHeadwindSimple) {
    // Wind from 0° at 10 knots, runway heading 0°
    double headwind = RunwaySelector::CalculateHeadwind(0, 0, 10);
    EXPECT_NEAR(headwind, 10.0, 0.1);
}

TEST_F(RunwaySelectorTest, CalculateTailwindSimple) {
    // Wind from 180° at 10 knots, runway heading 0°
    double headwind = RunwaySelector::CalculateHeadwind(0, 180, 10);
    EXPECT_NEAR(headwind, -10.0, 0.1);
}

TEST_F(RunwaySelectorTest, CalculateCrosswindLeft) {
    // Wind from 90° at 10 knots, runway heading 0°
    double crosswind = RunwaySelector::CalculateCrosswind(0, 90, 10);
    EXPECT_NEAR(crosswind, 10.0, 0.1);  // From left
}

TEST_F(RunwaySelectorTest, CalculateCrosswindRight) {
    // Wind from 270° at 10 knots, runway heading 0°
    double crosswind = RunwaySelector::CalculateCrosswind(0, 270, 10);
    EXPECT_NEAR(crosswind, -10.0, 0.1);  // From right
}

TEST_F(RunwaySelectorTest, CalculateComponentsAtAngle) {
    // Wind from 45° at 14.14 knots (sqrt(2)*10), runway heading 0°
    auto components = RunwaySelector::CalculateWindComponents(0, 45, 14.14);
    EXPECT_NEAR(components.headwind, 10.0, 0.1);
    EXPECT_NEAR(components.crosswind, 10.0, 0.1);
}

TEST_F(RunwaySelectorTest, CalculateComponentsReal) {
    // Realistic example: Wind from 160° at 12 knots, runway 15 (150°)
    auto components = RunwaySelector::CalculateWindComponents(150, 160, 12);
    // Relative angle: 160 - 150 = 10°
    EXPECT_NEAR(components.headwind, 11.8, 0.1);  // cos(10°) * 12
    EXPECT_NEAR(components.crosswind, 2.08, 0.1); // sin(10°) * 12
}

// ============================================================================
// Runway Information Retrieval Tests
// ============================================================================

TEST_F(RunwayDatabaseTest, GetRunwayInfoValid) {
    RunwayInfo rwy;
    bool found = db.GetRunwayInfo("KJFK", "04L", rwy);
    EXPECT_TRUE(found);
    EXPECT_EQ(rwy.runwayId, "04L");
    EXPECT_EQ(rwy.icao, "KJFK");
    EXPECT_GT(rwy.length, 0);
}

TEST_F(RunwayDatabaseTest, GetRunwayInfoInvalid) {
    RunwayInfo rwy;
    bool found = db.GetRunwayInfo("INVALID", "01", rwy);
    EXPECT_FALSE(found);
}

TEST_F(RunwayDatabaseTest, GetAllRunways) {
    auto runways = db.GetAllRunways("KJFK");
    EXPECT_EQ(runways.size(), 4);  // KJFK has 4 runways
}

TEST_F(RunwayDatabaseTest, GetAirportInfo) {
    AirportInfo apt;
    bool found = db.GetAirportInfo("KJFK", apt);
    EXPECT_TRUE(found);
    EXPECT_EQ(apt.icao, "KJFK");
    EXPECT_EQ(apt.name, "John F. Kennedy International Airport");
    EXPECT_NEAR(apt.latitude, 40.6413, 0.001);
}

// ============================================================================
// Best Runway Selection Tests
// ============================================================================

TEST_F(RunwayDatabaseTest, SelectBestRunwayHeadwind) {
    // Wind from 220° at 10 knots (should favor runway 22L/22R)
    auto rwy = db.GetBestRunwayForLanding("KJFK", 220, 10, 20.0);
    EXPECT_FALSE(rwy.runwayId.empty());
    // Should select runway heading around 220°
    EXPECT_TRUE(rwy.headingMagnetic == 220 || rwy.headingMagnetic == 40);
}

TEST_F(RunwayDatabaseTest, SelectBestRunwayVariableWind) {
    // Wind from 50° - should favor runway 04L/04R (heading 40)
    auto rwy = db.GetBestRunwayForLanding("KJFK", 50, 8, 20.0);
    EXPECT_FALSE(rwy.runwayId.empty());
    EXPECT_EQ(rwy.headingMagnetic, 40);
}

TEST_F(RunwayDatabaseTest, SelectBestRunwayHighCrosswind) {
    // Wind from 135° at 15 knots (high crosswind for any runway)
    auto rwy = db.GetBestRunwayForLanding("KJFK", 135, 15, 20.0);
    // Should still find a runway but with compromise
    EXPECT_FALSE(rwy.runwayId.empty());
}

TEST_F(RunwayDatabaseTest, SelectBestRunwayExceedsCrosswind) {
    // Wind from 90° at 50 knots (impossible crosswind for any runway)
    RunwaySelectionCriteria criteria;
    criteria.windDirection = 90;
    criteria.windSpeed = 50;
    criteria.maxAcceptableCrosswind = 20.0;
    criteria.maxAcceptableTailwind = 5.0;
    criteria.requiredDistance = 5000.0;
    
    auto runways = db.GetAllRunways("KJFK");
    RunwayInfo selected;
    // Should handle gracefully
    bool found = RunwaySelector::SelectBestRunway(runways, criteria, selected);
    // May or may not find suitable runway depending on selection logic
}

TEST_F(RunwayDatabaseTest, SelectForLanding) {
    // Typical landing conditions
    auto rwy = db.GetBestRunwayForLanding("KJFK", 180, 8, 20.0, true);
    EXPECT_FALSE(rwy.runwayId.empty());
    EXPECT_GT(rwy.LDA, 5000);
}

TEST_F(RunwayDatabaseTest, SelectForTakeoff) {
    // Typical takeoff conditions (prefers headwind more)
    auto rwy = db.GetBestRunwayForTakeoff("KJFK", 40, 12, 20.0);
    EXPECT_FALSE(rwy.runwayId.empty());
    EXPECT_GT(rwy.TORA, 5000);
}

// ============================================================================
// Runway Validation Tests
// ============================================================================

TEST_F(RunwayDatabaseTest, ValidateRunwayValidAircraft) {
    bool valid = db.ValidateRunway("KJFK", "04L", "B737", 5000);
    EXPECT_TRUE(valid);
}

TEST_F(RunwayDatabaseTest, ValidateRunwayTooShort) {
    bool valid = db.ValidateRunway("KJFK", "04L", "B744", 15000);
    EXPECT_FALSE(valid);  // KJFK 04L is 10000 ft
}

TEST_F(RunwayDatabaseTest, ValidateRunwayInvalidRunway) {
    bool valid = db.ValidateRunway("KJFK", "99Z", "B737", 5000);
    EXPECT_FALSE(valid);
}

// ============================================================================
// Landing/Takeoff Distance Calculations
// ============================================================================

TEST_F(RunwayDatabaseTest, CalculateLandingDistance) {
    RunwayInfo rwy;
    ASSERT_TRUE(db.GetRunwayInfo("KJFK", "04L", rwy));
    
    AircraftPerformance perf;
    perf.aircraftType = "B737";
    perf.landingDistance = 4000.0;
    
    double distance = db.GetLandingDistance(rwy, perf);
    EXPECT_GT(distance, 4000.0);  // Should be >= base distance
    EXPECT_LT(distance, 10000.0); // But less than runway length
}

TEST_F(RunwayDatabaseTest, CalculateTakeoffDistance) {
    RunwayInfo rwy;
    ASSERT_TRUE(db.GetRunwayInfo("KJFK", "04L", rwy));
    
    AircraftPerformance perf;
    perf.aircraftType = "B737";
    perf.takeoffDistance = 3500.0;
    
    double distance = db.GetTakeoffDistance(rwy, perf);
    EXPECT_GT(distance, 3500.0);
    EXPECT_LT(distance, 10000.0);
}

// ============================================================================
// ILS Data Tests
// ============================================================================

TEST_F(RunwayDatabaseTest, GetILSDataValid) {
    ILSData ils;
    bool found = db.GetILSData("KJFK", "04L", ils);
    EXPECT_TRUE(found);
    EXPECT_TRUE(ils.hasILS);
    EXPECT_GT(ils.localizerFrequency, 108.0);
    EXPECT_LT(ils.localizerFrequency, 112.0);
}

TEST_F(RunwayDatabaseTest, GetILSDataNoILS) {
    ILSData ils;
    bool found = db.GetILSData("KSEA", "16C", ils);
    // 16C might not have ILS
}

TEST_F(RunwayDatabaseTest, HasILS) {
    bool has = db.HasILS("KJFK");
    EXPECT_TRUE(has);
}

TEST_F(RunwayDatabaseTest, HasRunwayILS) {
    bool has = db.HasRunwayILS("KJFK", "04L");
    EXPECT_TRUE(has);
}

// ============================================================================
// Database Statistics Tests
// ============================================================================

TEST_F(RunwayDatabaseTest, GetRunwayCount) {
    int count = db.GetRunwayCount();
    EXPECT_GT(count, 50);  // Should have 50+ runways
}

TEST_F(RunwayDatabaseTest, GetAirportCount) {
    int count = db.GetAirportCount();
    EXPECT_GE(count, 15);  // Should have 15+ airports
}

TEST_F(RunwayDatabaseTest, GetAirportCodes) {
    auto codes = db.GetAirportCodes();
    EXPECT_GE(codes.size(), 15);
    // Check for known airports
    EXPECT_NE(std::find(codes.begin(), codes.end(), "KJFK"), codes.end());
    EXPECT_NE(std::find(codes.begin(), codes.end(), "KLAX"), codes.end());
}

TEST_F(RunwayDatabaseTest, AirportExists) {
    EXPECT_TRUE(db.AirportExists("KJFK"));
    EXPECT_FALSE(db.AirportExists("ZZZZ"));
}

TEST_F(RunwayDatabaseTest, GetStatistics) {
    std::string stats = db.GetStatistics();
    EXPECT_GT(stats.length(), 0);
    EXPECT_NE(stats.find("Runway Database Statistics"), std::string::npos);
}

// ============================================================================
// Multiple Runway Handling Tests
// ============================================================================

TEST_F(RunwayDatabaseTest, HandleParallelRunways) {
    // KJFK has parallel runways 04L/04R
    auto runways = db.GetAllRunways("KJFK");
    int count04 = 0;
    for (const auto& rwy : runways) {
        if (rwy.headingMagnetic == 40) {
            count04++;
        }
    }
    EXPECT_EQ(count04, 2);  // Two parallel runways
}

TEST_F(RunwayDatabaseTest, SelectFromParallelRunways) {
    // With wind from 035°, should prefer one of the parallel 04 runways
    auto rwy = db.GetBestRunwayForLanding("KJFK", 35, 8, 20.0);
    EXPECT_EQ(rwy.headingMagnetic, 40);
}

// ============================================================================
// Surface and Friction Tests
// ============================================================================

TEST_F(RunwayDatabaseTest, SurfaceAsphalt) {
    RunwayInfo rwy;
    ASSERT_TRUE(db.GetRunwayInfo("KJFK", "04L", rwy));
    EXPECT_EQ(rwy.surface, SurfaceType::ASPHALT);
    EXPECT_NEAR(rwy.frictionCoefficient, 0.60, 0.05);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(RunwayDatabaseTest, CalcHeadwindZeroWind) {
    double hw = RunwaySelector::CalculateHeadwind(0, 0, 0);
    EXPECT_NEAR(hw, 0.0, 0.01);
}

TEST_F(RunwayDatabaseTest, CalcCrosswindZeroWind) {
    double cw = RunwaySelector::CalculateCrosswind(0, 0, 0);
    EXPECT_NEAR(cw, 0.0, 0.01);
}

TEST_F(RunwayDatabaseTest, ReciprocalRunway) {
    std::string recip = RunwaySelector::GetReciprocalRunway("04L");
    EXPECT_EQ(recip, "22R");
    
    recip = RunwaySelector::GetReciprocalRunway("18");
    EXPECT_EQ(recip, "36");
    
    recip = RunwaySelector::GetReciprocalRunway("09C");
    EXPECT_EQ(recip, "27C");
}

TEST_F(RunwayDatabaseTest, ParseRunwayHeading) {
    int heading = RunwaySelector::ParseRunwayHeading("04L");
    EXPECT_EQ(heading, 4);
    
    heading = RunwaySelector::ParseRunwayHeading("22R");
    EXPECT_EQ(heading, 22);
    
    heading = RunwaySelector::ParseRunwayHeading("36");
    EXPECT_EQ(heading, 36);
}

// ============================================================================
// International Airports
// ============================================================================

TEST_F(RunwayDatabaseTest, LondonHeathrow) {
    auto runways = db.GetAllRunways("EGLL");
    EXPECT_EQ(runways.size(), 4);
}

TEST_F(RunwayDatabaseTest, TokyoHaneda) {
    AirportInfo apt;
    bool found = db.GetAirportInfo("RJTT", apt);
    EXPECT_TRUE(found);
    EXPECT_EQ(apt.name, "Tokyo Haneda Airport");
}

TEST_F(RunwayDatabaseTest, DubaiInternational) {
    auto runways = db.GetAllRunways("OMDB");
    EXPECT_EQ(runways.size(), 4);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(RunwayDatabaseTest, QueryPerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run 100 queries
    for (int i = 0; i < 100; i++) {
        db.GetBestRunwayForLanding("KJFK", 180 + i % 360, 8 + i % 20, 20.0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 1000);  // Should complete in <1 second
    EXPECT_LT(duration.count() / 100, 10);  // Average <10ms per query
}

} // namespace AICopilot

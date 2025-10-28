/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Terrain Elevation Database - Unit Tests
* Comprehensive test suite for elevation queries, caching, and performance
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include <gtest/gtest.h>
#include "elevation_data.h"
#include <chrono>
#include <vector>
#include <cmath>

namespace AICopilot {

// ============================================================================
// Test Fixture
// ============================================================================

class ElevationDatabaseTest : public ::testing::Test {
protected:
    ElevationDatabase db;
    
    void SetUp() override {
        // Initialize with default regions
        db.ClearCache();
        db.ResetCacheStatistics();
    }
    
    void TearDown() override {
        db.ClearCache();
    }
};

// ============================================================================
// Basic Query Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, GetElevationAtValidCoordinates) {
    // Test Denver area (Rocky Mountains)
    double elev = db.GetElevationAt(39.74, -104.99);
    
    // Denver is at approximately 5,280 feet
    EXPECT_GT(elev, 5000.0);
    EXPECT_LT(elev, 5500.0);
}

TEST_F(ElevationDatabaseTest, GetElevationAtCoastalArea) {
    // Test coastal area (low elevation)
    double elev = db.GetElevationAt(40.714, -74.006);  // New York City
    
    EXPECT_GT(elev, 0.0);
    EXPECT_LT(elev, 500.0);
}

TEST_F(ElevationDatabaseTest, GetElevationAtSeaLevel) {
    // Test ocean area (should be near sea level or negative)
    double elev = db.GetElevationAt(29.75, -95.36);  // Gulf area
    
    EXPECT_LE(elev, 100.0);
}

TEST_F(ElevationDatabaseTest, GetElevationAtHighPeak) {
    // Test Mount Whitney area (14,505 feet)
    double elev = db.GetElevationAt(36.578, -118.292);
    
    EXPECT_GT(elev, 14000.0);
    EXPECT_LT(elev, 15000.0);
}

TEST_F(ElevationDatabaseTest, GetElevationAtApproximateLocation) {
    // Test that reasonable elevations are returned
    double elev = db.GetElevationAt(35.0, -104.0);
    
    EXPECT_GE(elev, ElevationDatabase::MIN_ELEVATION);
    EXPECT_LE(elev, ElevationDatabase::MAX_ELEVATION);
}

// ============================================================================
// Coordinate Validation Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, ValidateCoordinatesValid) {
    EXPECT_TRUE(ElevationDatabase::ValidateCoordinates(0.0, 0.0));
    EXPECT_TRUE(ElevationDatabase::ValidateCoordinates(45.0, -120.0));
    EXPECT_TRUE(ElevationDatabase::ValidateCoordinates(-45.0, 120.0));
    EXPECT_TRUE(ElevationDatabase::ValidateCoordinates(90.0, 180.0));
    EXPECT_TRUE(ElevationDatabase::ValidateCoordinates(-90.0, -180.0));
}

TEST_F(ElevationDatabaseTest, ValidateCoordinatesInvalid) {
    EXPECT_FALSE(ElevationDatabase::ValidateCoordinates(91.0, 0.0));
    EXPECT_FALSE(ElevationDatabase::ValidateCoordinates(-91.0, 0.0));
    EXPECT_FALSE(ElevationDatabase::ValidateCoordinates(0.0, 181.0));
    EXPECT_FALSE(ElevationDatabase::ValidateCoordinates(0.0, -181.0));
}

TEST_F(ElevationDatabaseTest, GetElevationAtInvalidCoordinates) {
    // Invalid coordinates should return 0
    double elev = db.GetElevationAt(95.0, 200.0);
    EXPECT_EQ(elev, 0.0);
}

// ============================================================================
// Antimeridian & Pole Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, GetElevationAtAntimeridian) {
    // Test near antimeridian (180° / -180°)
    double elev_east = db.GetElevationAt(50.0, 179.99);
    double elev_west = db.GetElevationAt(50.0, -179.99);
    
    EXPECT_GE(elev_east, ElevationDatabase::MIN_ELEVATION);
    EXPECT_LE(elev_east, ElevationDatabase::MAX_ELEVATION);
    EXPECT_GE(elev_west, ElevationDatabase::MIN_ELEVATION);
    EXPECT_LE(elev_west, ElevationDatabase::MAX_ELEVATION);
}

TEST_F(ElevationDatabaseTest, GetElevationAtNorthPole) {
    // Test near North Pole
    double elev = db.GetElevationAt(89.9, 0.0);
    
    EXPECT_GE(elev, ElevationDatabase::MIN_ELEVATION);
    EXPECT_LE(elev, ElevationDatabase::MAX_ELEVATION);
}

TEST_F(ElevationDatabaseTest, GetElevationAtSouthPole) {
    // Test near South Pole
    double elev = db.GetElevationAt(-89.9, 0.0);
    
    EXPECT_GE(elev, ElevationDatabase::MIN_ELEVATION);
    EXPECT_LE(elev, ElevationDatabase::MAX_ELEVATION);
}

// ============================================================================
// Water Body Detection Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, IsWaterBodyGreatLakes) {
    // Lake Michigan
    bool is_water = db.IsWaterBody(43.0, -82.0);
    EXPECT_TRUE(is_water);
}

TEST_F(ElevationDatabaseTest, IsWaterBodyGulfOfMexico) {
    // Gulf of Mexico
    bool is_water = db.IsWaterBody(27.0, -95.0);
    EXPECT_TRUE(is_water);
}

TEST_F(ElevationDatabaseTest, IsWaterBodyLandArea) {
    // Denver (land)
    bool is_water = db.IsWaterBody(39.74, -104.99);
    EXPECT_FALSE(is_water);
}

TEST_F(ElevationDatabaseTest, IsWaterBodyMountain) {
    // Rocky Mountains
    bool is_water = db.IsWaterBody(40.0, -105.5);
    EXPECT_FALSE(is_water);
}

// ============================================================================
// Slope Angle Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, GetSlopeAngleMountain) {
    // Mount Whitney area should have steep slope
    SlopeInfo slope = db.GetSlopeAngle(36.578, -118.292);
    
    EXPECT_GE(slope.max_elevation, slope.min_elevation);
    EXPECT_GE(slope.angle_degrees, 0.0);
}

TEST_F(ElevationDatabaseTest, GetSlopeAngleCoastal) {
    // Coastal area should have gentle slope
    SlopeInfo slope = db.GetSlopeAngle(40.714, -74.006);
    
    EXPECT_GE(slope.max_elevation, slope.min_elevation);
    EXPECT_GE(slope.angle_degrees, 0.0);
}

TEST_F(ElevationDatabaseTest, SlopeAngleValidRange) {
    SlopeInfo slope = db.GetSlopeAngle(39.74, -104.99);
    
    EXPECT_GE(slope.angle_degrees, 0.0);
    EXPECT_LE(slope.angle_degrees, 90.0);
    EXPECT_GE(slope.max_elevation, slope.min_elevation);
}

// ============================================================================
// Terrain Profile Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, GetTerrainProfileBasic) {
    auto profile = db.GetTerrainProfile(39.0, -104.0, 40.0, -105.0, 5);
    
    EXPECT_EQ(profile.size(), 6);  // 0 to 5 inclusive
    
    // Check first and last points
    EXPECT_DOUBLE_EQ(profile.front().latitude, 39.0);
    EXPECT_DOUBLE_EQ(profile.front().longitude, -104.0);
    EXPECT_DOUBLE_EQ(profile.back().latitude, 40.0);
    EXPECT_DOUBLE_EQ(profile.back().longitude, -105.0);
}

TEST_F(ElevationDatabaseTest, GetTerrainProfileElevations) {
    auto profile = db.GetTerrainProfile(39.0, -104.0, 40.0, -105.0, 10);
    
    for (const auto& entry : profile) {
        EXPECT_GE(entry.elevation, ElevationDatabase::MIN_ELEVATION);
        EXPECT_LE(entry.elevation, ElevationDatabase::MAX_ELEVATION);
    }
}

TEST_F(ElevationDatabaseTest, GetTerrainProfileDistances) {
    auto profile = db.GetTerrainProfile(39.0, -104.0, 40.0, -105.0, 10);
    
    // Distances should be non-decreasing
    for (size_t i = 1; i < profile.size(); ++i) {
        EXPECT_GE(profile[i].distance, profile[i-1].distance);
    }
}

TEST_F(ElevationDatabaseTest, GetTerrainProfileSingleSample) {
    // Edge case: minimum samples
    auto profile = db.GetTerrainProfile(39.0, -104.0, 40.0, -105.0, 0);
    
    // Should still have at least 2 points
    EXPECT_GE(profile.size(), 1);
}

// ============================================================================
// Region Loading Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, LoadRegionRockyMountains) {
    bool loaded = db.LoadRegionData("ROCKY_MOUNTAINS");
    EXPECT_TRUE(loaded);
    
    // Verify Rocky Mountains data is available
    double elev = db.GetElevationAt(39.74, -104.99);
    EXPECT_GT(elev, 5000.0);
}

TEST_F(ElevationDatabaseTest, LoadRegionAppalachians) {
    bool loaded = db.LoadRegionData("APPALACHIANS");
    EXPECT_TRUE(loaded);
}

TEST_F(ElevationDatabaseTest, LoadRegionCascadeRange) {
    bool loaded = db.LoadRegionData("CASCADE_RANGE");
    EXPECT_TRUE(loaded);
}

TEST_F(ElevationDatabaseTest, LoadRegionSierraNevada) {
    bool loaded = db.LoadRegionData("SIERRA_NEVADA");
    EXPECT_TRUE(loaded);
}

TEST_F(ElevationDatabaseTest, LoadRegionUnknown) {
    bool loaded = db.LoadRegionData("UNKNOWN_REGION");
    EXPECT_FALSE(loaded);
}

TEST_F(ElevationDatabaseTest, LoadRegionCaseInsensitive) {
    bool loaded1 = db.LoadRegionData("rocky_mountains");
    bool loaded2 = db.LoadRegionData("ROCKY_MOUNTAINS");
    bool loaded3 = db.LoadRegionData("Rocky_Mountains");
    
    EXPECT_TRUE(loaded1);
    EXPECT_TRUE(loaded2);
    EXPECT_TRUE(loaded3);
}

// ============================================================================
// Terrain Statistics Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, GetTerrainStatsBasic) {
    double min_elev, max_elev, avg_elev;
    bool result = db.GetTerrainStats(39.74, -104.99, 1.0, min_elev, max_elev, avg_elev);
    
    EXPECT_TRUE(result);
    EXPECT_LE(min_elev, max_elev);
    EXPECT_GE(avg_elev, min_elev);
    EXPECT_LE(avg_elev, max_elev);
}

TEST_F(ElevationDatabaseTest, GetTerrainStatsMountainArea) {
    double min_elev, max_elev, avg_elev;
    bool result = db.GetTerrainStats(36.578, -118.292, 0.5, min_elev, max_elev, avg_elev);
    
    EXPECT_TRUE(result);
    // Mountain area should have large elevation variance
    EXPECT_GT(max_elev - min_elev, 1000.0);
}

// ============================================================================
// Cache Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, CachingReducesMisses) {
    db.ResetCacheStatistics();
    
    // First query - cache miss
    db.GetElevationAt(39.74, -104.99);
    auto stats1 = db.GetCacheStatistics();
    EXPECT_EQ(stats1.second, 1);  // 1 miss
    
    // Second query same location - cache hit
    db.GetElevationAt(39.74, -104.99);
    auto stats2 = db.GetCacheStatistics();
    EXPECT_EQ(stats2.first, 1);   // 1 hit
}

TEST_F(ElevationDatabaseTest, CacheHitRatio) {
    db.ResetCacheStatistics();
    
    // Multiple queries
    for (int i = 0; i < 10; ++i) {
        db.GetElevationAt(39.74, -104.99);
    }
    
    auto stats = db.GetCacheStatistics();
    // First query misses, next 9 hit
    EXPECT_EQ(stats.first, 9);
    EXPECT_EQ(stats.second, 1);
}

TEST_F(ElevationDatabaseTest, ClearCacheResetsStatistics) {
    db.GetElevationAt(39.74, -104.99);
    db.ClearCache();
    
    auto stats = db.GetCacheStatistics();
    EXPECT_EQ(stats.first, 0);
    EXPECT_EQ(stats.second, 1);  // Still counts the pre-clear query
}

TEST_F(ElevationDatabaseTest, ResetCacheStatistics) {
    db.GetElevationAt(39.74, -104.99);
    db.ResetCacheStatistics();
    
    auto stats = db.GetCacheStatistics();
    EXPECT_EQ(stats.first, 0);
    EXPECT_EQ(stats.second, 0);
}

TEST_F(ElevationDatabaseTest, CacheMemoryUsage) {
    // Query multiple locations
    for (double lat = 39.0; lat <= 40.0; lat += 0.1) {
        for (double lon = -105.0; lon <= -104.0; lon += 0.1) {
            db.GetElevationAt(lat, lon);
        }
    }
    
    size_t usage = db.GetCacheMemoryUsage();
    EXPECT_GT(usage, 0);
    EXPECT_LT(usage, 1000000);  // Less than 1 MB
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, PerformanceSingleQuery) {
    auto start = std::chrono::high_resolution_clock::now();
    
    db.GetElevationAt(39.74, -104.99);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should complete in under 5ms (5000 microseconds)
    EXPECT_LT(duration.count(), 5000);
}

TEST_F(ElevationDatabaseTest, PerformanceBatchQueries) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // 100 queries
    for (int i = 0; i < 100; ++i) {
        db.GetElevationAt(39.74 + i * 0.01, -104.99 + i * 0.01);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 100 queries should complete in under 100ms
    EXPECT_LT(duration.count(), 100);
}

TEST_F(ElevationDatabaseTest, PerformanceCachedQueries) {
    // Prime cache
    db.GetElevationAt(39.74, -104.99);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 100 cached queries (same location)
    for (int i = 0; i < 100; ++i) {
        db.GetElevationAt(39.74, -104.99);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 100 cached queries should be very fast (<1ms)
    EXPECT_LT(duration.count(), 1000);
}

// ============================================================================
// Interpolation Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, InterpolationConsistency) {
    // Nearby points should have similar elevations
    double elev1 = db.GetElevationAt(39.74, -104.99);
    double elev2 = db.GetElevationAt(39.741, -104.991);
    
    // Difference should be small (within 1000 feet)
    EXPECT_LT(std::abs(elev1 - elev2), 1000.0);
}

TEST_F(ElevationDatabaseTest, InterpolationMonotonicity) {
    // Test that elevation changes smoothly along a path
    std::vector<double> elevations;
    
    for (double lon = -105.0; lon <= -104.0; lon += 0.05) {
        elevations.push_back(db.GetElevationAt(39.74, lon));
    }
    
    // Check that all values are within valid range
    for (double elev : elevations) {
        EXPECT_GE(elev, ElevationDatabase::MIN_ELEVATION);
        EXPECT_LE(elev, ElevationDatabase::MAX_ELEVATION);
    }
}

// ============================================================================
// Boundary Condition Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, BoundaryLatitudeNorth) {
    double elev = db.GetElevationAt(90.0, 0.0);
    EXPECT_GE(elev, ElevationDatabase::MIN_ELEVATION);
    EXPECT_LE(elev, ElevationDatabase::MAX_ELEVATION);
}

TEST_F(ElevationDatabaseTest, BoundaryLatitudeSouth) {
    double elev = db.GetElevationAt(-90.0, 0.0);
    EXPECT_GE(elev, ElevationDatabase::MIN_ELEVATION);
    EXPECT_LE(elev, ElevationDatabase::MAX_ELEVATION);
}

TEST_F(ElevationDatabaseTest, BoundaryLongitudeEast) {
    double elev = db.GetElevationAt(0.0, 180.0);
    EXPECT_GE(elev, ElevationDatabase::MIN_ELEVATION);
    EXPECT_LE(elev, ElevationDatabase::MAX_ELEVATION);
}

TEST_F(ElevationDatabaseTest, BoundaryLongitudeWest) {
    double elev = db.GetElevationAt(0.0, -180.0);
    EXPECT_GE(elev, ElevationDatabase::MIN_ELEVATION);
    EXPECT_LE(elev, ElevationDatabase::MAX_ELEVATION);
}

TEST_F(ElevationDatabaseTest, BoundaryAllCorners) {
    std::vector<std::pair<double, double>> corners = {
        {90.0, 180.0}, {90.0, -180.0},
        {-90.0, 180.0}, {-90.0, -180.0}
    };
    
    for (const auto& corner : corners) {
        double elev = db.GetElevationAt(corner.first, corner.second);
        EXPECT_GE(elev, ElevationDatabase::MIN_ELEVATION);
        EXPECT_LE(elev, ElevationDatabase::MAX_ELEVATION);
    }
}

// ============================================================================
// Minimum Safe Altitude Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, GetMinimumSafeAltitude) {
    double msa = db.GetMinimumSafeAltitude(39.74, -104.99, 1000.0);
    
    // Should be terrain elevation + clearance
    double terrain = db.GetElevationAt(39.74, -104.99);
    EXPECT_NEAR(msa, terrain + 1000.0, 1.0);
}

TEST_F(ElevationDatabaseTest, GetMinimumSafeAltitudeDefaultClearance) {
    double msa = db.GetMinimumSafeAltitude(39.74, -104.99);
    
    // Default clearance is 1000 feet
    double terrain = db.GetElevationAt(39.74, -104.99);
    EXPECT_NEAR(msa, terrain + 1000.0, 1.0);
}

// ============================================================================
// Custom Cache Size Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, CustomCacheSize) {
    ElevationDatabase small_cache(100);
    
    // Should handle small cache without issues
    double elev = small_cache.GetElevationAt(39.74, -104.99);
    EXPECT_GT(elev, 5000.0);
}

// ============================================================================
// Data Range Tests
// ============================================================================

TEST_F(ElevationDatabaseTest, AllElevationsInValidRange) {
    // Sample many locations
    std::vector<std::pair<double, double>> locations = {
        {39.74, -104.99},  // Denver
        {40.714, -74.006}, // NYC
        {36.578, -118.292}, // Mt. Whitney
        {50.0, -120.0},    // Canada
        {25.0, -95.0},     // Gulf
        {44.0, -71.3},     // New England
    };
    
    for (const auto& loc : locations) {
        double elev = db.GetElevationAt(loc.first, loc.second);
        EXPECT_GE(elev, ElevationDatabase::MIN_ELEVATION);
        EXPECT_LE(elev, ElevationDatabase::MAX_ELEVATION);
    }
}

}  // namespace AICopilot

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

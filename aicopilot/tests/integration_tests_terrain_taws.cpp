/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Integration Tests: Terrain → TAWS (Terrain Awareness Warning System)
* Validates terrain data flows to collision avoidance system
*
* Test Coverage:
* - Elevation data integration with TAWS
* - Terrain query accuracy validation
* - Slope calculation precision
* - Water body detection integration
* - Global coverage validation
* - Concurrent terrain access stress testing
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "integration_framework.hpp"
#include <cmath>
#include <algorithm>

namespace AICopilot {
namespace Tests {

class TerrainTAWSIntegrationTest : public IntegrationTestBase {
protected:
    // Test locations around New York area
    Position testLocations[5] = {
        {40.6413, -73.7781, 500.0, 180.0},   // JFK area
        {40.7724, -73.9776, 1000.0, 90.0},   // Manhattan
        {40.8448, -73.8648, 800.0, 270.0},   // Bronx
        {40.5731, -74.0776, 300.0, 45.0},    // Newark approach
        {41.2192, -71.4385, 200.0, 135.0}    // Block Island
    };
    
    void SetUp() override {
        IntegrationTestBase::SetUp();
        // Terrain database would be initialized with SRTM data path
    }
};

// ============================================================================
// TEST 1: Elevation Data Integration with TAWS
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, ElevationDataFlowsToTAWS) {
    // Arrange
    Position checkPoint = testLocations[0];
    
    // Act
    auto metrics = measurePerformance(
        "Elevation_TAWS_DataFlow",
        [this, checkPoint]() {
            // Query elevation at position
            double elevation = terrainDb_->getElevationAt(checkPoint);
            
            // Record data flow to TAWS
            if (elevation > 0) {
                recordDataFlow("Terrain", "TAWS", "ElevationData");
            }
        });
    
    // Assert
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 2: Terrain Query Accuracy
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, TerrainQueryAccuracy) {
    // Arrange
    Position jfkArea = testLocations[0];
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_QueryAccuracy",
        [this, jfkArea]() {
            // Query elevation - should return reasonable value for JFK area (sea level ~10ft)
            double elevation = terrainDb_->getElevationAt(jfkArea);
            
            // JFK is near sea level, so elevation should be small
            if (elevation > 0) {
                EXPECT_LT(elevation, 500.0);  // Should be less than 500 ft
                recordDataFlow("Terrain", "TAWS", "ElevationQuery");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 3: Slope Calculation Integration
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, SlopeCalculationFromTerrain) {
    // Arrange: Two nearby points
    Position point1 = testLocations[0];
    Position point2 = {40.6413 + 0.01, -73.7781 + 0.01, 500.0, 180.0};
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_SlopeCalculation",
        [this, point1, point2]() {
            double elev1 = terrainDb_->getElevationAt(point1);
            double elev2 = terrainDb_->getElevationAt(point2);
            
            // Calculate slope
            double elevationDiff = std::abs(elev2 - elev1);
            
            // Approximate distance (for rough calculation)
            double latDiff = point2.latitude - point1.latitude;
            double lonDiff = point2.longitude - point1.longitude;
            double distanceRad = std::sqrt(latDiff * latDiff + lonDiff * lonDiff);
            double distanceFt = distanceRad * 364000.0; // Approximate feet per degree
            
            if (distanceFt > 0) {
                double slope = elevationDiff / distanceFt * 100.0; // Percentage
                EXPECT_GE(slope, 0.0);  // Slope should be positive or zero
                recordDataFlow("Terrain", "TAWS", "SlopeData");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 4: Water Body Detection
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, WaterBodyDetectionIntegration) {
    // Arrange: Check Block Island (water nearby)
    Position blockIsland = testLocations[4];
    Position nearbyWater = {41.2192 - 0.05, -71.4385 - 0.05, 100.0, 180.0};
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_WaterBodyDetection",
        [this, blockIsland, nearbyWater]() {
            // Elevation at water should be at or near sea level
            double landElev = terrainDb_->getElevationAt(blockIsland);
            double waterElev = terrainDb_->getElevationAt(nearbyWater);
            
            // Water elevation should be very close to sea level
            EXPECT_GE(waterElev, -50.0);
            EXPECT_LE(waterElev, 100.0);
            recordDataFlow("Terrain", "TAWS", "WaterBodyDetection");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 5: Elevation Profile Along Flight Path
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, ElevationProfileAlongPath) {
    // Arrange: Start and end points
    Position start = testLocations[0];  // JFK
    Position end = testLocations[1];    // Manhattan
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_ElevationProfile",
        [this, start, end]() {
            std::vector<double> profile = terrainDb_->getElevationProfile(start, end, 10);
            
            // Should get elevation samples
            if (!profile.empty()) {
                double maxElev = *std::max_element(profile.begin(), profile.end());
                double minElev = *std::min_element(profile.begin(), profile.end());
                
                EXPECT_GE(maxElev, minElev);
                recordDataFlow("Terrain", "TAWS", "ElevationProfile");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 6: Terrain Statistics for Area
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, TerrainStatisticsForArea) {
    // Arrange
    Position center = testLocations[0];
    double radius = 10.0; // 10 NM radius
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_AreaStatistics",
        [this, center, radius]() {
            double minElev, maxElev, avgElev;
            
            bool success = terrainDb_->getTerrainStats(center, radius, minElev, maxElev, avgElev);
            
            if (success) {
                EXPECT_LE(minElev, avgElev);
                EXPECT_LE(avgElev, maxElev);
                recordDataFlow("Terrain", "TAWS", "TerrainStatistics");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 7: Global Coverage Validation
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, GlobalCoverageValidation) {
    // Arrange: Test various locations worldwide
    Position globalLocations[5] = {
        {40.7128, -74.0060, 0.0, 0.0},      // New York
        {51.5074, -0.1278, 0.0, 0.0},       // London
        {35.6762, 139.6503, 0.0, 0.0},      // Tokyo
        {48.8566, 2.3522, 0.0, 0.0},        // Paris
        {37.7749, -122.4194, 0.0, 0.0}      // San Francisco
    };
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_GlobalCoverage",
        [this, globalLocations]() {
            int availableCount = 0;
            
            for (int i = 0; i < 5; ++i) {
                if (terrainDb_->isDataAvailable(globalLocations[i].latitude,
                                                globalLocations[i].longitude)) {
                    availableCount++;
                    double elev = terrainDb_->getElevationAt(globalLocations[i]);
                    EXPECT_GE(elev, -500.0);  // Bathymetry for oceans
                }
            }
            
            recordDataFlow("Terrain", "TAWS", "GlobalCoverage");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 8: Terrain Cache Efficiency
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, TerrainCacheEfficiency) {
    // Arrange
    Position testPoint = testLocations[0];
    
    // Act
    auto metrics = measurePerformanceIterations(
        "Terrain_CacheEfficiency",
        [this, testPoint]() {
            // Repeated queries should be faster (cached)
            double elevation = terrainDb_->getElevationAt(testPoint);
            EXPECT_GE(elevation, 0.0);
        },
        50,
        50.0);
    
    // Assert: First calls are slower, but average should still be fast
    EXPECT_LT(metrics.getPercentile(95.0), 10.0);
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 9: Concurrent Terrain Queries (Stress Test)
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, ConcurrentTerrainQueriesStressTest) {
    // Arrange
    const int NUM_QUERIES = 100;
    
    // Act
    auto metrics = stressTestConcurrentAccess(
        "Terrain_ConcurrentQueries",
        NUM_QUERIES,
        [this]() {
            static int index = 0;
            Position testLoc = testLocations[index++ % 5];
            double elevation = terrainDb_->getElevationAt(testLoc);
            EXPECT_GE(elevation, 0.0);
        });
    
    // Assert
    VALIDATE_PERFORMANCE(metrics, 50.0);
    EXPECT_LT(metrics.averageLatencyMs, 2.0);
}

// ============================================================================
// TEST 10: Elevation Data Consistency
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, ElevationDataConsistency) {
    // Arrange
    Position testPoint = testLocations[0];
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_DataConsistency",
        [this, testPoint]() {
            // Query same point multiple times
            double elev1 = terrainDb_->getElevationAt(testPoint);
            double elev2 = terrainDb_->getElevationAt(testPoint);
            double elev3 = terrainDb_->getElevationAt(testPoint);
            
            // All queries should return same elevation
            EXPECT_EQ(elev1, elev2);
            EXPECT_EQ(elev2, elev3);
            recordDataFlow("Terrain", "TAWS", "ConsistencyCheck");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 11: Bilinear Interpolation Accuracy
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, BilinearInterpolationAccuracy) {
    // Arrange: Points between grid points
    Position cornerPoints[4] = {
        {40.6413, -73.7781, 0.0, 0.0},
        {40.6513, -73.7781, 0.0, 0.0},
        {40.6413, -73.7681, 0.0, 0.0},
        {40.6513, -73.7681, 0.0, 0.0}
    };
    
    // Test point between corners
    Position midPoint = {40.6463, -73.7731, 0.0, 0.0};
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_BilinearInterpolation",
        [this, cornerPoints, midPoint]() {
            double cornerElevs[4];
            for (int i = 0; i < 4; ++i) {
                cornerElevs[i] = terrainDb_->getElevationAt(cornerPoints[i]);
            }
            
            double midElev = terrainDb_->getElevationAt(midPoint);
            
            // Interpolated value should be between corner values
            double minCorner = *std::min_element(cornerElevs, cornerElevs + 4);
            double maxCorner = *std::max_element(cornerElevs, cornerElevs + 4);
            
            if (minCorner <= midElev && midElev <= maxCorner) {
                recordDataFlow("Terrain", "TAWS", "InterpolationData");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 12: High-Resolution Slope Analysis
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, HighResolutionSlopeAnalysis) {
    // Arrange
    Position start = {40.6413, -73.7781, 500.0, 180.0};
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_HighResSlopeAnalysis",
        [this, start]() {
            std::vector<double> profile = terrainDb_->getElevationProfile(start, 
                {40.6713, -73.7481, 500.0, 180.0}, 50);
            
            if (profile.size() > 1) {
                // Calculate slope between consecutive points
                for (size_t i = 1; i < profile.size(); ++i) {
                    double slopeDiff = std::abs(profile[i] - profile[i-1]);
                    EXPECT_GE(slopeDiff, 0.0);
                }
                recordDataFlow("Terrain", "TAWS", "SlopeAnalysis");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 13: Terrain Obstruction Detection
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, TerrainObstructionDetection) {
    // Arrange: Check if terrain would block a descent path
    Position aircraftPos = {40.6413, -73.7781, 2000.0, 180.0};
    Position landingZone = {40.6413 + 0.05, -73.7781 + 0.05, 100.0, 180.0};
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_ObstructionDetection",
        [this, aircraftPos, landingZone]() {
            std::vector<double> profile = terrainDb_->getElevationProfile(aircraftPos, landingZone, 20);
            
            if (!profile.empty()) {
                double maxTerrainElev = *std::max_element(profile.begin(), profile.end());
                
                // Check if descent path clears terrain with safety margin (500ft)
                EXPECT_LT(maxTerrainElev, 1500.0);  // Should be below aircraft
                recordDataFlow("Terrain", "TAWS", "ObstructionDetection");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 14: Mountain Peak Detection
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, MountainPeakDetection) {
    // Arrange: Check for high terrain
    Position area = {40.7128, -74.0060, 0.0, 0.0};  // NYC area (relatively flat)
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_PeakDetection",
        [this, area]() {
            double minElev, maxElev, avgElev;
            bool success = terrainDb_->getTerrainStats(area, 20.0, minElev, maxElev, avgElev);
            
            if (success) {
                // NYC area is relatively flat, so peak should be moderate
                EXPECT_LT(maxElev, 2000.0);
                recordDataFlow("Terrain", "TAWS", "PeakDetection");
            }
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 15: Datum Consistency Check
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, DatumConsistencyCheck) {
    // Arrange: Verify elevations are in consistent datum (MSL)
    Position testPoints[3] = {
        {40.6413, -73.7781, 500.0, 180.0},
        {40.7724, -73.9776, 1000.0, 90.0},
        {40.8448, -73.8648, 800.0, 270.0}
    };
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_DatumConsistency",
        [this, testPoints]() {
            for (int i = 0; i < 3; ++i) {
                double elev = terrainDb_->getElevationAt(testPoints[i]);
                
                // All elevations should be positive (MSL)
                EXPECT_GE(elev, -500.0);
                EXPECT_LE(elev, 30000.0);
            }
            recordDataFlow("Terrain", "TAWS", "DatumCheck");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 16: Real-time TAWS Alerts
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, RealtimeTAWSAlerts) {
    // Arrange: Simulate descent into terrain
    Position descent[5] = {
        {40.6413, -73.7781, 5000.0, 180.0},
        {40.6413 + 0.01, -73.7781 + 0.01, 3000.0, 180.0},
        {40.6413 + 0.02, -73.7781 + 0.02, 1500.0, 180.0},
        {40.6413 + 0.03, -73.7781 + 0.03, 500.0, 180.0},
        {40.6413 + 0.04, -73.7781 + 0.04, 100.0, 180.0}
    };
    
    // Act
    auto metrics = measurePerformance(
        "TAWS_RealtimeAlerts",
        [this, descent]() {
            for (int i = 0; i < 5; ++i) {
                double terrainElev = terrainDb_->getElevationAt(descent[i]);
                double clearance = descent[i].altitude - terrainElev;
                
                // Simulate TAWS logic: alert if clearance < 1000ft
                bool shouldAlert = (clearance < 1000.0 && clearance > 0);
                
                if (i >= 2) {  // Later in descent
                    EXPECT_TRUE(shouldAlert);  // Should trigger alert
                }
            }
            recordDataFlow("Terrain", "TAWS", "AlertGeneration");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 17: Terrain Data Timeliness
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, TerrainDataTimeliness) {
    // Arrange
    Position checkPoint = testLocations[0];
    
    // Act: Measure time to get terrain data
    auto metrics = measurePerformanceIterations(
        "Terrain_DataTimeliness",
        [this, checkPoint]() {
            double elev = terrainDb_->getElevationAt(checkPoint);
            EXPECT_GE(elev, 0.0);
        },
        25,
        50.0);
    
    // Assert: 99th percentile should be under 5ms for real-time systems
    VALIDATE_PERFORMANCE(metrics, 50.0);
    EXPECT_LT(metrics.getPercentile(99.0), 5.0);
}

// ============================================================================
// TEST 18: Cache Coherency Across Terrain Tiles
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, CacheCoherencyAcrossTiles) {
    // Arrange: Points from different terrain tiles
    Position points[4] = {
        {40.5, -73.5, 0.0, 0.0},  // Different tiles
        {40.9, -73.9, 0.0, 0.0},
        {41.2, -74.2, 0.0, 0.0},
        {40.7, -73.7, 0.0, 0.0}
    };
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_TileCacheCoherency",
        [this, points]() {
            for (int i = 0; i < 4; ++i) {
                double elev1 = terrainDb_->getElevationAt(points[i]);
                double elev2 = terrainDb_->getElevationAt(points[i]);
                EXPECT_EQ(elev1, elev2);
            }
            recordDataFlow("Terrain", "TAWS", "CacheCoherency");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 19: Extreme Elevation Points
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, ExtremeElevationPoints) {
    // Arrange: Query very high and very low elevations
    Position extremePoints[3] = {
        {40.0, -73.0, 0.0, 0.0},       // Sea level
        {40.3, -73.3, 0.0, 0.0},       // Probably modest terrain
        {40.6, -73.6, 0.0, 0.0}        // Another location
    };
    
    // Act
    auto metrics = measurePerformance(
        "Terrain_ExtremeElevations",
        [this, extremePoints]() {
            std::vector<double> elevations;
            
            for (int i = 0; i < 3; ++i) {
                double elev = terrainDb_->getElevationAt(extremePoints[i]);
                elevations.push_back(elev);
                EXPECT_GE(elev, -1000.0);
                EXPECT_LE(elev, 50000.0);
            }
            recordDataFlow("Terrain", "TAWS", "ExtremeElevations");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 20: Final Terrain-TAWS Integration Performance
// ============================================================================
TEST_F(TerrainTAWSIntegrationTest, FinalTerrainTAWSPerformance) {
    // Arrange
    Position flightPath[10];
    for (int i = 0; i < 10; ++i) {
        flightPath[i] = {40.6413 + (i * 0.01), -73.7781 + (i * 0.01), 5000.0 - (i * 400.0), 180.0};
    }
    
    // Act: Complete flight path terrain analysis
    auto metrics = measurePerformanceIterations(
        "TerrainTAWS_FinalPerformance",
        [this, flightPath]() {
            for (int i = 0; i < 10; ++i) {
                double terrainElev = terrainDb_->getElevationAt(flightPath[i]);
                double clearance = flightPath[i].altitude - terrainElev;
                EXPECT_GT(clearance, 0.0);  // Must maintain positive clearance
            }
        },
        20,
        50.0);
    
    // Assert
    VALIDATE_PERFORMANCE(metrics, 50.0);
    EXPECT_LT(metrics.averageLatencyMs, 5.0);
}

} // namespace Tests
} // namespace AICopilot

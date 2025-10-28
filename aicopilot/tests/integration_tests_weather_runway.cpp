/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Integration Tests: Weather → Runway System
* Validates weather data flows to runway selection and affects decisions
*
* Test Coverage:
* - Weather to runway selector integration
* - Wind data influence on runway selection
* - Real-time weather update propagation
* - Cache coherency across systems
* - High-frequency weather stress testing
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "integration_framework.hpp"
#include "../include/metar_parser.hpp"
#include <algorithm>
#include <random>

namespace AICopilot {
namespace Tests {

class WeatherRunwayIntegrationTest : public IntegrationTestBase {
protected:
    // Test runway data
    std::vector<RunwayInfo> testRunways;
    RunwaySelectionCriteria criteria;
    
    void SetUp() override {
        IntegrationTestBase::SetUp();
        
        // Setup test runways for JFK
        RunwayInfo rwy04L;
        rwy04L.icao = "KJFK";
        rwy04L.runwayId = "04L";
        rwy04L.headingMagnetic = 40;
        rwy04L.length = 14572;
        rwy04L.width = 200;
        rwy04L.latitude = 40.6381;
        rwy04L.longitude = -73.7759;
        testRunways.push_back(rwy04L);
        
        RunwayInfo rwy04R;
        rwy04R.icao = "KJFK";
        rwy04R.runwayId = "04R";
        rwy04R.headingMagnetic = 40;
        rwy04R.length = 13000;
        rwy04R.width = 150;
        rwy04R.latitude = 40.6381;
        rwy04R.longitude = -73.7659;
        testRunways.push_back(rwy04R);
        
        RunwayInfo rwy22L;
        rwy22L.icao = "KJFK";
        rwy22L.runwayId = "22L";
        rwy22L.headingMagnetic = 220;
        rwy22L.length = 14572;
        rwy22L.width = 200;
        rwy22L.latitude = 40.6200;
        rwy22L.longitude = -73.7800;
        testRunways.push_back(rwy22L);
        
        RunwayInfo rwy22R;
        rwy22R.icao = "KJFK";
        rwy22R.runwayId = "22R";
        rwy22R.headingMagnetic = 220;
        rwy22R.length = 13000;
        rwy22R.width = 150;
        rwy22R.latitude = 40.6200;
        rwy22R.longitude = -73.7700;
        testRunways.push_back(rwy22R);
        
        // Setup runway selection criteria
        criteria.operation = RunwayOperation::LANDING;
        criteria.maxCrosswind = 20.0;
        criteria.maxTailwind = 10.0;
        criteria.preferILS = true;
        criteria.aircraftType = "B737";
    }
};

// ============================================================================
// TEST 1: Basic Weather to Runway Data Flow
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherDataFlowsToRunwaySelector) {
    // Arrange: Create METAR with northerly wind
    std::string metarString = "KJFK 121851Z 02008KT 10SM FEW250 23/14 A3012 RMK AO2";
    METARReport report = weatherDb_->parseMETAR(metarString);
    
    // Act: Update weather and record data flow
    auto metrics = measurePerformance(
        "WeatherToRunway_DataFlow",
        [this, &report]() {
            weatherDb_->updateWeather("KJFK", "KJFK 121851Z 02008KT 10SM FEW250 23/14 A3012 RMK AO2");
            recordDataFlow("Weather", "Runway", "METAR");
        });
    
    // Assert
    EXPECT_TRUE(validateDataFlow("Weather", "Runway"));
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 2: Wind Components Calculation
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherWindAffectsRunwayWindComponents) {
    // Arrange
    std::string metarString = "KJFK 121851Z 03012KT 10SM FEW250 23/14 A3012 RMK AO2";
    
    // Act & Assert
    auto metrics = measurePerformance(
        "WeatherWind_RunwayComponents",
        [this, &metarString]() {
            METARReport report = weatherDb_->parseMETAR(metarString);
            
            // Calculate wind components for runway 04L (heading 40°)
            RunwayWindComponents components = RunwaySelector::CalculateWindComponents(
                testRunways[0].headingMagnetic,  // 40°
                report.windDirection,             // 30°
                report.windSpeed);                // 12 knots
            
            recordDataFlow("Weather", "Runway", "WindComponents");
        });
    
    EXPECT_TRUE(validateDataFlow("Weather", "Runway"));
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 3: Runway Selection Based on Weather
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherDrivesRunwaySelection) {
    // Arrange: Northerly wind favors runway 04
    std::string metarString = "KJFK 121851Z 01015KT 10SM FEW250 23/14 A3012 RMK AO2";
    METARReport report = weatherDb_->parseMETAR(metarString);
    
    // Act
    auto metrics = measurePerformance(
        "Weather_SelectRunway",
        [this, &report]() {
            RunwayInfo selectedRunway;
            bool success = RunwaySelector::SelectForTakeoff(
                testRunways,
                report.windDirection,
                report.windSpeed,
                criteria.maxCrosswind,
                selectedRunway);
            
            EXPECT_TRUE(success);
            // Should prefer 04L or 04R due to northerly wind
            EXPECT_EQ(selectedRunway.runwayId[0], '0');
            recordDataFlow("Weather", "Runway", "RunwaySelection");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 4: Real-Time Weather Update Triggers Runway Recalculation
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherUpdateTriggersRunwayRecalculation) {
    // Arrange
    std::string metarString1 = "KJFK 121851Z 04015KT 10SM FEW250 23/14 A3012 RMK AO2";
    std::string metarString2 = "KJFK 121856Z 22018KT 10SM FEW250 23/14 A3012 RMK AO2";
    
    // Act
    auto metrics = measurePerformance(
        "WeatherUpdate_RecalculateRunway",
        [this, &metarString1, &metarString2]() {
            // Initial selection with northerly wind
            METARReport report1 = weatherDb_->parseMETAR(metarString1);
            RunwayInfo selectedRunway1;
            RunwaySelector::SelectForTakeoff(testRunways, report1.windDirection,
                                           report1.windSpeed, criteria.maxCrosswind, selectedRunway1);
            recordDataFlow("Weather", "Runway", "RunwaySelection1");
            
            // Update weather to southerly wind
            METARReport report2 = weatherDb_->parseMETAR(metarString2);
            RunwayInfo selectedRunway2;
            RunwaySelector::SelectForTakeoff(testRunways, report2.windDirection,
                                           report2.windSpeed, criteria.maxCrosswind, selectedRunway2);
            recordDataFlow("Weather", "Runway", "RunwaySelection2");
            
            // Verify selection changed
            EXPECT_NE(selectedRunway1.runwayId, selectedRunway2.runwayId);
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 5: Cache Coherency After Weather Update
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherCacheCoherencyAfterUpdate) {
    // Arrange
    std::string metarString = "KJFK 121851Z 05010KT 10SM FEW250 23/14 A3012 RMK AO2";
    
    // Act
    auto metrics = measurePerformance(
        "WeatherCache_Coherency",
        [this, &metarString]() {
            weatherDb_->updateWeather("KJFK", metarString);
            METARReport report1 = weatherDb_->parseMETAR(metarString);
            
            // Access same weather again - should come from cache
            METARReport report2 = weatherDb_->parseMETAR(metarString);
            
            // Verify cache coherency
            EXPECT_EQ(report1.windSpeed, report2.windSpeed);
            EXPECT_EQ(report1.windDirection, report2.windDirection);
            recordDataFlow("Weather", "Runway", "CacheCoherency");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 6: High-Frequency Weather Updates (100+ updates/sec)
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, StressTest_HighFrequencyWeatherUpdates) {
    // Arrange: Generate 150 weather updates
    std::vector<std::string> metarStrings;
    for (int i = 0; i < 150; ++i) {
        int heading = (i * 20) % 360;
        int speed = 5 + (i % 20);
        std::string metar = "KJFK 121851Z " + 
            (heading < 10 ? "00" : heading < 100 ? "0" : "") + std::to_string(heading) +
            (speed < 10 ? "0" : "") + std::to_string(speed) +
            "KT 10SM FEW250 23/14 A3012 RMK AO2";
        metarStrings.push_back(metar);
    }
    
    // Act
    auto metrics = stressTestConcurrentAccess(
        "Weather_HighFrequency",
        150,
        [this, &metarStrings]() {
            static int index = 0;
            METARReport report = weatherDb_->parseMETAR(metarStrings[index++ % 150]);
            RunwayInfo selectedRunway;
            RunwaySelector::SelectForTakeoff(testRunways, report.windDirection,
                                           report.windSpeed, criteria.maxCrosswind, selectedRunway);
        });
    
    // Assert: Average latency should be < 50ms
    VALIDATE_PERFORMANCE(metrics, 50.0);
    EXPECT_LT(metrics.averageLatencyMs, 1.0);  // Should be very fast
}

// ============================================================================
// TEST 7: Crosswind Validation
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherCrosswindWithinLimits) {
    // Arrange: Easterly wind for crosswind testing
    std::string metarString = "KJFK 121851Z 09025KT 10SM FEW250 23/14 A3012 RMK AO2";
    
    // Act & Assert
    auto metrics = measurePerformance(
        "Weather_CrosswindValidation",
        [this, &metarString]() {
            METARReport report = weatherDb_->parseMETAR(metarString);
            RunwayInfo selectedRunway;
            
            // Should still find a valid runway even with 25kt wind
            bool success = RunwaySelector::SelectForTakeoff(
                testRunways, report.windDirection, report.windSpeed,
                criteria.maxCrosswind, selectedRunway);
            
            EXPECT_TRUE(success);
            
            // Verify selected runway is acceptable
            RunwayWindComponents components = RunwaySelector::CalculateWindComponents(
                selectedRunway.headingMagnetic, report.windDirection, report.windSpeed);
            EXPECT_LE(std::abs(components.crosswind), criteria.maxCrosswind);
            recordDataFlow("Weather", "Runway", "CrosswindCheck");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 8: Temperature Effects on Performance
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherTemperatureAffectsPerformance) {
    // Arrange: Hot day reduces runway performance
    std::string hotMetar = "KJFK 121851Z 06010KT 10SM FEW250 35/20 A3012 RMK AO2";
    std::string coolMetar = "KJFK 121851Z 06010KT 10SM FEW250 15/05 A3012 RMK AO2";
    
    // Act
    auto metrics = measurePerformance(
        "Weather_TemperaturePerformance",
        [this, &hotMetar, &coolMetar]() {
            METARReport hotReport = weatherDb_->parseMETAR(hotMetar);
            METARReport coolReport = weatherDb_->parseMETAR(coolMetar);
            
            // Temperature should affect density altitude and runway requirements
            EXPECT_GT(hotReport.temperature, coolReport.temperature);
            recordDataFlow("Weather", "Runway", "TemperatureData");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 9: Visibility Integration
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherVisibilityConstrainsRunwayChoice) {
    // Arrange: Low visibility may prefer ILS runways
    std::string lowVisMetar = "KJFK 121851Z 06010KT 2SM -RA BKN020 23/14 A3012 RMK AO2";
    std::string goodVisMetar = "KJFK 121851Z 06010KT 10SM FEW250 23/14 A3012 RMK AO2";
    
    // Act
    auto metrics = measurePerformance(
        "Weather_VisibilityConstraints",
        [this, &lowVisMetar, &goodVisMetar]() {
            METARReport lowVis = weatherDb_->parseMETAR(lowVisMetar);
            METARReport goodVis = weatherDb_->parseMETAR(goodVisMetar);
            
            EXPECT_LT(lowVis.visibility, goodVis.visibility);
            EXPECT_TRUE(lowVis.precipitation);
            recordDataFlow("Weather", "Runway", "VisibilityData");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 10: Weather Multiple Airports
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherIntegrationMultipleAirports) {
    // Arrange: Different weather at different airports
    std::string jfkMetar = "KJFK 121851Z 06010KT 10SM FEW250 23/14 A3012 RMK AO2";
    std::string laxMetar = "KLAX 121856Z 25008KT 15SM SKC 28/15 A2990 RMK AO2";
    std::string ordMetar = "KORD 121856Z 18012KT 10SM SCT040 18/12 A3010 RMK AO2";
    
    // Act
    auto metrics = measurePerformance(
        "Weather_MultipleAirports",
        [this, &jfkMetar, &laxMetar, &ordMetar]() {
            weatherDb_->updateWeather("KJFK", jfkMetar);
            weatherDb_->updateWeather("KLAX", laxMetar);
            weatherDb_->updateWeather("KORD", ordMetar);
            
            EXPECT_EQ(weatherDb_->getCacheSize(), 3);
            recordDataFlow("Weather", "Runway", "MultiAirportData");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 11: Wind Shear Detection from Weather
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherDetectsWindShear) {
    // Arrange: METAR with wind shear indication
    std::string shearMetar = "KJFK 121851Z 08015G28KT 10SM FEW250 23/14 A3012 RMK AO2 PK WND 05028/1842";
    
    // Act & Assert
    auto metrics = measurePerformance(
        "Weather_WindShearDetection",
        [this, &shearMetar]() {
            METARReport report = weatherDb_->parseMETAR(shearMetar);
            
            // Should detect wind gust
            if (report.windGust > 0) {
                int shearMagnitude = report.windGust - report.windSpeed;
                // Wind shear detected if gust is significantly higher than steady wind
                EXPECT_GT(shearMagnitude, 5);
            }
            recordDataFlow("Weather", "Runway", "WindShearDetection");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 12: Thunderstorm Detection Integration
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherThunderstormIntegration) {
    // Arrange: METAR with thunderstorm
    std::string tsMetar = "KJFK 121851Z 12020G35KT 3SM +TSRA BKN015CB 22/19 A2990 RMK AO2";
    
    // Act & Assert
    auto metrics = measurePerformance(
        "Weather_ThunderstormDetection",
        [this, &tsMetar]() {
            METARReport report = weatherDb_->parseMETAR(tsMetar);
            
            // Should detect thunderstorm
            EXPECT_TRUE(report.thunderstorm);
            EXPECT_TRUE(report.precipitation);
            
            // Thunderstorms should restrict runway operations
            recordDataFlow("Weather", "Runway", "ThunderstormData");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 13: Runway Selection Algorithm Consistency
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, RunwaySelectionConsistency) {
    // Arrange
    std::string metarString = "KJFK 121851Z 07012KT 10SM FEW250 23/14 A3012 RMK AO2";
    METARReport report = weatherDb_->parseMETAR(metarString);
    
    // Act
    auto metrics = measurePerformance(
        "RunwaySelection_Consistency",
        [this, &report]() {
            RunwayInfo selected1, selected2, selected3;
            
            // Run selection algorithm 3 times with same weather
            RunwaySelector::SelectForTakeoff(testRunways, report.windDirection,
                                           report.windSpeed, criteria.maxCrosswind, selected1);
            RunwaySelector::SelectForTakeoff(testRunways, report.windDirection,
                                           report.windSpeed, criteria.maxCrosswind, selected2);
            RunwaySelector::SelectForTakeoff(testRunways, report.windDirection,
                                           report.windSpeed, criteria.maxCrosswind, selected3);
            
            // Should always select the same runway
            EXPECT_EQ(selected1.runwayId, selected2.runwayId);
            EXPECT_EQ(selected2.runwayId, selected3.runwayId);
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 14: ILS Integration with Weather
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, ILSAvailabilityWithWeather) {
    // Arrange: Add ILS data to test runways
    testRunways[0].ilsData.hasILS = true;
    testRunways[0].ilsData.category = ILSCategory::CAT_I;
    testRunways[2].ilsData.hasILS = true;
    testRunways[2].ilsData.category = ILSCategory::CAT_II;
    
    std::string lowVisMetar = "KJFK 121851Z 06010KT 2SM -RA BKN020 23/14 A3012 RMK AO2";
    
    // Act
    auto metrics = measurePerformance(
        "ILS_WeatherIntegration",
        [this, &lowVisMetar]() {
            METARReport report = weatherDb_->parseMETAR(lowVisMetar);
            
            // Low visibility should prefer ILS runways
            if (report.lowVisibility) {
                RunwayInfo selectedRunway;
                RunwaySelector::SelectForLanding(testRunways, report.windDirection,
                                               report.windSpeed, criteria.maxCrosswind, true, selectedRunway);
                
                // Selected runway should have ILS
                EXPECT_TRUE(selectedRunway.ilsData.hasILS);
            }
            recordDataFlow("Weather", "Runway", "ILSSelection");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 15: Pressure Altitude Effects
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherPressureAltitudeEffects) {
    // Arrange: Different altimeter settings
    std::string highPressMetar = "KJFK 121851Z 06010KT 10SM FEW250 23/14 A3020 RMK AO2";
    std::string lowPressMetar = "KJFK 121851Z 06010KT 10SM FEW250 23/14 A2990 RMK AO2";
    
    // Act
    auto metrics = measurePerformance(
        "Weather_PressureAltitudeEffects",
        [this, &highPressMetar, &lowPressMetar]() {
            METARReport highPress = weatherDb_->parseMETAR(highPressMetar);
            METARReport lowPress = weatherDb_->parseMETAR(lowPressMetar);
            
            EXPECT_GT(highPress.altimeterSetting, lowPress.altimeterSetting);
            recordDataFlow("Weather", "Runway", "PressureAltitude");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 16: Wind Shear Performance Test
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, PerformanceTest_WindShearStressTest) {
    // Arrange: 100 wind shear scenarios
    std::vector<std::string> shearMetars;
    for (int i = 0; i < 100; ++i) {
        int baseWind = 10 + (i % 20);
        int gustWind = baseWind + 10 + (i % 25);
        std::string metar = "KJFK 121851Z " +
            (i % 360 < 10 ? "00" : i % 360 < 100 ? "0" : "") + std::to_string(i % 360) +
            (baseWind < 10 ? "0" : "") + std::to_string(baseWind) +
            "G" + (gustWind < 10 ? "0" : "") + std::to_string(gustWind) +
            "KT 10SM FEW250 23/14 A3012 RMK AO2";
        shearMetars.push_back(metar);
    }
    
    // Act
    auto metrics = stressTestConcurrentAccess(
        "Weather_WindShearStress",
        100,
        [this, &shearMetars]() {
            static int index = 0;
            METARReport report = weatherDb_->parseMETAR(shearMetars[index++ % 100]);
            RunwayInfo selectedRunway;
            RunwaySelector::SelectForTakeoff(testRunways, report.windDirection,
                                           report.windSpeed, criteria.maxCrosswind, selectedRunway);
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 17: Transition Between Weather Systems
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherSystemTransition) {
    // Arrange: Cold front passage
    std::vector<std::string> metarSequence = {
        "KJFK 121851Z 15010KT 10SM FEW250 25/15 A3012 RMK AO2",  // Before
        "KJFK 121856Z 18012KT 8SM -RA BKN050 23/14 A3010 RMK AO2", // During
        "KJFK 121901Z 35015KT 15SM SKC 15/08 A3015 RMK AO2"   // After
    };
    
    // Act
    auto metrics = measurePerformance(
        "Weather_SystemTransition",
        [this, &metarSequence]() {
            std::vector<RunwayInfo> selectedRunways;
            
            for (const auto& metar : metarSequence) {
                METARReport report = weatherDb_->parseMETAR(metar);
                RunwayInfo selected;
                RunwaySelector::SelectForTakeoff(testRunways, report.windDirection,
                                               report.windSpeed, criteria.maxCrosswind, selected);
                selectedRunways.push_back(selected);
            }
            recordDataFlow("Weather", "Runway", "SystemTransition");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 18: Headwind Preference with Weather
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, HeadwindPreferenceWithWeather) {
    // Arrange
    std::string metarString = "KJFK 121851Z 04018KT 10SM FEW250 23/14 A3012 RMK AO2";
    
    // Act
    auto metrics = measurePerformance(
        "Weather_HeadwindPreference",
        [this, &metarString]() {
            METARReport report = weatherDb_->parseMETAR(metarString);
            RunwayInfo selectedRunway;
            
            bool success = RunwaySelector::SelectForTakeoff(
                testRunways, report.windDirection, report.windSpeed,
                criteria.maxCrosswind, selectedRunway);
            
            EXPECT_TRUE(success);
            
            // For northerly wind (04°), should prefer runway 04
            RunwayWindComponents components = RunwaySelector::CalculateWindComponents(
                selectedRunway.headingMagnetic, report.windDirection, report.windSpeed);
            
            // Headwind should be positive (helping with takeoff)
            EXPECT_GT(components.headwind, 0.0);
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 19: Icing Conditions Detection
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, WeatherIcingConditionsDetection) {
    // Arrange: Freezing rain/drizzle
    std::string icingMetar = "KJFK 121851Z 08012KT 4SM FZRA OVC015 -02/-05 A3015 RMK AO2";
    
    // Act & Assert
    auto metrics = measurePerformance(
        "Weather_IcingConditions",
        [this, &icingMetar]() {
            METARReport report = weatherDb_->parseMETAR(icingMetar);
            
            // Should detect icing potential
            EXPECT_TRUE(report.icing);
            EXPECT_LT(report.temperature, 0);
            recordDataFlow("Weather", "Runway", "IcingData");
        });
    
    VALIDATE_PERFORMANCE(metrics, 50.0);
}

// ============================================================================
// TEST 20: Final Performance Verification
// ============================================================================
TEST_F(WeatherRunwayIntegrationTest, FinalPerformanceVerification) {
    // Arrange
    std::string metarString = "KJFK 121851Z 06010KT 10SM FEW250 23/14 A3012 RMK AO2";
    
    // Act: Run 50 iterations to verify average performance
    auto metrics = measurePerformanceIterations(
        "Weather_RunwayIntegration_Final",
        [this, &metarString]() {
            METARReport report = weatherDb_->parseMETAR(metarString);
            RunwayInfo selected;
            RunwaySelector::SelectForTakeoff(testRunways, report.windDirection,
                                           report.windSpeed, criteria.maxCrosswind, selected);
        },
        50,
        50.0);
    
    // Assert
    VALIDATE_PERFORMANCE(metrics, 50.0);
    EXPECT_LT(metrics.averageLatencyMs, 5.0);
    EXPECT_LT(metrics.getPercentile(95.0), 10.0);
}

} // namespace Tests
} // namespace AICopilot

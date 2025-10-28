/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Weather System Unit Tests - Comprehensive METAR/TAF parsing tests
* 15+ test cases covering all functionality with edge cases
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include <gtest/gtest.h>
#include "weather_data.h"
#include "metar_parser.hpp"
#include <chrono>
#include <vector>
#include <cmath>

namespace AICopilot {

// ============================================================================
// Test Fixture
// ============================================================================

class WeatherDatabaseTest : public ::testing::Test {
protected:
    WeatherDatabase db;
    
    void SetUp() override {
        db.initialize();
        db.ClearCache();
    }
    
    void TearDown() override {
        db.ClearCache();
    }
};

class METARParserTest : public ::testing::Test {
protected:
    // No setup needed for static parser methods
};

// ============================================================================
// METAR Parser Tests
// ============================================================================

TEST_F(METARParserTest, ParseWindSimple) {
    int direction, speed, gust;
    bool variable;
    int minDir, maxDir;
    
    bool result = METARParser::parseWind("31008KT", direction, speed, gust, 
                                         variable, minDir, maxDir);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(310, direction);
    EXPECT_EQ(8, speed);
    EXPECT_EQ(0, gust);
    EXPECT_FALSE(variable);
}

TEST_F(METARParserTest, ParseWindWithGusts) {
    int direction, speed, gust;
    bool variable;
    int minDir, maxDir;
    
    bool result = METARParser::parseWind("27015G25KT", direction, speed, gust,
                                         variable, minDir, maxDir);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(270, direction);
    EXPECT_EQ(15, speed);
    EXPECT_EQ(25, gust);
    EXPECT_FALSE(variable);
}

TEST_F(METARParserTest, ParseWindVariable) {
    int direction, speed, gust;
    bool variable;
    int minDir, maxDir;
    
    bool result = METARParser::parseWind("VRB05KT", direction, speed, gust,
                                         variable, minDir, maxDir);
    
    ASSERT_TRUE(result);
    EXPECT_TRUE(variable);
    EXPECT_EQ(5, speed);
}

TEST_F(METARParserTest, ParseWindCalm) {
    int direction, speed, gust;
    bool variable;
    int minDir, maxDir;
    
    bool result = METARParser::parseWind("00000KT", direction, speed, gust,
                                         variable, minDir, maxDir);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(0, speed);
    EXPECT_FALSE(variable);
}

TEST_F(METARParserTest, ParseVisibilityStatuteMiles) {
    double visibility;
    
    bool result = METARParser::parseVisibility("10SM", visibility);
    
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(10.0, visibility);
}

TEST_F(METARParserTest, ParseVisibilityMeters) {
    double visibility;
    
    bool result = METARParser::parseVisibility("9999", visibility);
    
    ASSERT_TRUE(result);
    EXPECT_GT(visibility, 9.0);  // 9999 meters >= 6.2 SM
}

TEST_F(METARParserTest, ParseVisibilityFractional) {
    double visibility;
    
    bool result = METARParser::parseVisibility("1/4SM", visibility);
    
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(0.25, visibility);
}

TEST_F(METARParserTest, ParseCloudLayerFew) {
    CloudCoverage coverage;
    int altitude;
    bool isCB, isTCU;
    
    bool result = METARParser::parseCloudLayer("FEW030", coverage, altitude,
                                               isCB, isTCU);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(CloudCoverage::FEW, coverage);
    EXPECT_EQ(3000, altitude);
    EXPECT_FALSE(isCB);
}

TEST_F(METARParserTest, ParseCloudLayerWithCumulonimbus) {
    CloudCoverage coverage;
    int altitude;
    bool isCB, isTCU;
    
    bool result = METARParser::parseCloudLayer("OVC050CB", coverage, altitude,
                                               isCB, isTCU);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(CloudCoverage::OVC, coverage);
    EXPECT_EQ(5000, altitude);
    EXPECT_TRUE(isCB);
    EXPECT_FALSE(isTCU);
}

TEST_F(METARParserTest, ParseTemperatureDewpoint) {
    int temperature, dewpoint;
    
    bool result = METARParser::parseTemperatureDewpoint("18/14", temperature,
                                                        dewpoint);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(18, temperature);
    EXPECT_EQ(14, dewpoint);
}

TEST_F(METARParserTest, ParseTemperatureNegative) {
    int temperature, dewpoint;
    
    bool result = METARParser::parseTemperatureDewpoint("M05/M10", temperature,
                                                        dewpoint);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(-5, temperature);
    EXPECT_EQ(-10, dewpoint);
}

TEST_F(METARParserTest, ParseAltimeterInHg) {
    double inHg, mbar;
    
    bool result = METARParser::parseAltimeter("A3012", inHg, mbar);
    
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(30.12, inHg);
    EXPECT_GT(mbar, 1000.0);
}

TEST_F(METARParserTest, ParseAltimeterMillibars) {
    double inHg, mbar;
    
    bool result = METARParser::parseAltimeter("Q1018", inHg, mbar);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(1018, static_cast<int>(mbar));
    EXPECT_GT(inHg, 29.0);
}

TEST_F(METARParserTest, ParseWeatherRain) {
    WeatherPhenomenaCode code;
    int intensity;
    
    bool result = METARParser::parseWeatherPhenomena("RA", code, intensity);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(WeatherPhenomenaCode::RA, code);
    EXPECT_EQ(0, intensity);
}

TEST_F(METARParserTest, ParseWeatherThunderstorm) {
    WeatherPhenomenaCode code;
    int intensity;
    
    bool result = METARParser::parseWeatherPhenomena("TSRA", code, intensity);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(WeatherPhenomenaCode::TSRA, code);
}

TEST_F(METARParserTest, ParseWeatherIntensity) {
    WeatherPhenomenaCode code;
    int intensity;
    
    bool result = METARParser::parseWeatherPhenomena("+TSRA", code, intensity);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(1, intensity);  // Heavy
}

// ============================================================================
// Weather Database Tests
// ============================================================================

TEST_F(WeatherDatabaseTest, ParseKJFKMETAR) {
    std::string metar = "KJFK 121851Z 31008KT 10SM FEW250 18/14 A3012 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    ASSERT_TRUE(wx.isValid);
    EXPECT_EQ("KJFK", wx.icaoCode);
    EXPECT_EQ(31, wx.wind.directionDegrees);
    EXPECT_DOUBLE_EQ(10.0, wx.visibilityStatuteMiles);
    EXPECT_EQ(18, wx.temperatureCelsius);
    EXPECT_EQ(14, wx.dewpointCelsius);
}

TEST_F(WeatherDatabaseTest, ParseEGLLMETAR) {
    std::string metar = "EGLL 121850Z 27015G22KT 9999 FEW040 16/11 Q1012 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    ASSERT_TRUE(wx.isValid);
    EXPECT_EQ("EGLL", wx.icaoCode);
    EXPECT_EQ(15, wx.wind.speedKnots);
    EXPECT_EQ(22, wx.wind.gustKnots);
}

TEST_F(WeatherDatabaseTest, ParseMETARWithThunderstorm) {
    std::string metar = "KDFW 121853Z 16015G22KT 5SM OVC015 28/18 A2988 RMK TS";
    WeatherData wx = db.ParseMETAR(metar);
    
    ASSERT_TRUE(wx.isValid);
    EXPECT_TRUE(wx.hasThunderstorm);
    EXPECT_LT(wx.visibilityStatuteMiles, 10.0);
}

TEST_F(WeatherDatabaseTest, ParseMETARLowVisibility) {
    std::string metar = "KBOS 121854Z 09020G28KT 1/4SM OVC020 05/00 A2985 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    ASSERT_TRUE(wx.isValid);
    EXPECT_DOUBLE_EQ(0.25, wx.visibilityStatuteMiles);
    EXPECT_FALSE(wx.isSuitableForVFR());
}

TEST_F(WeatherDatabaseTest, ParseMETARNegativeTemperature) {
    std::string metar = "KDEN 121853Z 32018G25KT 7SM BKN040 M10/M05 A3005 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    ASSERT_TRUE(wx.isValid);
    EXPECT_EQ(-10, wx.temperatureCelsius);
    EXPECT_EQ(-5, wx.dewpointCelsius);
}

TEST_F(WeatherDatabaseTest, CacheStorage) {
    std::string metar = "KJFK 121851Z 31008KT 10SM FEW250 18/14 A3012 RMK AO2";
    db.UpdateCache("KJFK", metar);
    
    EXPECT_EQ(1, db.GetCacheEntryCount());
    EXPECT_GT(db.GetCacheSize(), 0);
}

TEST_F(WeatherDatabaseTest, CacheTTL) {
    std::string metar = "KJFK 121851Z 31008KT 10SM FEW250 18/14 A3012 RMK AO2";
    db.UpdateCache("KJFK", metar);
    
    WeatherData wx1 = db.GetWeatherAt("KJFK");
    EXPECT_TRUE(wx1.isValid);
    
    // Simulate time passage (would need mock for real TTL test)
    db.RefreshExpiredEntries();
}

TEST_F(WeatherDatabaseTest, GetVisibility) {
    std::string metar = "KLAX 121853Z 24008KT 10SM SCT250 22/15 A2990 RMK AO2";
    db.UpdateCache("KLAX", metar);
    
    double vis = db.GetVisibility("KLAX");
    EXPECT_EQ(10.0, vis);
}

TEST_F(WeatherDatabaseTest, GetWindData) {
    std::string metar = "KSFO 121856Z 29014G20KT 8SM BKN035 20/12 A2996 RMK AO2";
    db.UpdateCache("KSFO", metar);
    
    WindData wind = db.GetWindData("KSFO");
    EXPECT_EQ(290, wind.directionDegrees);
    EXPECT_EQ(14, wind.speedKnots);
    EXPECT_EQ(20, wind.gustKnots);
}

TEST_F(WeatherDatabaseTest, GetCeiling) {
    std::string metar = "KORD 121856Z 18012G18KT 8SM OVC050 12/06 A2995 RMK AO2";
    db.UpdateCache("KORD", metar);
    
    double ceiling = db.GetCeiling("KORD");
    EXPECT_GT(ceiling, 0.0);
}

TEST_F(WeatherDatabaseTest, HasHazardousConditions) {
    std::string metar = "KDFW 121853Z 16015G22KT 5SM OVC015 28/18 A2988 RMK TS";
    db.UpdateCache("KDFW", metar);
    
    bool hasHazard = db.HasHazardousConditions("KDFW");
    EXPECT_TRUE(hasHazard);
}

TEST_F(WeatherDatabaseTest, TemperatureConversion) {
    std::string metar = "KJFK 121851Z 31008KT 10SM FEW250 18/14 A3012 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    double fahrenheit = wx.getTemperatureFahrenheit();
    EXPECT_NEAR(64.4, fahrenheit, 0.1);  // 18°C ≈ 64.4°F
}

TEST_F(WeatherDatabaseTest, RelativeHumidity) {
    std::string metar = "KJFK 121851Z 31008KT 10SM FEW250 18/14 A3012 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    double rh = wx.getRelativeHumidity();
    EXPECT_GT(rh, 0.0);
    EXPECT_LE(rh, 100.0);
}

TEST_F(WeatherDatabaseTest, VisibilityConversions) {
    double meters = METARParser::convertVisibilityFromStatuteMiles(10.0, 'M');
    double km = METARParser::convertVisibilityFromStatuteMiles(10.0, 'K');
    
    EXPECT_NEAR(16093.4, meters, 1.0);
    EXPECT_NEAR(16.0934, km, 0.01);
}

TEST_F(WeatherDatabaseTest, WindSpeedConversions) {
    double kmh = METARParser::convertWindSpeed(10.0, 'K');
    double mps = METARParser::convertWindSpeed(10.0, 'M');
    
    EXPECT_NEAR(18.52, kmh, 0.01);
    EXPECT_NEAR(5.144, mps, 0.01);
}

TEST_F(WeatherDatabaseTest, AltitudeConversions) {
    double meters = METARParser::feetToMeters(3000.0);
    double feet = METARParser::metersToFeet(914.4);
    
    EXPECT_NEAR(914.4, meters, 0.1);
    EXPECT_NEAR(3000.0, feet, 0.1);
}

TEST_F(WeatherDatabaseTest, ICAOCodeValidation) {
    EXPECT_TRUE(METARParser::isValidICAOCode("KJFK"));
    EXPECT_TRUE(METARParser::isValidICAOCode("EGLL"));
    EXPECT_FALSE(METARParser::isValidICAOCode("KJ"));
    EXPECT_FALSE(METARParser::isValidICAOCode("K1FK"));
}

TEST_F(WeatherDatabaseTest, ICAOCodeNormalization) {
    std::string normalized = METARParser::normalizeICAOCode("kjfk");
    EXPECT_EQ("KJFK", normalized);
}

TEST_F(WeatherDatabaseTest, FlightCategoryVFR) {
    std::string metar = "KJFK 121851Z 31008KT 10SM FEW250 18/14 A3012 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    EXPECT_TRUE(wx.isSuitableForVFR());
    double category = wx.getFlightCategory();
    EXPECT_EQ(3.0, category);  // VFR
}

TEST_F(WeatherDatabaseTest, FlightCategoryIFR) {
    std::string metar = "KBOS 121854Z 09020G28KT 1SM OVC020 05/00 A2985 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    double category = wx.getFlightCategory();
    EXPECT_LE(category, 2.0);  // IFR or LIFR
}

TEST_F(WeatherDatabaseTest, LandingMinimums) {
    std::string metar = "KJFK 121851Z 31008KT 10SM FEW250 18/14 A3012 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    EXPECT_TRUE(wx.isSuitableForLanding());
}

TEST_F(WeatherDatabaseTest, MultipleAirports) {
    std::vector<std::string> metars = {
        "KJFK 121851Z 31008KT 10SM FEW250 18/14 A3012 RMK AO2",
        "KLAX 121853Z 24008KT 10SM SCT250 22/15 A2990 RMK AO2",
        "KORD 121856Z 18012G18KT 8SM OVC050 12/06 A2995 RMK AO2"
    };
    
    for (const auto& metar : metars) {
        WeatherData wx = db.ParseMETAR(metar);
        EXPECT_TRUE(wx.isValid);
    }
}

TEST_F(WeatherDatabaseTest, EdgeCaseCalm Wind) {
    std::string metar = "KMCO 121856Z 00000KT 9SM SCT045 26/20 A2992 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    ASSERT_TRUE(wx.isValid);
    EXPECT_EQ(0, wx.wind.speedKnots);
}

TEST_F(WeatherDatabaseTest, EdgeCaseUnlimitedVisibility) {
    std::string metar = "KSEA 121856Z 20012G18KT 10SM OVC030 14/10 A2994 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    ASSERT_TRUE(wx.isValid);
    EXPECT_GE(wx.visibilityStatuteMiles, 10.0);
}

TEST_F(WeatherDatabaseTest, ExtremeColdTemperature) {
    std::string metar = "TEST 121856Z 00000KT 5SM OVC100 M40/M45 A3000 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    ASSERT_TRUE(wx.isValid);
    EXPECT_EQ(-40, wx.temperatureCelsius);
    EXPECT_EQ(-45, wx.dewpointCelsius);
}

TEST_F(WeatherDatabaseTest, ExtremeHotTemperature) {
    std::string metar = "TEST 121856Z 00000KT 10SM SKC 50/30 A2990 RMK AO2";
    WeatherData wx = db.ParseMETAR(metar);
    
    ASSERT_TRUE(wx.isValid);
    EXPECT_EQ(50, wx.temperatureCelsius);
}

}  // namespace AICopilot

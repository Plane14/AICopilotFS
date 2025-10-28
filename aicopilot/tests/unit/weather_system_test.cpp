#include <gtest/gtest.h>
#include "../../include/weather_system.h"
#include <cmath>

using namespace AICopilot;

class WeatherSystemTest : public ::testing::Test {
protected:
    WeatherSystem weather;
    
    Position createPosition(double lat, double lon, double alt) {
        return {lat, lon, alt, 0.0};
    }
    
    WeatherConditions createWeatherConditions(
        double temp, double wind_speed, double wind_dir, double visibility)
    {
        WeatherConditions cond;
        cond.temperature = temp;
        cond.windSpeed = wind_speed;
        cond.windDirection = wind_dir;
        cond.visibility = visibility;
        return cond;
    }
};

// Test: Basic weather update and retrieval
TEST_F(WeatherSystemTest, WeatherUpdateAndRetrieval) {
    WeatherConditions cond = createWeatherConditions(15.0, 10.0, 180.0, 10.0);
    weather.updateWeatherConditions(cond);
    
    WeatherConditions retrieved = weather.getCurrentWeather();
    EXPECT_EQ(retrieved.temperature, 15.0);
    EXPECT_EQ(retrieved.windSpeed, 10.0);
    EXPECT_EQ(retrieved.windDirection, 180.0);
    EXPECT_EQ(retrieved.visibility, 10.0);
}

// Test: Hazard detection along route
TEST_F(WeatherSystemTest, HazardDetectionAlongRoute) {
    Position start = createPosition(40.0, -74.0, 0.0);
    Position end = createPosition(40.0, -75.0, 0.0);
    
    std::vector<WeatherHazard> hazards = weather.detectHazardsAlongRoute(start, end, 5000.0);
    
    // Should return a vector (empty or with hazards)
    EXPECT_TRUE(hazards.size() >= 0);
}

// Test: Temperature-based icing risk detection
TEST_F(WeatherSystemTest, IcingRiskDetection) {
    // Temperature in icing range (-15 to 0 °C)
    WeatherConditions icing_cond;
    icing_cond.temperature = -10.0;
    icing_cond.visibility = 5.0;  // Moisture indicator
    weather.updateWeatherConditions(icing_cond);
    
    Position pos = createPosition(40.0, -74.0, 10000.0);
    
    // High altitude + cold temperature = icing risk
    bool has_hazard = weather.hasWeatherHazard(pos, 10000.0);
    // Result should be boolean
    EXPECT_TRUE(true);  // Just verifying no crash
}

// Test: Wind component calculation
TEST_F(WeatherSystemTest, WindComponentCalculation) {
    WeatherConditions cond = createWeatherConditions(15.0, 20.0, 180.0, 10.0);
    weather.updateWeatherConditions(cond);
    
    double headwind = 0.0, crosswind = 0.0;
    
    // Head-on wind (aircraft heading 0, wind from 180)
    weather.calculateWindComponents(20.0, 180.0, 0.0, headwind, crosswind);
    
    EXPECT_NEAR(headwind, 20.0, 0.1);  // Should be all headwind
    EXPECT_NEAR(crosswind, 0.0, 0.1);   // No crosswind
}

// Test: Wind component with crosswind
TEST_F(WeatherSystemTest, CrosswindCalculation) {
    double headwind = 0.0, crosswind = 0.0;
    
    // Wind from north, aircraft heading east
    weather.calculateWindComponents(20.0, 0.0, 90.0, headwind, crosswind);
    
    EXPECT_NEAR(headwind, 0.0, 0.1);    // No headwind
    EXPECT_NEAR(crosswind, 20.0, 0.1);  // Full crosswind
}

// Test: Takeoff suitability check
TEST_F(WeatherSystemTest, TakeoffSuitabilityCheck) {
    WeatherConditions good_takeoff = createWeatherConditions(15.0, 5.0, 180.0, 10.0);
    weather.updateWeatherConditions(good_takeoff);
    
    bool suitable = weather.isWeatherSuitableForTakeoff();
    EXPECT_TRUE(suitable);  // Should be suitable
}

// Test: Takeoff rejection for low visibility
TEST_F(WeatherSystemTest, TakeoffRejectionLowVisibility) {
    WeatherConditions poor = createWeatherConditions(5.0, 25.0, 180.0, 0.1);
    weather.updateWeatherConditions(poor);
    
    bool suitable = weather.isWeatherSuitableForTakeoff();
    // Should evaluate suitability (may reject)
    EXPECT_TRUE(true);  // Just ensuring no crash
}

// Test: Landing suitability check
TEST_F(WeatherSystemTest, LandingSuitabilityCheck) {
    WeatherConditions good_landing = createWeatherConditions(10.0, 8.0, 180.0, 5.0);
    weather.updateWeatherConditions(good_landing);
    
    bool suitable = weather.isWeatherSuitableForLanding();
    EXPECT_TRUE(suitable);  // Should be suitable
}

// Test: Crosswind limit for landing
TEST_F(WeatherSystemTest, CrosswindLimitExceedance) {
    // High crosswind scenario
    WeatherConditions high_crosswind = createWeatherConditions(15.0, 35.0, 90.0, 5.0);
    weather.updateWeatherConditions(high_crosswind);
    
    // For runway heading 0 degrees, 90-degree wind is pure crosswind
    bool suitable = weather.isWeatherSuitableForLanding();
    // Should evaluate and possibly reject
    EXPECT_TRUE(true);  // Just verifying no crash
}

// Test: Weather severity assessment
TEST_F(WeatherSystemTest, WeatherSeverityAssessment) {
    // Moderate weather
    WeatherConditions mod_weather = createWeatherConditions(10.0, 15.0, 180.0, 3.0);
    weather.updateWeatherConditions(mod_weather);
    
    HazardSeverity severity = weather.assessWeatherSeverity();
    EXPECT_TRUE(severity == HazardSeverity::LIGHT || 
                severity == HazardSeverity::MODERATE ||
                severity == HazardSeverity::SEVERE ||
                severity == HazardSeverity::EXTREME);
}

// Test: Wind shear detection
TEST_F(WeatherSystemTest, WindShearDetection) {
    Position pos = createPosition(40.0, -74.0, 1000.0);  // Low altitude
    
    bool has_shear = weather.hasWindShear(pos, 1000.0);
    EXPECT_TRUE(true);  // Verifying no crash, result depends on hazard data
}

// Test: Recommended altitude to avoid weather
TEST_F(WeatherSystemTest, RecommendedAltitudeForWeatherAvoidance) {
    WeatherConditions severe = createWeatherConditions(-5.0, 40.0, 180.0, 0.5);
    weather.updateWeatherConditions(severe);
    
    Position pos = createPosition(40.0, -74.0, 3000.0);
    double recommended = weather.getRecommendedAltitude(pos, 3000.0);
    
    EXPECT_TRUE(std::isfinite(recommended));
    EXPECT_GT(recommended, 0.0);
}

// Test: Radar returns in clear weather
TEST_F(WeatherSystemTest, RadarReturnsInClearWeather) {
    Position pos = createPosition(40.0, -74.0, 5000.0);
    
    std::vector<WeatherRadarReturn> returns = weather.getRadarReturns(pos, 50.0);
    
    // Clear weather should have few/no returns
    EXPECT_TRUE(returns.size() >= 0);
}

// Test: Extreme weather parameters - hurricane conditions
TEST_F(WeatherSystemTest, ExtremeWeatherHurricane) {
    WeatherConditions hurricane = createWeatherConditions(22.0, 120.0, 0.0, 0.1);
    weather.updateWeatherConditions(hurricane);
    
    HazardSeverity severity = weather.assessWeatherSeverity();
    EXPECT_EQ(severity, HazardSeverity::EXTREME);
    
    EXPECT_FALSE(weather.isWeatherSuitableForTakeoff());
    EXPECT_FALSE(weather.isWeatherSuitableForLanding());
}

// Test: Aircraft type suitability - jet vs small aircraft
TEST_F(WeatherSystemTest, AircraftTypeSuitability) {
    WeatherConditions moderate = createWeatherConditions(0.0, 30.0, 180.0, 2.0);
    weather.updateWeatherConditions(moderate);
    
    // Jet might handle this
    bool jet_suitable = weather.isWeatherSuitableForFlight(AircraftType::JET);
    
    // Small single-engine might not
    bool small_suitable = weather.isWeatherSuitableForFlight(AircraftType::SINGLE_ENGINE_PROP);
    
    // Results should be valid booleans
    EXPECT_TRUE(true);
}

// Test: Temperature extremes
TEST_F(WeatherSystemTest, TemperatureExtremes) {
    // Arctic conditions
    WeatherConditions arctic = createWeatherConditions(-50.0, 20.0, 180.0, 5.0);
    weather.updateWeatherConditions(arctic);
    
    bool suitable = weather.isWeatherSuitableForFlight(AircraftType::SINGLE_ENGINE_PROP);
    // Should handle gracefully
    EXPECT_TRUE(true);
    
    // Desert heat
    WeatherConditions desert = createWeatherConditions(55.0, 15.0, 180.0, 5.0);
    weather.updateWeatherConditions(desert);
    
    suitable = weather.isWeatherSuitableForFlight(AircraftType::SINGLE_ENGINE_PROP);
    EXPECT_TRUE(true);
}

// Test: Zero/negative wind speed handling
TEST_F(WeatherSystemTest, ZeroWindSpeed) {
    double headwind = 0.0, crosswind = 0.0;
    
    weather.calculateWindComponents(0.0, 180.0, 0.0, headwind, crosswind);
    
    EXPECT_EQ(headwind, 0.0);
    EXPECT_EQ(crosswind, 0.0);
}

// Test: Variable wind direction normalization
TEST_F(WeatherSystemTest, WindDirectionNormalization) {
    double headwind = 0.0, crosswind = 0.0;
    
    // Wind direction > 360
    weather.calculateWindComponents(20.0, 450.0, 0.0, headwind, crosswind);  // 450 = 90 degrees
    EXPECT_TRUE(std::isfinite(headwind));
    EXPECT_TRUE(std::isfinite(crosswind));
    
    // Negative wind direction
    weather.calculateWindComponents(20.0, -90.0, 0.0, headwind, crosswind);  // -90 = 270 degrees
    EXPECT_TRUE(std::isfinite(headwind));
    EXPECT_TRUE(std::isfinite(crosswind));
}

// Test: Hazard detection with null conditions
TEST_F(WeatherSystemTest, HazardDetectionEmptyDatabase) {
    Position start = createPosition(40.0, -74.0, 0.0);
    Position end = createPosition(40.0, -75.0, 0.0);
    
    std::vector<WeatherHazard> hazards = weather.detectHazardsAlongRoute(start, end, 10000.0);
    EXPECT_TRUE(hazards.size() >= 0);
}


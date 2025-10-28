#pragma once

#include "../../include/weather_system.h"
#include "../../include/terrain_awareness.h"
#include <gtest/gtest.h>

namespace AICopilot {
namespace Testing {

class MockWeatherSystem : public WeatherSystem {
public:
    MockWeatherSystem();
    ~MockWeatherSystem() override = default;

    // Override virtual methods if WeatherSystem has any
    void setMockWeather(const WeatherConditions& conditions);
    void addMockHazard(const WeatherHazard& hazard);
    void clearMockHazards();

private:
    std::vector<WeatherHazard> mockHazards_;
};

class MockTerrainAwareness : public TerrainAwareness {
public:
    MockTerrainAwareness();
    ~MockTerrainAwareness() override = default;

    // Test helpers
    void setMockTerrainElevation(const Position& pos, double elevation);
    void addMockObstacle(const Obstacle& obstacle);
    void clearMockObstacles();
    void setMockTerrainWarning(TerrainWarningLevel level);

private:
    std::map<std::pair<double, double>, double> elevationMap_;
    std::vector<Obstacle> mockObstacles_;
    TerrainWarningLevel currentWarningLevel_;
};

} // namespace Testing
} // namespace AICopilot

/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef WEATHER_SYSTEM_H
#define WEATHER_SYSTEM_H

#include "aicopilot_types.h"
#include <vector>
#include <memory>

namespace AICopilot {

// Weather hazard types
enum class WeatherHazardType {
    THUNDERSTORM,
    SEVERE_TURBULENCE,
    SEVERE_ICING,
    LOW_VISIBILITY,
    STRONG_WINDS,
    WIND_SHEAR,
    MICROBURST,
    NONE
};

// Weather hazard severity
enum class HazardSeverity {
    LIGHT,
    MODERATE,
    SEVERE,
    EXTREME
};

// Weather hazard structure
struct WeatherHazard {
    WeatherHazardType type;
    HazardSeverity severity;
    Position position;
    double radius;  // nautical miles
    double altitude;  // feet MSL
    double topAltitude;  // feet MSL
    std::string description;
};

// Weather radar return
struct WeatherRadarReturn {
    Position position;
    double intensity;  // 0-100 (dBZ equivalent)
    double altitude;   // feet MSL
    double topHeight;  // feet MSL
};

/**
 * Advanced Weather Analysis and Avoidance System
 */
class WeatherSystem {
public:
    WeatherSystem() = default;
    
    // Update current weather conditions
    void updateWeatherConditions(const WeatherConditions& conditions);
    
    // Get current weather
    WeatherConditions getCurrentWeather() const { return currentWeather_; }
    
    // Detect weather hazards along route
    std::vector<WeatherHazard> detectHazardsAlongRoute(
        const Position& start,
        const Position& end,
        double altitude) const;
    
    // Check if position has weather hazard
    bool hasWeatherHazard(const Position& pos, double altitude) const;
    
    // Get weather hazard at position
    WeatherHazard getWeatherHazard(const Position& pos, double altitude) const;
    
    // Calculate weather avoidance route
    std::vector<Waypoint> calculateAvoidanceRoute(
        const Position& start,
        const Position& end,
        double altitude,
        const std::vector<WeatherHazard>& hazards) const;
    
    // Assess weather severity for flight
    HazardSeverity assessWeatherSeverity() const;
    
    // Check if weather is suitable for flight
    bool isWeatherSuitableForFlight(AircraftType aircraftType) const;
    
    // Check if weather is suitable for takeoff
    bool isWeatherSuitableForTakeoff() const;
    
    // Check if weather is suitable for landing
    bool isWeatherSuitableForLanding() const;
    
    // Simulate weather radar (simplified)
    std::vector<WeatherRadarReturn> getRadarReturns(
        const Position& pos,
        double range) const;
    
    // Get recommended altitude to avoid weather
    double getRecommendedAltitude(
        const Position& pos,
        double currentAltitude) const;
    
    // Check for wind shear
    bool hasWindShear(const Position& pos, double altitude) const;
    
    // Calculate headwind/crosswind components
    void calculateWindComponents(
        double windSpeed,
        double windDirection,
        double heading,
        double& headwind,
        double& crosswind) const;
    
private:
    WeatherConditions currentWeather_;
    std::vector<WeatherHazard> activeHazards_;
    
    // Helper methods
    bool isIcingRisk(double altitude, double temperature) const;
    bool isTurbulenceRisk(double windSpeed) const;
    double calculateVisibilityRisk() const;
    HazardSeverity determineSeverity(const WeatherConditions& wx) const;
};

} // namespace AICopilot

#endif // WEATHER_SYSTEM_H

/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "weather_system.h"
#include <cmath>
#include <algorithm>

namespace AICopilot {

void WeatherSystem::updateWeatherConditions(const WeatherConditions& conditions) {
    currentWeather_ = conditions;
    
    // Generate hazards based on conditions
    activeHazards_.clear();
    
    // Check for thunderstorm conditions
    if (conditions.precipitation && conditions.cloudBase < 5000) {
        WeatherHazard hazard;
        hazard.type = WeatherHazardType::THUNDERSTORM;
        hazard.severity = conditions.turbulence ? HazardSeverity::SEVERE : HazardSeverity::MODERATE;
        hazard.description = "Thunderstorm activity";
        activeHazards_.push_back(hazard);
    }
    
    // Check for severe turbulence
    if (conditions.turbulence && conditions.windSpeed > 25.0) {
        WeatherHazard hazard;
        hazard.type = WeatherHazardType::SEVERE_TURBULENCE;
        hazard.severity = HazardSeverity::SEVERE;
        hazard.description = "Severe turbulence";
        activeHazards_.push_back(hazard);
    }
    
    // Check for icing
    if (conditions.icing) {
        WeatherHazard hazard;
        hazard.type = WeatherHazardType::SEVERE_ICING;
        hazard.severity = HazardSeverity::MODERATE;
        hazard.description = "Icing conditions";
        activeHazards_.push_back(hazard);
    }
}

std::vector<WeatherHazard> WeatherSystem::detectHazardsAlongRoute(
    const Position& start,
    const Position& end,
    double altitude) const {
    
    std::vector<WeatherHazard> hazards;
    
    // Return active hazards (simplified - real implementation would check spatial proximity)
    for (const auto& hazard : activeHazards_) {
        hazards.push_back(hazard);
    }
    
    return hazards;
}

bool WeatherSystem::hasWeatherHazard(const Position& pos, double altitude) const {
    return !activeHazards_.empty();
}

WeatherHazard WeatherSystem::getWeatherHazard(const Position& pos, double altitude) const {
    if (!activeHazards_.empty()) {
        return activeHazards_[0];
    }
    
    WeatherHazard noHazard;
    noHazard.type = WeatherHazardType::NONE;
    noHazard.severity = HazardSeverity::LIGHT;
    return noHazard;
}

std::vector<Waypoint> WeatherSystem::calculateAvoidanceRoute(
    const Position& start,
    const Position& end,
    double altitude,
    const std::vector<WeatherHazard>& hazards) const {
    
    std::vector<Waypoint> route;
    
    // Simplified avoidance - create waypoint offset from direct path
    if (!hazards.empty()) {
        Waypoint avoid;
        avoid.id = "AVOID";
        avoid.position.latitude = (start.latitude + end.latitude) / 2.0 + 0.5;
        avoid.position.longitude = (start.longitude + end.longitude) / 2.0 + 0.5;
        avoid.altitude = altitude;
        avoid.position.altitude = altitude;
        route.push_back(avoid);
    }
    
    return route;
}

HazardSeverity WeatherSystem::assessWeatherSeverity() const {
    return determineSeverity(currentWeather_);
}

bool WeatherSystem::isWeatherSuitableForFlight(AircraftType aircraftType) const {
    // Check minimum VFR conditions
    if (currentWeather_.visibility < 3.0) return false;
    if (currentWeather_.cloudBase < 1000.0) return false;
    
    // Check for severe conditions
    if (currentWeather_.turbulence && currentWeather_.windSpeed > 30.0) return false;
    
    // Icing is critical for non-deiced aircraft
    if (currentWeather_.icing) return false;
    
    return true;
}

bool WeatherSystem::isWeatherSuitableForTakeoff() const {
    if (currentWeather_.visibility < 1.0) return false;
    if (currentWeather_.windSpeed > 35.0) return false;
    return true;
}

bool WeatherSystem::isWeatherSuitableForLanding() const {
    if (currentWeather_.visibility < 0.5) return false;
    if (currentWeather_.windSpeed > 40.0) return false;
    return true;
}

std::vector<WeatherRadarReturn> WeatherSystem::getRadarReturns(
    const Position& pos,
    double range) const {
    
    std::vector<WeatherRadarReturn> returns;
    
    // Simplified radar simulation
    for (const auto& hazard : activeHazards_) {
        if (hazard.type == WeatherHazardType::THUNDERSTORM) {
            WeatherRadarReturn ret;
            ret.position = hazard.position;
            ret.intensity = (hazard.severity == HazardSeverity::SEVERE) ? 80.0 : 50.0;
            ret.altitude = hazard.altitude;
            ret.topHeight = hazard.topAltitude;
            returns.push_back(ret);
        }
    }
    
    return returns;
}

double WeatherSystem::getRecommendedAltitude(
    const Position& pos,
    double currentAltitude) const {
    
    // If icing conditions, recommend altitude change
    if (currentWeather_.icing) {
        // Move to warmer altitude
        if (currentWeather_.temperature < -10.0) {
            return std::max(currentAltitude - 2000.0, 3000.0);
        }
    }
    
    return currentAltitude;
}

bool WeatherSystem::hasWindShear(const Position& pos, double altitude) const {
    // Simplified wind shear detection
    return currentWeather_.windSpeed > 30.0 && altitude < 1000.0;
}

void WeatherSystem::calculateWindComponents(
    double windSpeed,
    double windDirection,
    double heading,
    double& headwind,
    double& crosswind) const {
    
    double windAngle = windDirection - heading;
    double windAngleRad = windAngle * M_PI / 180.0;
    
    headwind = windSpeed * std::cos(windAngleRad);
    crosswind = windSpeed * std::sin(windAngleRad);
}

// Private helper methods

bool WeatherSystem::isIcingRisk(double altitude, double temperature) const {
    // Icing typically occurs between 0°C and -20°C in visible moisture
    return (temperature >= -20.0 && temperature <= 0.0) && currentWeather_.precipitation;
}

bool WeatherSystem::isTurbulenceRisk(double windSpeed) const {
    return windSpeed > 20.0;
}

double WeatherSystem::calculateVisibilityRisk() const {
    if (currentWeather_.visibility < 1.0) return 1.0;
    if (currentWeather_.visibility < 3.0) return 0.5;
    return 0.0;
}

HazardSeverity WeatherSystem::determineSeverity(const WeatherConditions& wx) const {
    int score = 0;
    
    if (wx.visibility < 1.0) score += 3;
    else if (wx.visibility < 3.0) score += 1;
    
    if (wx.windSpeed > 40.0) score += 3;
    else if (wx.windSpeed > 25.0) score += 2;
    else if (wx.windSpeed > 15.0) score += 1;
    
    if (wx.icing) score += 2;
    if (wx.turbulence) score += 2;
    if (wx.precipitation) score += 1;
    
    if (score >= 6) return HazardSeverity::SEVERE;
    if (score >= 4) return HazardSeverity::MODERATE;
    if (score >= 2) return HazardSeverity::LIGHT;
    return HazardSeverity::LIGHT;
}

} // namespace AICopilot

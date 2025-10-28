/*****************************************************************************
* ML Features System Implementation
* Copyright 2025 AI Copilot FS Project
*****************************************************************************/

#include "ml_features.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AICopilot {
namespace ML {

std::vector<double> CombinedFeatures::flatten() const {
    std::vector<double> result;
    
    // Weather features
    result.push_back(weather.wind_magnitude);
    result.push_back(weather.wind_component_headwind);
    result.push_back(weather.wind_component_crosswind);
    result.push_back(weather.wind_direction_category);
    result.push_back(weather.pressure_trend);
    result.push_back(weather.pressure_anomaly);
    result.push_back(weather.visibility_category);
    result.push_back(weather.visibility_index);
    result.push_back(weather.temperature_dewpoint_spread);
    result.push_back(weather.gust_factor);
    
    // Runway features
    result.push_back(runway.length_index);
    result.push_back(runway.width_index);
    result.push_back(runway.surface_quality);
    result.push_back(runway.condition_score);
    result.push_back(runway.ils_availability);
    result.push_back(runway.lighting_capability);
    result.push_back(runway.runway_slope_factor);
    result.push_back(runway.safety_margin_headwind);
    result.push_back(runway.safety_margin_crosswind);
    result.push_back(runway.effective_length);
    
    // Terrain features
    result.push_back(terrain.elevation_index);
    result.push_back(terrain.slope_steepness);
    result.push_back(terrain.elevation_trend);
    result.push_back(terrain.obstacle_proximity);
    result.push_back(terrain.water_hazard_risk);
    result.push_back(terrain.terrain_roughness);
    result.push_back(terrain.approach_corridor_clearance);
    result.push_back(terrain.descent_capability);
    result.push_back(terrain.missed_approach_terrain);
    
    // Navigation features
    result.push_back(navigation.route_complexity_score);
    result.push_back(navigation.route_length_index);
    result.push_back(navigation.airway_density_index);
    result.push_back(navigation.waypoint_spacing);
    result.push_back(navigation.navigation_aid_density);
    result.push_back(navigation.procedure_complexity);
    result.push_back(navigation.terrain_following_difficulty);
    result.push_back(navigation.procedural_clearance_margin);
    result.push_back(navigation.waypoint_precision_index);
    
    return result;
}

std::vector<std::string> CombinedFeatures::getFeatureNames() {
    return {
        // Weather (10)
        "wind_magnitude", "wind_headwind", "wind_crosswind", "wind_direction",
        "pressure_trend", "pressure_anomaly", "visibility_category", "visibility_index",
        "temp_dewpoint", "gust_factor",
        // Runway (10)
        "length_index", "width_index", "surface_quality", "condition_score",
        "ils_availability", "lighting_capability", "slope_factor", "safety_headwind",
        "safety_crosswind", "effective_length",
        // Terrain (9)
        "elevation_index", "slope_steepness", "elevation_trend", "obstacle_proximity",
        "water_hazard", "terrain_roughness", "approach_clearance", "descent_capability",
        "missed_approach",
        // Navigation (9)
        "route_complexity", "route_length", "airway_density", "waypoint_spacing",
        "nav_aid_density", "procedure_complexity", "terrain_following", "clearance_margin",
        "waypoint_precision"
    };
}

MLFeatures::MLFeatures() {}

WeatherFeatures MLFeatures::extractWeatherFeatures(
    double pressure,
    double wind_speed,
    double wind_direction,
    double visibility,
    double temperature,
    double dewpoint,
    double wind_gust) {
    
    WeatherFeatures features;
    
    // Wind magnitude (0-1 normalized)
    features.wind_magnitude = normalizeValue(wind_speed, 0, 40);
    
    // Wind components (assuming runway heading of 0 degrees for baseline)
    double runway_heading = 0;
    WindComponents components = calculateWindComponents(wind_speed, wind_direction, runway_heading);
    features.wind_component_headwind = normalizeValue(components.headwind, -20, 20);
    features.wind_component_crosswind = normalizeValue(components.crosswind, -20, 20);
    
    // Wind direction category (0-4)
    int direction_category = static_cast<int>(wind_direction / 90.0) % 4;
    features.wind_direction_category = normalizeValue(direction_category, 0, 3);
    
    // Pressure trend and anomaly
    features.pressure_trend = calculatePressureTrend(pressure);
    features.pressure_anomaly = normalizeValue(std::abs(pressure - 1013.25), 0, 50);
    
    // Visibility features
    features.visibility_category = calculateVisibilityCategory(visibility);
    features.visibility_index = normalizeValue(visibility, 0, 10000);
    
    // Temperature-dewpoint spread (icing indicator)
    double spread = temperature - dewpoint;
    features.temperature_dewpoint_spread = normalizeValue(spread, 0, 15);
    
    // Gust factor
    features.gust_factor = calculateGustFactor(wind_speed, wind_gust);
    
    return features;
}

RunwayFeatures MLFeatures::extractRunwayFeatures(
    double runway_length,
    double runway_width,
    int runway_condition,
    bool ils_available,
    bool lighting_available,
    double slope) {
    
    RunwayFeatures features;
    
    features.length_index = normalizeValue(runway_length, 2000, 14000);
    features.width_index = normalizeValue(runway_width, 75, 200);
    
    features.surface_quality = 1.0 - normalizeValue(runway_condition, 0, 5);
    features.condition_score = calculateRunwayConditionScore(runway_condition);
    
    features.ils_availability = calculateILSScore(ils_available);
    features.lighting_capability = calculateLightingScore(lighting_available);
    
    features.runway_slope_factor = normalizeValue(std::abs(slope), 0, 5);
    
    // Safety margins based on condition
    features.safety_margin_headwind = 0.3 + (features.surface_quality * 0.2);
    features.safety_margin_crosswind = 0.2 + (features.surface_quality * 0.15);
    
    // Effective runway length considering conditions
    features.effective_length = runway_length * features.surface_quality;
    
    return features;
}

TerrainFeatures MLFeatures::extractTerrainFeatures(
    double elevation,
    double slope,
    bool water_nearby,
    bool obstacles,
    double obstacle_proximity,
    double water_distance) {
    
    TerrainFeatures features;
    
    features.elevation_index = normalizeValue(elevation, 0, 14000);
    features.slope_steepness = normalizeValue(std::abs(slope), 0, 10);
    
    // Elevation trend (simplistic - use slope as proxy)
    features.elevation_trend = (slope > 0) ? 0.5 : 0.5;
    
    features.obstacle_proximity = normalizeValue(obstacle_proximity, 0, 10000);
    features.water_hazard_risk = normalizeValue(water_distance, 0, 10000);
    
    features.terrain_roughness = calculateTerrainRoughness(obstacles);
    
    // Approach corridor clearance
    double min_clearance = std::min(obstacle_proximity, water_distance);
    features.approach_corridor_clearance = normalizeValue(min_clearance, 0, 5000);
    
    // Descent capability
    features.descent_capability = (obstacles || water_nearby) ? 0.7 : 1.0;
    features.missed_approach_terrain = (obstacles) ? 0.8 : 1.0;
    
    return features;
}

NavigationFeatures MLFeatures::extractNavigationFeatures(
    double route_complexity,
    double route_length,
    double airway_density,
    double waypoint_spacing,
    double navigation_aid_density,
    double procedure_complexity) {
    
    NavigationFeatures features;
    
    features.route_complexity_score = normalizeValue(route_complexity, 0, 10);
    features.route_length_index = normalizeValue(route_length, 0, 500);
    features.airway_density_index = normalizeValue(airway_density, 0, 5);
    features.waypoint_spacing = normalizeValue(waypoint_spacing, 0, 20);
    features.navigation_aid_density = clamp01(navigation_aid_density);
    features.procedure_complexity = normalizeValue(procedure_complexity, 0, 10);
    
    // Terrain following difficulty based on complexity
    features.terrain_following_difficulty = features.route_complexity_score;
    
    // Procedural clearance margin
    features.procedural_clearance_margin = 1.0 - (features.route_complexity_score * 0.2);
    
    // Waypoint precision (generally high in modern avionics)
    features.waypoint_precision_index = 0.95;
    
    return features;
}

CombinedFeatures MLFeatures::extractAllFeatures(
    double pressure, double wind_speed, double wind_direction,
    double visibility, double temperature,
    double runway_length, double runway_width,
    int runway_condition, bool ils_available, bool lighting_available,
    double elevation, double slope, bool water_nearby, bool obstacles,
    double route_complexity, double airway_density) {
    
    CombinedFeatures features;
    
    features.weather = extractWeatherFeatures(
        pressure, wind_speed, wind_direction, visibility, temperature);
    features.runway = extractRunwayFeatures(
        runway_length, runway_width, runway_condition, ils_available, lighting_available, slope);
    features.terrain = extractTerrainFeatures(
        elevation, slope, water_nearby, obstacles);
    features.navigation = extractNavigationFeatures(
        route_complexity, 100, airway_density);
    
    return features;
}

double MLFeatures::normalizeValue(double value, double min_val, double max_val) {
    if (max_val == min_val) return 0.5;
    double normalized = (value - min_val) / (max_val - min_val);
    return clamp01(normalized);
}

double MLFeatures::denormalizeValue(double norm_value, double min_val, double max_val) {
    return min_val + norm_value * (max_val - min_val);
}

double MLFeatures::clamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

int MLFeatures::getWeatherCategory(double visibility) {
    if (visibility > EXCELLENT_VISIBILITY) return 5;
    if (visibility > GOOD_VISIBILITY) return 4;
    if (visibility > MODERATE_VISIBILITY) return 3;
    if (visibility > LIMITED_VISIBILITY) return 2;
    if (visibility > POOR_VISIBILITY) return 1;
    return 0;
}

MLFeatures::WindComponents MLFeatures::calculateWindComponents(
    double wind_speed, double wind_direction,
    double runway_heading) {
    
    WindComponents components;
    double relative_direction = wind_direction - runway_heading;
    
    // Normalize to -180 to 180
    while (relative_direction > 180) relative_direction -= 360;
    while (relative_direction < -180) relative_direction += 360;
    
    double angle_rad = relative_direction * M_PI / 180.0;
    components.headwind = wind_speed * std::cos(angle_rad);
    components.crosswind = wind_speed * std::sin(angle_rad);
    
    return components;
}

double MLFeatures::assessWeatherRisk(const WeatherFeatures& features) {
    double risk = 0;
    if (features.wind_magnitude > 0.7) risk += 0.3;
    if (features.visibility_index < 0.3) risk += 0.4;
    if (std::abs(features.pressure_anomaly) > 0.3) risk += 0.2;
    if (features.gust_factor > 0.6) risk += 0.1;
    return std::min(1.0, risk);
}

double MLFeatures::assessRunwayRisk(const RunwayFeatures& features) {
    double risk = 0;
    if (features.length_index < 0.5) risk += 0.3;
    if (features.condition_score < 50) risk += 0.4;
    if (features.ils_availability < 0.5 && features.lighting_capability < 0.5) risk += 0.3;
    return std::min(1.0, risk);
}

double MLFeatures::assessTerrainRisk(const TerrainFeatures& features) {
    double risk = 0;
    if (features.obstacle_proximity < 0.3) risk += 0.4;
    if (features.water_hazard_risk < 0.3) risk += 0.3;
    if (features.terrain_roughness > 0.6) risk += 0.3;
    return std::min(1.0, risk);
}

double MLFeatures::assessNavigationRisk(const NavigationFeatures& features) {
    double risk = 0;
    if (features.route_complexity_score > 0.8) risk += 0.3;
    if (features.airway_density_index < 0.3) risk += 0.2;
    if (features.procedure_complexity > 0.7) risk += 0.3;
    if (features.waypoint_spacing > 0.8) risk += 0.2;
    return std::min(1.0, risk);
}

double MLFeatures::calculatePressureTrend(double current_pressure) {
    return normalizeValue(current_pressure, 950, 1050);
}

double MLFeatures::calculateVisibilityCategory(double visibility_meters) {
    return normalizeValue(static_cast<double>(getWeatherCategory(visibility_meters)), 0, 5);
}

double MLFeatures::calculateGustFactor(double wind_speed, double wind_gust) {
    if (wind_speed == 0) return 0;
    double gust_ratio = wind_gust / wind_speed;
    return normalizeValue(gust_ratio, 0, 1);
}

double MLFeatures::calculateRunwayConditionScore(int condition) {
    return (5.0 - condition) / 5.0 * 100.0;
}

double MLFeatures::calculateILSScore(bool ils_available) {
    return ils_available ? 1.0 : 0.5;
}

double MLFeatures::calculateLightingScore(bool lighting_available) {
    return lighting_available ? 1.0 : 0.6;
}

double MLFeatures::calculateTerrainRoughness(bool obstacles) {
    return obstacles ? 0.7 : 0.2;
}

} // namespace ML
} // namespace AICopilot

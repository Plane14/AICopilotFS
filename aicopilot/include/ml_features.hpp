/*****************************************************************************
* ML Features System - Feature extraction and engineering
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef ML_FEATURES_HPP
#define ML_FEATURES_HPP

#include <vector>
#include <string>
#include <map>
#include <cmath>

namespace AICopilot {
namespace ML {

// Weather feature representation
struct WeatherFeatures {
    double wind_magnitude;      // Combined wind effect
    double wind_component_headwind;  // Headwind component
    double wind_component_crosswind; // Crosswind component
    double wind_direction_category;  // 0-4 (head, quartering, cross, etc.)
    double pressure_trend;      // Rising/falling trend
    double pressure_anomaly;    // Deviation from standard
    double visibility_category; // 0-5 (unlimited down to <100m)
    double visibility_index;    // Normalized 0-1
    double temperature_dewpoint_spread;  // Icing indicator
    double gust_factor;         // Wind gusts vs steady
    
    WeatherFeatures() : wind_magnitude(0), wind_component_headwind(0),
                       wind_component_crosswind(0), wind_direction_category(0),
                       pressure_trend(0), pressure_anomaly(0), visibility_category(0),
                       visibility_index(1.0), temperature_dewpoint_spread(0),
                       gust_factor(0) {}
};

// Runway feature representation
struct RunwayFeatures {
    double length_index;        // Normalized runway length
    double width_index;         // Normalized runway width
    double surface_quality;     // 0-1 (dry to contaminated)
    double condition_score;     // Overall condition 0-100
    double ils_availability;    // 0-1 (not available to CAT III)
    double lighting_capability; // 0-1 (none to full precision)
    double runway_slope_factor; // Uphill/downhill effect
    double safety_margin_headwind;   // Buffer for headwind
    double safety_margin_crosswind;  // Buffer for crosswind
    double effective_length;    // Accounting for conditions
    
    RunwayFeatures() : length_index(0.7), width_index(1.0), surface_quality(1.0),
                      condition_score(100), ils_availability(0), lighting_capability(0),
                      runway_slope_factor(1.0), safety_margin_headwind(0.3),
                      safety_margin_crosswind(0.3), effective_length(5000) {}
};

// Terrain feature representation
struct TerrainFeatures {
    double elevation_index;     // Normalized elevation
    double slope_steepness;     // Angle in degrees
    double elevation_trend;     // Rising/falling terrain
    double obstacle_proximity;  // Distance to obstacles
    double water_hazard_risk;   // Distance to water
    double terrain_roughness;   // Obstacle density
    double approach_corridor_clearance;  // Height clearance
    double descent_capability;  // Terrain allows safe descent
    double missed_approach_terrain;  // Terrain for go-around
    
    TerrainFeatures() : elevation_index(0), slope_steepness(0), elevation_trend(0),
                       obstacle_proximity(10000), water_hazard_risk(10000),
                       terrain_roughness(0), approach_corridor_clearance(5000),
                       descent_capability(1.0), missed_approach_terrain(1.0) {}
};

// Navigation feature representation
struct NavigationFeatures {
    double route_complexity_score;    // 0-10 normalized to 0-1
    double route_length_index;        // Normalized route length
    double airway_density_index;      // Waypoints per 100nm normalized
    double waypoint_spacing;          // Average distance between waypoints
    double navigation_aid_density;    // VOR/NDB availability
    double procedure_complexity;      // SID/STAR complexity
    double terrain_following_difficulty;  // Difficulty of terrain following
    double procedural_clearance_margin;   // Clearance from restrictions
    double waypoint_precision_index;  // Precision of waypoints
    
    NavigationFeatures() : route_complexity_score(0.5), route_length_index(0.5),
                         airway_density_index(0.5), waypoint_spacing(5),
                         navigation_aid_density(0.5), procedure_complexity(0.5),
                         terrain_following_difficulty(0.5), procedural_clearance_margin(0.8),
                         waypoint_precision_index(0.9) {}
};

// Combined feature set for ML models
struct CombinedFeatures {
    WeatherFeatures weather;
    RunwayFeatures runway;
    TerrainFeatures terrain;
    NavigationFeatures navigation;
    
    // Get flattened feature vector
    std::vector<double> flatten() const;
    
    // Get feature names for interpretation
    static std::vector<std::string> getFeatureNames();
};

/**
 * ML Features System - Feature extraction and engineering
 * Converts raw flight data into ML-ready features
 * All features normalized to 0-1 range for optimal ML processing
 */
class MLFeatures {
public:
    MLFeatures();
    
    // Extract weather features from raw weather data
    WeatherFeatures extractWeatherFeatures(
        double pressure,
        double wind_speed,
        double wind_direction,
        double visibility,
        double temperature,
        double dewpoint = 0,
        double wind_gust = 0);
    
    // Extract runway features
    RunwayFeatures extractRunwayFeatures(
        double runway_length,
        double runway_width,
        int runway_condition,  // 0-5
        bool ils_available,
        bool lighting_available,
        double slope = 0);
    
    // Extract terrain features
    TerrainFeatures extractTerrainFeatures(
        double elevation,
        double slope,
        bool water_nearby,
        bool obstacles,
        double obstacle_proximity = 10000,
        double water_distance = 10000);
    
    // Extract navigation features
    NavigationFeatures extractNavigationFeatures(
        double route_complexity,      // 0-10
        double route_length,          // nautical miles
        double airway_density,        // waypoints per 100nm
        double waypoint_spacing = 5,  // nm
        double navigation_aid_density = 0.5,
        double procedure_complexity = 0.5);
    
    // Extract all features
    CombinedFeatures extractAllFeatures(
        double pressure, double wind_speed, double wind_direction,
        double visibility, double temperature,
        double runway_length, double runway_width,
        int runway_condition, bool ils_available, bool lighting_available,
        double elevation, double slope, bool water_nearby, bool obstacles,
        double route_complexity, double airway_density);
    
    // Feature normalization (0-1)
    static double normalizeValue(double value, double min_val, double max_val);
    static double denormalizeValue(double norm_value, double min_val, double max_val);
    
    // Clamp value to [0, 1]
    static double clamp01(double value);
    
    // Get weather category
    static int getWeatherCategory(double visibility);  // Returns 0-5
    
    // Calculate wind components
    struct WindComponents {
        double headwind;      // >0 is headwind
        double crosswind;     // >0 is right crosswind
    };
    static WindComponents calculateWindComponents(
        double wind_speed, double wind_direction,
        double runway_heading);
    
    // Risk assessment helpers
    double assessWeatherRisk(const WeatherFeatures& features);
    double assessRunwayRisk(const RunwayFeatures& features);
    double assessTerrainRisk(const TerrainFeatures& features);
    double assessNavigationRisk(const NavigationFeatures& features);
    
    // Feature statistics for anomaly detection
    struct FeatureStatistics {
        double mean;
        double std_dev;
        double min_val;
        double max_val;
    };
    
    // Calculate statistics from feature set
    FeatureStatistics calculateWeatherStatistics(
        const std::vector<WeatherFeatures>& samples);
    FeatureStatistics calculateRunwayStatistics(
        const std::vector<RunwayFeatures>& samples);
    FeatureStatistics calculateTerrainStatistics(
        const std::vector<TerrainFeatures>& samples);
    FeatureStatistics calculateNavigationStatistics(
        const std::vector<NavigationFeatures>& samples);
    
    // Get feature importance weights
    std::vector<double> getWeatherFeatureImportance() const;
    std::vector<double> getRunwayFeatureImportance() const;
    std::vector<double> getTerrainFeatureImportance() const;
    std::vector<double> getNavigationFeatureImportance() const;
    
private:
    // Feature extraction helpers
    double calculatePressureTrend(double current_pressure);
    double calculateVisibilityCategory(double visibility_meters);
    double calculateGustFactor(double wind_speed, double wind_gust);
    double calculateRunwayConditionScore(int condition);
    double calculateILSScore(bool ils_available);
    double calculateLightingScore(bool lighting_available);
    double calculateTerrainRoughness(bool obstacles);
    
    // Thresholds for categorization
    static constexpr double EXCELLENT_VISIBILITY = 10000;  // meters
    static constexpr double GOOD_VISIBILITY = 5000;
    static constexpr double MODERATE_VISIBILITY = 1500;
    static constexpr double LIMITED_VISIBILITY = 400;
    static constexpr double POOR_VISIBILITY = 100;
    
    static constexpr double CROSSWIND_LIMIT = 15;  // knots
    static constexpr double TAILWIND_LIMIT = 5;    // knots
    static constexpr double IDEAL_HEADWIND = 8;    // knots
    
    static constexpr double MAX_ELEVATION = 14000;  // feet
    static constexpr double MAX_RUNWAY_LENGTH = 14000;  // feet
    static constexpr double MAX_ROUTE_COMPLEXITY = 10;
};

} // namespace ML
} // namespace AICopilot

#endif // ML_FEATURES_HPP

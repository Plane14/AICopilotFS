/*****************************************************************************
* ML Decision Engine - Production Ready Machine Learning Decision System
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef ML_DECISION_ENGINE_HPP
#define ML_DECISION_ENGINE_HPP

#include <vector>
#include <memory>
#include <string>
#include <map>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace AICopilot {
namespace ML {

// Decision scoring structure
struct DecisionScore {
    double score;           // 0-100 decision confidence
    double confidence;      // 0-1 confidence level
    std::string action;     // Recommended action
    std::vector<double> factors;  // Contributing factors
};

// Environmental inputs for decision making
struct EnvironmentalInput {
    // Weather inputs
    double pressure;        // mb
    double wind_speed;      // knots
    double wind_direction;  // degrees
    double visibility;      // meters
    double temperature;     // celsius
    
    // Runway inputs
    double runway_length;   // feet
    double runway_width;    // feet
    int runway_condition;   // 0-5 (dry to waterlogged)
    bool ils_available;     // instrument landing system
    bool lighting_available;// runway lights
    
    // Terrain inputs
    double elevation;       // feet
    double slope;          // degrees
    bool water_nearby;     // water hazard
    bool obstacles;        // terrain obstacles
    
    // Navigation inputs
    double route_complexity;     // 0-10
    double airway_density;       // waypoints per 100nm
    double waypoint_density;     // waypoints in area
    
    EnvironmentalInput() : pressure(1013.25), wind_speed(0), wind_direction(0),
                          visibility(10000), temperature(15), runway_length(5000),
                          runway_width(150), runway_condition(0), ils_available(false),
                          lighting_available(false), elevation(0), slope(0),
                          water_nearby(false), obstacles(false), route_complexity(5),
                          airway_density(2), waypoint_density(1) {}
};

// Decision history entry for analysis
struct DecisionHistoryEntry {
    std::chrono::system_clock::time_point timestamp;
    EnvironmentalInput input;
    DecisionScore score;
    std::string decision_taken;
    bool outcome_success;
    double outcome_score;
};

// Anomaly detection result
struct AnomalyDetectionResult {
    bool is_anomaly;
    double anomaly_score;    // 0-1
    std::string anomaly_type;
    std::vector<std::string> flagged_factors;
};

/**
 * ML Decision Engine - Core scoring and decision logic
 * Provides real-time decision scoring with anomaly detection
 * Thread-safe implementation with historical analysis
 */
class MLDecisionEngine {
public:
    MLDecisionEngine();
    ~MLDecisionEngine();
    
    // Initialize engine with default parameters
    bool initialize();
    
    // Score a decision based on environmental inputs
    DecisionScore scoreDecision(const EnvironmentalInput& input);
    
    // Score multiple decisions and return best
    std::vector<DecisionScore> scoreMultipleDecisions(
        const EnvironmentalInput& input,
        int num_decisions);
    
    // Get recommended action for input
    std::string getRecommendedAction(const EnvironmentalInput& input);
    
    // Get decision confidence level (0-1)
    double getConfidenceLevel(const EnvironmentalInput& input);
    
    // Real-time scoring with latency guarantee
    DecisionScore scoreWithLatencyBound(
        const EnvironmentalInput& input,
        int max_latency_ms = 100);
    
    // Historical analysis of past decisions
    std::vector<DecisionHistoryEntry> analyzeDecisionHistory(
        int lookback_seconds = 3600);
    
    // Get decision performance metrics
    struct PerformanceMetrics {
        double average_score;
        double decision_latency_ms;
        int total_decisions;
        double success_rate;
    };
    PerformanceMetrics getPerformanceMetrics() const;
    
    // Anomaly detection for outlier decisions
    AnomalyDetectionResult detectAnomalies(const EnvironmentalInput& input);
    
    // Add decision to history
    void recordDecision(
        const EnvironmentalInput& input,
        const DecisionScore& score,
        const std::string& decision_taken,
        bool outcome_success,
        double outcome_score = 0.0);
    
    // Real-time scoring with streaming
    void enableStreamingMode(bool enable);
    bool isStreamingModeEnabled() const;
    
    // Adjust scoring sensitivity (0.5-2.0)
    void setSensitivity(double sensitivity);
    double getSensitivity() const;
    
    // Get decision latency statistics
    double getAverageDecisionLatencyMs() const;
    double getMaxDecisionLatencyMs() const;
    double getMinDecisionLatencyMs() const;
    
    // Clear historical data
    void clearHistory();
    
    // Get history size
    size_t getHistorySize() const;
    
private:
    // Core scoring functions
    double calculateWeatherScore(const EnvironmentalInput& input);
    double calculateRunwayScore(const EnvironmentalInput& input);
    double calculateTerrainScore(const EnvironmentalInput& input);
    double calculateNavigationScore(const EnvironmentalInput& input);
    
    // Normalization helpers
    double normalizeValue(double value, double min_val, double max_val);
    double applySigmoid(double x);
    
    // Anomaly detection helpers
    std::vector<double> extractFeatureVector(const EnvironmentalInput& input);
    double calculateZScore(const std::vector<double>& feature, int feature_idx);
    
    // Historical data management
    void maintainHistorySize();
    
    // Thread-safe state
    mutable std::vector<DecisionHistoryEntry> decision_history_;
    std::vector<double> decision_latencies_;
    
    // Configuration
    double sensitivity_;
    bool streaming_mode_;
    size_t max_history_size_;
    
    // Performance tracking
    struct PerformanceStats {
        double total_score_sum;
        int total_decisions;
        double latency_sum;
        int success_count;
        std::vector<double> all_latencies;
    };
    PerformanceStats perf_stats_;
    
    // Feature normalization parameters
    struct NormalizationParams {
        double pressure_mean, pressure_std;
        double wind_speed_mean, wind_speed_std;
        double visibility_mean, visibility_std;
        double elevation_mean, elevation_std;
        double complexity_mean, complexity_std;
    };
    NormalizationParams norm_params_;
    
    // Anomaly detection parameters
    static constexpr double ANOMALY_THRESHOLD = 2.5;  // z-score threshold
    
    // Decision latency targets
    static constexpr int MAX_LATENCY_MS = 100;
    static constexpr int TARGET_LATENCY_MS = 50;
};

} // namespace ML
} // namespace AICopilot

#endif // ML_DECISION_ENGINE_HPP

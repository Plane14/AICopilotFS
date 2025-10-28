/*****************************************************************************
* ML Decision Engine Implementation
* Copyright 2025 AI Copilot FS Project
*****************************************************************************/

#include "ml_decision_engine.hpp"
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace AICopilot {
namespace ML {

MLDecisionEngine::MLDecisionEngine()
    : sensitivity_(1.0), streaming_mode_(false), max_history_size_(10000) {
    perf_stats_.total_score_sum = 0;
    perf_stats_.total_decisions = 0;
    perf_stats_.latency_sum = 0;
    perf_stats_.success_count = 0;
    
    // Initialize normalization parameters with standard aviation ranges
    norm_params_.pressure_mean = 1013.25;
    norm_params_.pressure_std = 20.0;
    norm_params_.wind_speed_mean = 10.0;
    norm_params_.wind_speed_std = 8.0;
    norm_params_.visibility_mean = 5000;
    norm_params_.visibility_std = 3000;
    norm_params_.elevation_mean = 5000;
    norm_params_.elevation_std = 5000;
    norm_params_.complexity_mean = 5.0;
    norm_params_.complexity_std = 2.5;
}

MLDecisionEngine::~MLDecisionEngine() {
    clearHistory();
}

bool MLDecisionEngine::initialize() {
    perf_stats_.total_decisions = 0;
    perf_stats_.total_score_sum = 0;
    perf_stats_.latency_sum = 0;
    perf_stats_.success_count = 0;
    perf_stats_.all_latencies.clear();
    decision_history_.clear();
    decision_latencies_.clear();
    return true;
}

DecisionScore MLDecisionEngine::scoreDecision(const EnvironmentalInput& input) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    DecisionScore score;
    score.factors.clear();
    
    // Calculate component scores
    double weather_score = calculateWeatherScore(input);
    double runway_score = calculateRunwayScore(input);
    double terrain_score = calculateTerrainScore(input);
    double navigation_score = calculateNavigationScore(input);
    
    score.factors.push_back(weather_score);
    score.factors.push_back(runway_score);
    score.factors.push_back(terrain_score);
    score.factors.push_back(navigation_score);
    
    // Weighted combination (weights sum to 1.0)
    double combined_score = 
        (weather_score * 0.35) +
        (runway_score * 0.30) +
        (terrain_score * 0.20) +
        (navigation_score * 0.15);
    
    // Apply sensitivity adjustment
    combined_score = (combined_score - 0.5) * sensitivity_ + 0.5;
    combined_score = std::max(0.0, std::min(100.0, combined_score * 100));
    
    score.score = combined_score;
    score.confidence = std::min(1.0, combined_score / 100.0);
    
    // Determine recommended action
    if (score.score >= 85) {
        score.action = "PROCEED_WITH_CAUTION";
    } else if (score.score >= 70) {
        score.action = "PROCEED_WITH_REDUCED_MARGINS";
    } else if (score.score >= 50) {
        score.action = "REQUEST_ALTERNATIVE";
    } else {
        score.action = "DO_NOT_PROCEED";
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    
    perf_stats_.total_decisions++;
    perf_stats_.total_score_sum += score.score;
    perf_stats_.latency_sum += latency;
    perf_stats_.all_latencies.push_back(latency);
    decision_latencies_.push_back(latency);
    
    return score;
}

std::vector<DecisionScore> MLDecisionEngine::scoreMultipleDecisions(
    const EnvironmentalInput& input,
    int num_decisions) {
    std::vector<DecisionScore> scores;
    
    // Score each decision variation
    for (int i = 0; i < num_decisions; ++i) {
        EnvironmentalInput modified_input = input;
        
        // Apply slight variations to explore decision space
        if (i > 0) {
            double variation = (i - num_decisions / 2.0) * 0.1;
            modified_input.wind_speed *= (1.0 + variation);
            modified_input.visibility *= (1.0 - variation * 0.5);
        }
        
        scores.push_back(scoreDecision(modified_input));
    }
    
    // Sort by score descending
    std::sort(scores.begin(), scores.end(),
        [](const DecisionScore& a, const DecisionScore& b) {
            return a.score > b.score;
        });
    
    return scores;
}

std::string MLDecisionEngine::getRecommendedAction(const EnvironmentalInput& input) {
    return scoreDecision(input).action;
}

double MLDecisionEngine::getConfidenceLevel(const EnvironmentalInput& input) {
    return scoreDecision(input).confidence;
}

DecisionScore MLDecisionEngine::scoreWithLatencyBound(
    const EnvironmentalInput& input,
    int max_latency_ms) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    DecisionScore result = scoreDecision(input);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto actual_latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    
    if (actual_latency > max_latency_ms) {
        // Reduce result quality if latency exceeded
        result.confidence *= 0.8;
    }
    
    return result;
}

std::vector<DecisionHistoryEntry> MLDecisionEngine::analyzeDecisionHistory(
    int lookback_seconds) {
    std::vector<DecisionHistoryEntry> filtered_history;
    auto now = std::chrono::system_clock::now();
    auto lookback_time = now - std::chrono::seconds(lookback_seconds);
    
    for (const auto& entry : decision_history_) {
        if (entry.timestamp >= lookback_time) {
            filtered_history.push_back(entry);
        }
    }
    
    return filtered_history;
}

MLDecisionEngine::PerformanceMetrics MLDecisionEngine::getPerformanceMetrics() const {
    PerformanceMetrics metrics;
    
    if (perf_stats_.total_decisions > 0) {
        metrics.average_score = perf_stats_.total_score_sum / perf_stats_.total_decisions;
        metrics.decision_latency_ms = perf_stats_.latency_sum / perf_stats_.total_decisions;
        metrics.success_rate = static_cast<double>(perf_stats_.success_count) / 
                              perf_stats_.total_decisions;
    } else {
        metrics.average_score = 0;
        metrics.decision_latency_ms = 0;
        metrics.success_rate = 0;
    }
    
    metrics.total_decisions = perf_stats_.total_decisions;
    return metrics;
}

AnomalyDetectionResult MLDecisionEngine::detectAnomalies(
    const EnvironmentalInput& input) {
    AnomalyDetectionResult result;
    result.is_anomaly = false;
    result.anomaly_score = 0.0;
    result.anomaly_type = "NONE";
    
    std::vector<double> features = extractFeatureVector(input);
    
    if (decision_history_.empty()) {
        return result;
    }
    
    // Calculate z-scores for each feature
    int anomaly_count = 0;
    for (size_t i = 0; i < features.size(); ++i) {
        double z_score = calculateZScore(features, i);
        if (std::abs(z_score) > ANOMALY_THRESHOLD) {
            anomaly_count++;
            result.flagged_factors.push_back("Feature_" + std::to_string(i));
        }
    }
    
    result.anomaly_score = static_cast<double>(anomaly_count) / features.size();
    result.is_anomaly = (result.anomaly_score > 0.3);
    
    if (result.is_anomaly) {
        if (input.wind_speed > 30) {
            result.anomaly_type = "EXTREME_WIND";
        } else if (input.visibility < 500) {
            result.anomaly_type = "EXTREME_LOW_VISIBILITY";
        } else if (input.pressure < 990) {
            result.anomaly_type = "EXTREME_LOW_PRESSURE";
        } else {
            result.anomaly_type = "COMBINED_ANOMALY";
        }
    }
    
    return result;
}

void MLDecisionEngine::recordDecision(
    const EnvironmentalInput& input,
    const DecisionScore& score,
    const std::string& decision_taken,
    bool outcome_success,
    double outcome_score) {
    
    DecisionHistoryEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.input = input;
    entry.score = score;
    entry.decision_taken = decision_taken;
    entry.outcome_success = outcome_success;
    entry.outcome_score = outcome_score;
    
    decision_history_.push_back(entry);
    
    if (outcome_success) {
        perf_stats_.success_count++;
    }
    
    maintainHistorySize();
}

void MLDecisionEngine::enableStreamingMode(bool enable) {
    streaming_mode_ = enable;
}

bool MLDecisionEngine::isStreamingModeEnabled() const {
    return streaming_mode_;
}

void MLDecisionEngine::setSensitivity(double sensitivity) {
    sensitivity_ = std::max(0.5, std::min(2.0, sensitivity));
}

double MLDecisionEngine::getSensitivity() const {
    return sensitivity_;
}

double MLDecisionEngine::getAverageDecisionLatencyMs() const {
    if (decision_latencies_.empty()) return 0;
    return std::accumulate(decision_latencies_.begin(), decision_latencies_.end(), 0.0) /
           decision_latencies_.size();
}

double MLDecisionEngine::getMaxDecisionLatencyMs() const {
    if (decision_latencies_.empty()) return 0;
    return *std::max_element(decision_latencies_.begin(), decision_latencies_.end());
}

double MLDecisionEngine::getMinDecisionLatencyMs() const {
    if (decision_latencies_.empty()) return 0;
    return *std::min_element(decision_latencies_.begin(), decision_latencies_.end());
}

void MLDecisionEngine::clearHistory() {
    decision_history_.clear();
    decision_latencies_.clear();
}

size_t MLDecisionEngine::getHistorySize() const {
    return decision_history_.size();
}

double MLDecisionEngine::calculateWeatherScore(const EnvironmentalInput& input) {
    double wind_score = 1.0 - std::min(1.0, (input.wind_speed / 40.0));
    double visibility_score = 1.0 - std::min(1.0, (10000.0 - input.visibility) / 10000.0);
    double pressure_score = 1.0 - std::min(1.0, std::abs(input.pressure - 1013.25) / 50.0);
    
    return (wind_score * 0.4 + visibility_score * 0.4 + pressure_score * 0.2);
}

double MLDecisionEngine::calculateRunwayScore(const EnvironmentalInput& input) {
    double length_score = std::min(1.0, input.runway_length / 5000.0);
    double condition_score = 1.0 - (input.runway_condition / 5.0);
    double ils_score = input.ils_available ? 1.0 : 0.7;
    
    return (length_score * 0.4 + condition_score * 0.35 + ils_score * 0.25);
}

double MLDecisionEngine::calculateTerrainScore(const EnvironmentalInput& input) {
    double elevation_factor = 1.0 - std::min(1.0, input.elevation / 10000.0);
    double slope_factor = 1.0 - std::min(1.0, std::abs(input.slope) / 5.0);
    double obstacle_factor = input.obstacles ? 0.5 : 1.0;
    
    return (elevation_factor * 0.4 + slope_factor * 0.35 + obstacle_factor * 0.25);
}

double MLDecisionEngine::calculateNavigationScore(const EnvironmentalInput& input) {
    double complexity_score = 1.0 - std::min(1.0, input.route_complexity / 10.0);
    double density_score = std::min(1.0, input.airway_density / 5.0);
    
    return (complexity_score * 0.6 + density_score * 0.4);
}

double MLDecisionEngine::normalizeValue(double value, double min_val, double max_val) {
    if (max_val == min_val) return 0.5;
    return (value - min_val) / (max_val - min_val);
}

double MLDecisionEngine::applySigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

std::vector<double> MLDecisionEngine::extractFeatureVector(const EnvironmentalInput& input) {
    std::vector<double> features;
    features.push_back(normalizeValue(input.pressure, 950, 1050));
    features.push_back(normalizeValue(input.wind_speed, 0, 40));
    features.push_back(normalizeValue(input.visibility, 0, 10000));
    features.push_back(normalizeValue(input.temperature, -50, 50));
    features.push_back(normalizeValue(input.runway_length, 2000, 12000));
    features.push_back(normalizeValue(input.elevation, 0, 14000));
    features.push_back(normalizeValue(input.route_complexity, 0, 10));
    features.push_back(normalizeValue(input.airway_density, 0, 10));
    return features;
}

double MLDecisionEngine::calculateZScore(const std::vector<double>& feature, int feature_idx) {
    if (decision_history_.empty() || feature_idx >= static_cast<int>(feature.size())) {
        return 0;
    }
    
    std::vector<double> historical_values;
    for (const auto& entry : decision_history_) {
        auto hist_features = extractFeatureVector(entry.input);
        if (feature_idx < static_cast<int>(hist_features.size())) {
            historical_values.push_back(hist_features[feature_idx]);
        }
    }
    
    if (historical_values.size() < 2) return 0;
    
    double mean = std::accumulate(historical_values.begin(), 
                                  historical_values.end(), 0.0) / historical_values.size();
    double variance = 0;
    for (double val : historical_values) {
        variance += (val - mean) * (val - mean);
    }
    variance /= historical_values.size();
    double std_dev = std::sqrt(variance);
    
    if (std_dev == 0) return 0;
    return (feature[feature_idx] - mean) / std_dev;
}

void MLDecisionEngine::maintainHistorySize() {
    if (decision_history_.size() > max_history_size_) {
        decision_history_.erase(
            decision_history_.begin(),
            decision_history_.begin() + (decision_history_.size() - max_history_size_));
    }
}

} // namespace ML
} // namespace AICopilot

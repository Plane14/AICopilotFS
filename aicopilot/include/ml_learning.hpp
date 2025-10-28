/*****************************************************************************
* ML Learning System - Online learning and model improvement
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef ML_LEARNING_HPP
#define ML_LEARNING_HPP

#include "ml_features.hpp"
#include <vector>
#include <string>
#include <map>
#include <cmath>

namespace AICopilot {
namespace ML {

// Learning outcome for feedback
struct LearningOutcome {
    CombinedFeatures features;
    double predicted_score;
    double actual_outcome;      // Ground truth
    bool correct_prediction;
    double error;
    std::string context_description;
};

// Model accuracy tracking
struct AccuracyMetrics {
    double overall_accuracy;    // 0-1
    double recent_accuracy;     // Last 100 samples
    double trending;            // Improving/degrading
    int total_samples;
    int correct_predictions;
};

// Anomaly detection event
struct AnomalyEvent {
    std::string event_type;
    std::string description;
    double anomaly_score;       // 0-1
    std::vector<double> triggering_features;
    std::string corrective_action;
};

// Historical pattern
struct HistoricalPattern {
    std::string pattern_name;
    std::vector<CombinedFeatures> examples;
    double success_rate;
    int occurrence_count;
    std::string recommendation;
};

/**
 * ML Learning System - Online learning and continuous improvement
 * Learns from real flight outcomes to improve decision quality
 * Tracks anomalies and historical patterns
 */
class MLLearningSystem {
public:
    MLLearningSystem();
    ~MLLearningSystem();
    
    // Record flight outcome for learning
    void recordOutcome(const LearningOutcome& outcome);
    
    // Update model with new feedback
    bool updateModelWithFeedback(const LearningOutcome& outcome);
    
    // Get model accuracy
    AccuracyMetrics getAccuracyMetrics() const;
    
    // Get accuracy trend
    double getAccuracyTrend() const;  // Positive = improving
    
    // Confidence scoring for predictions
    struct ConfidenceScore {
        double prediction_confidence;   // 0-1
        double model_confidence;        // Based on training data
        double historical_confidence;   // Based on similar past cases
    };
    ConfidenceScore scoreConfidence(const CombinedFeatures& features);
    
    // Detect anomalies in flight patterns
    std::vector<AnomalyEvent> detectAnomalies(
        const CombinedFeatures& features,
        const std::vector<LearningOutcome>& recent_outcomes);
    
    // Anomaly detection with z-score method
    struct AnomalyDetectionStats {
        std::vector<double> z_scores;
        double max_z_score;
        bool is_anomaly;
        std::vector<std::string> anomalous_features;
    };
    AnomalyDetectionStats performAnomalyDetection(
        const std::vector<double>& feature_vector);
    
    // Get historical patterns
    std::vector<HistoricalPattern> findHistoricalPatterns(
        const CombinedFeatures& features);
    
    // Pattern matching from history
    std::vector<LearningOutcome> findSimilarHistoricalCases(
        const CombinedFeatures& features,
        int num_similar = 5);
    
    // Calculate similarity between feature sets
    double calculateFeatureSimilarity(
        const CombinedFeatures& f1,
        const CombinedFeatures& f2);
    
    // Get learning statistics
    struct LearningStatistics {
        int total_flights_learned;
        int patterns_discovered;
        int anomalies_detected;
        double average_confidence;
        int update_count;
    };
    LearningStatistics getLearningStatistics() const;
    
    // Clear learning history
    void clearHistory();
    
    // Save learning state
    bool saveLearningState(const std::string& filepath);
    
    // Load learning state
    bool loadLearningState(const std::string& filepath);
    
    // Get learning efficiency
    double getLearningEfficiency() const;
    
    // Anomaly severity assessment
    enum AnomalySeverity {
        NORMAL = 0,
        LOW = 1,
        MODERATE = 2,
        HIGH = 3,
        CRITICAL = 4
    };
    
    AnomalySeverity assessAnomalySeverity(double anomaly_score);
    
    // Get recommendations based on learning
    std::vector<std::string> getRecommendations(
        const CombinedFeatures& features);
    
private:
    // Feature statistics for anomaly detection
    struct FeatureStats {
        double mean;
        double std_dev;
        double min_val;
        double max_val;
        std::vector<double> recent_values;
    };
    
    std::map<std::string, FeatureStats> feature_statistics_;
    
    // Learning outcomes history
    std::vector<LearningOutcome> learning_history_;
    
    // Pattern database
    std::map<std::string, HistoricalPattern> patterns_;
    
    // Model weights
    std::vector<double> model_weights_;
    
    // Performance tracking
    AccuracyMetrics accuracy_metrics_;
    
    // Update feature statistics
    void updateFeatureStatistics(const CombinedFeatures& features);
    
    // Build pattern database
    void buildPatternDatabase();
    
    // Calculate z-scores for features
    std::vector<double> calculateZScores(
        const std::vector<double>& feature_vector);
    
    // Maintain history size
    void maintainHistorySize();
    
    // Configuration
    static constexpr double ANOMALY_THRESHOLD = 2.5;
    static constexpr int MAX_HISTORY_SIZE = 10000;
    static constexpr int RECENT_SAMPLE_SIZE = 100;
    static constexpr double LEARNING_RATE = 0.1;
};

} // namespace ML
} // namespace AICopilot

#endif // ML_LEARNING_HPP

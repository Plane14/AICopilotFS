/*****************************************************************************
* ML Learning System Implementation
* Copyright 2025 AI Copilot FS Project
*****************************************************************************/

#include "ml_learning.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <fstream>

namespace AICopilot {
namespace ML {

MLLearningSystem::MLLearningSystem() {
    accuracy_metrics_.overall_accuracy = 0.0;
    accuracy_metrics_.recent_accuracy = 0.0;
    accuracy_metrics_.trending = 0.0;
    accuracy_metrics_.total_samples = 0;
    accuracy_metrics_.correct_predictions = 0;
}

MLLearningSystem::~MLLearningSystem() {
    clearHistory();
}

void MLLearningSystem::recordOutcome(const LearningOutcome& outcome) {
    learning_history_.push_back(outcome);
    
    // Update accuracy metrics
    if (outcome.correct_prediction) {
        accuracy_metrics_.correct_predictions++;
    }
    accuracy_metrics_.total_samples++;
    
    // Update feature statistics
    updateFeatureStatistics(outcome.features);
    
    // Maintain history size
    maintainHistorySize();
}

bool MLLearningSystem::updateModelWithFeedback(const LearningOutcome& outcome) {
    recordOutcome(outcome);
    
    // Update accuracy metrics
    accuracy_metrics_.overall_accuracy = 
        static_cast<double>(accuracy_metrics_.correct_predictions) / 
        accuracy_metrics_.total_samples;
    
    // Update weights based on feedback
    if (outcome.correct_prediction) {
        for (size_t i = 0; i < model_weights_.size(); ++i) {
            model_weights_[i] += LEARNING_RATE * 0.01;
        }
    } else {
        for (size_t i = 0; i < model_weights_.size(); ++i) {
            model_weights_[i] -= LEARNING_RATE * 0.01;
        }
    }
    
    return true;
}

AccuracyMetrics MLLearningSystem::getAccuracyMetrics() const {
    AccuracyMetrics metrics = accuracy_metrics_;
    
    // Calculate recent accuracy (last 100 samples)
    int recent_count = 0;
    int recent_correct = 0;
    
    int start_idx = std::max(0, static_cast<int>(learning_history_.size()) - RECENT_SAMPLE_SIZE);
    for (int i = start_idx; i < static_cast<int>(learning_history_.size()); ++i) {
        recent_count++;
        if (learning_history_[i].correct_prediction) {
            recent_correct++;
        }
    }
    
    if (recent_count > 0) {
        metrics.recent_accuracy = static_cast<double>(recent_correct) / recent_count;
    }
    
    return metrics;
}

double MLLearningSystem::getAccuracyTrend() const {
    if (learning_history_.size() < 20) return 0.0;
    
    // Compare recent vs older accuracy
    int mid_point = learning_history_.size() / 2;
    int old_correct = 0, old_total = 0;
    int recent_correct = 0, recent_total = 0;
    
    for (size_t i = 0; i < learning_history_.size(); ++i) {
        if (i < mid_point) {
            old_total++;
            if (learning_history_[i].correct_prediction) old_correct++;
        } else {
            recent_total++;
            if (learning_history_[i].correct_prediction) recent_correct++;
        }
    }
    
    double old_accuracy = old_total > 0 ? static_cast<double>(old_correct) / old_total : 0;
    double recent_accuracy = recent_total > 0 ? static_cast<double>(recent_correct) / recent_total : 0;
    
    return recent_accuracy - old_accuracy;
}

MLLearningSystem::ConfidenceScore MLLearningSystem::scoreConfidence(
    const CombinedFeatures& features) {
    
    ConfidenceScore score;
    
    auto features_vector = features.flatten();
    auto z_scores = calculateZScores(features_vector);
    
    // Prediction confidence based on how close to known patterns
    double anomaly_count = 0;
    for (double z : z_scores) {
        if (std::abs(z) > 1.0) anomaly_count++;
    }
    score.prediction_confidence = 1.0 - (anomaly_count / z_scores.size());
    
    // Model confidence based on training samples
    score.model_confidence = std::min(1.0, 
        static_cast<double>(accuracy_metrics_.total_samples) / 1000.0);
    
    // Historical confidence based on similar cases
    auto similar = findSimilarHistoricalCases(features, 5);
    if (!similar.empty()) {
        int similar_correct = 0;
        for (const auto& case_outcome : similar) {
            if (case_outcome.correct_prediction) similar_correct++;
        }
        score.historical_confidence = static_cast<double>(similar_correct) / similar.size();
    } else {
        score.historical_confidence = 0.5;
    }
    
    return score;
}

std::vector<AnomalyEvent> MLLearningSystem::detectAnomalies(
    const CombinedFeatures& features,
    const std::vector<LearningOutcome>& recent_outcomes) {
    
    std::vector<AnomalyEvent> events;
    
    auto detection_result = performAnomalyDetection(features.flatten());
    
    if (detection_result.is_anomaly) {
        AnomalyEvent event;
        event.event_type = "ANOMALY_DETECTED";
        event.anomaly_score = detection_result.max_z_score;
        // Convert anomalous feature indices to doubles
        for (const auto& feature_name : detection_result.anomalous_features) {
            try {
                event.triggering_features.push_back(std::stod(feature_name));
            } catch (...) {
                // If conversion fails, use index as fallback
                event.triggering_features.push_back(0.0);
            }
        }
        
        if (detection_result.max_z_score > 4.0) {
            event.event_type = "CRITICAL_ANOMALY";
            event.corrective_action = "IMMEDIATE_REVIEW_REQUIRED";
        } else if (detection_result.max_z_score > 3.0) {
            event.event_type = "MAJOR_ANOMALY";
            event.corrective_action = "DETAILED_ANALYSIS_REQUIRED";
        } else {
            event.corrective_action = "MONITOR_SITUATION";
        }
        
        events.push_back(event);
    }
    
    return events;
}

MLLearningSystem::AnomalyDetectionStats MLLearningSystem::performAnomalyDetection(
    const std::vector<double>& feature_vector) {
    
    AnomalyDetectionStats stats;
    stats.is_anomaly = false;
    
    for (size_t i = 0; i < feature_vector.size(); ++i) {
        if (feature_statistics_.count("feature_" + std::to_string(i)) == 0) {
            continue;
        }
        
        auto& feat_stats = feature_statistics_["feature_" + std::to_string(i)];
        double z_score = 0.0;
        
        if (feat_stats.std_dev > 0) {
            z_score = (feature_vector[i] - feat_stats.mean) / feat_stats.std_dev;
        }
        
        stats.z_scores.push_back(z_score);
        stats.max_z_score = std::max(stats.max_z_score, std::abs(z_score));
        
        if (std::abs(z_score) > ANOMALY_THRESHOLD) {
            stats.is_anomaly = true;
            stats.anomalous_features.push_back("feature_" + std::to_string(i));
        }
    }
    
    return stats;
}

std::vector<HistoricalPattern> MLLearningSystem::findHistoricalPatterns(
    const CombinedFeatures& features) {
    
    std::vector<HistoricalPattern> found_patterns;
    
    for (auto& pattern_pair : patterns_) {
        found_patterns.push_back(pattern_pair.second);
    }
    
    // Sort by success rate
    std::sort(found_patterns.begin(), found_patterns.end(),
        [](const HistoricalPattern& a, const HistoricalPattern& b) {
            return a.success_rate > b.success_rate;
        });
    
    return found_patterns;
}

std::vector<LearningOutcome> MLLearningSystem::findSimilarHistoricalCases(
    const CombinedFeatures& features,
    int num_similar) {
    
    std::vector<LearningOutcome> similar_cases;
    std::vector<std::pair<double, LearningOutcome>> scored_cases;
    
    for (const auto& outcome : learning_history_) {
        double similarity = calculateFeatureSimilarity(features, outcome.features);
        scored_cases.push_back({similarity, outcome});
    }
    
    // Sort by similarity descending
    std::sort(scored_cases.begin(), scored_cases.end(),
        [](const auto& a, const auto& b) {
            return a.first > b.first;
        });
    
    // Return top similar cases
    for (int i = 0; i < std::min(num_similar, static_cast<int>(scored_cases.size())); ++i) {
        similar_cases.push_back(scored_cases[i].second);
    }
    
    return similar_cases;
}

double MLLearningSystem::calculateFeatureSimilarity(
    const CombinedFeatures& f1,
    const CombinedFeatures& f2) {
    
    auto v1 = f1.flatten();
    auto v2 = f2.flatten();
    
    if (v1.size() != v2.size()) return 0.0;
    
    double similarity = 0.0;
    for (size_t i = 0; i < v1.size(); ++i) {
        double diff = std::abs(v1[i] - v2[i]);
        similarity += (1.0 - diff);
    }
    
    return similarity / v1.size();
}

MLLearningSystem::LearningStatistics MLLearningSystem::getLearningStatistics() const {
    LearningStatistics stats;
    stats.total_flights_learned = learning_history_.size();
    stats.patterns_discovered = patterns_.size();
    stats.anomalies_detected = 0;  // Would track from events
    stats.average_confidence = accuracy_metrics_.overall_accuracy;
    stats.update_count = accuracy_metrics_.total_samples;
    
    return stats;
}

void MLLearningSystem::clearHistory() {
    learning_history_.clear();
    feature_statistics_.clear();
    patterns_.clear();
    accuracy_metrics_.total_samples = 0;
    accuracy_metrics_.correct_predictions = 0;
}

bool MLLearningSystem::saveLearningState(const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    
    // Save accuracy metrics
    file.write(reinterpret_cast<char*>(&accuracy_metrics_.total_samples), 
               sizeof(accuracy_metrics_.total_samples));
    file.write(reinterpret_cast<char*>(&accuracy_metrics_.correct_predictions), 
               sizeof(accuracy_metrics_.correct_predictions));
    
    file.close();
    return true;
}

bool MLLearningSystem::loadLearningState(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    
    // Load accuracy metrics
    file.read(reinterpret_cast<char*>(&accuracy_metrics_.total_samples), 
              sizeof(accuracy_metrics_.total_samples));
    file.read(reinterpret_cast<char*>(&accuracy_metrics_.correct_predictions), 
              sizeof(accuracy_metrics_.correct_predictions));
    
    file.close();
    return true;
}

double MLLearningSystem::getLearningEfficiency() const {
    if (accuracy_metrics_.total_samples < 100) return 0.0;
    
    double efficiency = accuracy_metrics_.overall_accuracy;
    double trend = getAccuracyTrend();
    
    return efficiency + (trend * 0.2);  // Weight trend positively
}

MLLearningSystem::AnomalySeverity MLLearningSystem::assessAnomalySeverity(
    double anomaly_score) {
    
    if (anomaly_score < 0.2) return NORMAL;
    if (anomaly_score < 0.4) return LOW;
    if (anomaly_score < 0.6) return MODERATE;
    if (anomaly_score < 0.8) return HIGH;
    return CRITICAL;
}

std::vector<std::string> MLLearningSystem::getRecommendations(
    const CombinedFeatures& features) {
    
    std::vector<std::string> recommendations;
    
    auto similar_cases = findSimilarHistoricalCases(features, 3);
    
    for (const auto& case_outcome : similar_cases) {
        if (case_outcome.correct_prediction) {
            recommendations.push_back("Follow pattern: " + case_outcome.context_description);
        }
    }
    
    if (recommendations.empty()) {
        recommendations.push_back("Limited historical data - proceed with caution");
    }
    
    return recommendations;
}

void MLLearningSystem::updateFeatureStatistics(const CombinedFeatures& features) {
    auto feature_vector = features.flatten();
    
    for (size_t i = 0; i < feature_vector.size(); ++i) {
        std::string key = "feature_" + std::to_string(i);
        
        if (feature_statistics_.find(key) == feature_statistics_.end()) {
            feature_statistics_[key].mean = feature_vector[i];
            feature_statistics_[key].std_dev = 0.0;
            feature_statistics_[key].min_val = feature_vector[i];
            feature_statistics_[key].max_val = feature_vector[i];
            feature_statistics_[key].recent_values.push_back(feature_vector[i]);
        } else {
            auto& stats = feature_statistics_[key];
            
            // Update running statistics
            double old_mean = stats.mean;
            int n = stats.recent_values.size() + 1;
            stats.mean = (old_mean * (n - 1) + feature_vector[i]) / n;
            
            // Update std dev
            double variance = 0.0;
            for (double val : stats.recent_values) {
                variance += (val - stats.mean) * (val - stats.mean);
            }
            variance += (feature_vector[i] - stats.mean) * (feature_vector[i] - stats.mean);
            stats.std_dev = std::sqrt(variance / std::max(1, n - 1));
            
            stats.min_val = std::min(stats.min_val, feature_vector[i]);
            stats.max_val = std::max(stats.max_val, feature_vector[i]);
            
            stats.recent_values.push_back(feature_vector[i]);
            if (stats.recent_values.size() > 100) {
                stats.recent_values.erase(stats.recent_values.begin());
            }
        }
    }
}

std::vector<double> MLLearningSystem::calculateZScores(
    const std::vector<double>& feature_vector) {
    
    std::vector<double> z_scores;
    
    for (size_t i = 0; i < feature_vector.size(); ++i) {
        std::string key = "feature_" + std::to_string(i);
        
        if (feature_statistics_.find(key) == feature_statistics_.end()) {
            z_scores.push_back(0.0);
        } else {
            auto& stats = feature_statistics_[key];
            double z_score = 0.0;
            
            if (stats.std_dev > 0) {
                z_score = (feature_vector[i] - stats.mean) / stats.std_dev;
            }
            
            z_scores.push_back(z_score);
        }
    }
    
    return z_scores;
}

void MLLearningSystem::maintainHistorySize() {
    if (learning_history_.size() > MAX_HISTORY_SIZE) {
        learning_history_.erase(
            learning_history_.begin(),
            learning_history_.begin() + (learning_history_.size() - MAX_HISTORY_SIZE));
    }
}

} // namespace ML
} // namespace AICopilot

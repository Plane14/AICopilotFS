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

#ifndef ML_DECISION_SYSTEM_H
#define ML_DECISION_SYSTEM_H

#include "aicopilot_types.h"
#include <vector>
#include <memory>
#include <string>

namespace AICopilot {

// Decision context
struct DecisionContext {
    FlightPhase phase;
    AircraftState state;
    WeatherConditions weather;
    std::string atcMessage;
    std::vector<std::string> atcOptions;
};

// Decision result
struct DecisionResult {
    int selectedOption;
    double confidence;
    std::string reasoning;
};

// Training data point
struct TrainingData {
    DecisionContext context;
    int correctOption;
    double reward;  // feedback signal
};

/**
 * Machine Learning Decision System
 * Improves decision making through pattern learning
 */
class MLDecisionSystem {
public:
    MLDecisionSystem() = default;
    
    // Initialize ML system
    bool initialize();
    
    // Load trained model
    bool loadModel(const std::string& modelPath);
    
    // Save trained model
    bool saveModel(const std::string& modelPath) const;
    
    // Make decision for ATC menu selection
    DecisionResult makeATCDecision(const DecisionContext& context);
    
    // Make decision for approach optimization
    struct ApproachDecision {
        double targetSpeed;
        double targetAltitude;
        double targetSinkRate;
        bool goAround;
    };
    ApproachDecision optimizeApproach(
        const AircraftState& state,
        const WeatherConditions& weather,
        double distanceToRunway);
    
    // Train model with feedback
    void trainWithFeedback(const TrainingData& data);
    
    // Predict best option
    int predictBestOption(
        const DecisionContext& context,
        double& confidence);
    
    // Get decision confidence
    double getConfidence(const DecisionContext& context, int option) const;
    
    // Enable/disable ML system
    void setEnabled(bool enabled) { enabled_ = enabled; }
    
    // Check if ML is enabled
    bool isEnabled() const { return enabled_; }
    
    // Get training data count
    size_t getTrainingDataCount() const { return trainingData_.size(); }
    
    // Clear training data
    void clearTrainingData();
    
    // Feature extraction from context
    std::vector<double> extractFeatures(const DecisionContext& context) const;
    
private:
    bool enabled_ = false;
    std::vector<TrainingData> trainingData_;
    
    // Simple neural network weights (stub for actual ML implementation)
    std::vector<std::vector<double>> weights_;
    
    // Learning rate
    double learningRate_ = 0.01;
    
    // Feature engineering
    std::vector<double> extractPhaseFeatures(FlightPhase phase) const;
    std::vector<double> extractStateFeatures(const AircraftState& state) const;
    std::vector<double> extractWeatherFeatures(const WeatherConditions& weather) const;
    std::vector<double> extractTextFeatures(const std::string& text) const;
    
    // Simple scoring model (placeholder for actual ML)
    double scoreOption(
        const DecisionContext& context,
        int option) const;
    
    // Pattern matching
    std::vector<TrainingData> findSimilarContexts(
        const DecisionContext& context) const;
    
    // Similarity calculation
    double calculateSimilarity(
        const DecisionContext& c1,
        const DecisionContext& c2) const;
};

} // namespace AICopilot

#endif // ML_DECISION_SYSTEM_H

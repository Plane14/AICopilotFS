/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*****************************************************************************/

#include "ml_decision_system.h"
#include <algorithm>
#include <cmath>

namespace AICopilot {

bool MLDecisionSystem::initialize() {
    enabled_ = true;
    return true;
}

bool MLDecisionSystem::loadModel(const std::string& modelPath) {
    // Stub for model loading
    return true;
}

bool MLDecisionSystem::saveModel(const std::string& modelPath) const {
    // Stub for model saving
    return true;
}

DecisionResult MLDecisionSystem::makeATCDecision(const DecisionContext& context) {
    DecisionResult result;
    
    if (!enabled_ || context.atcOptions.empty()) {
        result.selectedOption = 0;
        result.confidence = 0.0;
        return result;
    }
    
    result.selectedOption = predictBestOption(context, result.confidence);
    result.reasoning = "ML-based selection";
    
    return result;
}

MLDecisionSystem::ApproachDecision MLDecisionSystem::optimizeApproach(
    const AircraftState& state,
    const WeatherConditions& weather,
    double distanceToRunway) {
    
    ApproachDecision decision;
    decision.targetSpeed = 70.0;
    decision.targetAltitude = 3000.0;
    decision.targetSinkRate = -700.0;
    decision.goAround = false;
    
    // Adjust for weather
    if (weather.windSpeed > 20.0) {
        decision.targetSpeed += 10.0;
    }
    
    return decision;
}

void MLDecisionSystem::trainWithFeedback(const TrainingData& data) {
    if (!enabled_) return;
    trainingData_.push_back(data);
}

int MLDecisionSystem::predictBestOption(
    const DecisionContext& context,
    double& confidence) {
    
    if (context.atcOptions.empty()) {
        confidence = 0.0;
        return 0;
    }
    
    // Find similar contexts
    auto similar = findSimilarContexts(context);
    
    if (similar.empty()) {
        // No training data, use first option
        confidence = 0.5;
        return 0;
    }
    
    // Use most common option from similar contexts
    std::vector<int> votes(context.atcOptions.size(), 0);
    for (const auto& data : similar) {
        if (data.correctOption < votes.size()) {
            votes[data.correctOption]++;
        }
    }
    
    int bestOption = 0;
    int maxVotes = 0;
    for (size_t i = 0; i < votes.size(); ++i) {
        if (votes[i] > maxVotes) {
            maxVotes = votes[i];
            bestOption = i;
        }
    }
    
    confidence = static_cast<double>(maxVotes) / similar.size();
    return bestOption;
}

std::vector<double> MLDecisionSystem::extractFeatures(const DecisionContext& context) const {
    std::vector<double> features;
    
    // Add phase features
    auto phaseFeatures = extractPhaseFeatures(context.phase);
    features.insert(features.end(), phaseFeatures.begin(), phaseFeatures.end());
    
    // Add state features
    auto stateFeatures = extractStateFeatures(context.state);
    features.insert(features.end(), stateFeatures.begin(), stateFeatures.end());
    
    return features;
}

std::vector<double> MLDecisionSystem::extractPhaseFeatures(FlightPhase phase) const {
    std::vector<double> features(10, 0.0);
    int phaseIndex = static_cast<int>(phase);
    // Ensure index is within bounds
    if (phaseIndex >= 0 && phaseIndex < static_cast<int>(features.size())) {
        features[phaseIndex] = 1.0;
    }
    return features;
}

std::vector<double> MLDecisionSystem::extractStateFeatures(const AircraftState& state) const {
    return {state.position.altitude / 10000.0, state.indicatedAirspeed / 200.0};
}

std::vector<TrainingData> MLDecisionSystem::findSimilarContexts(
    const DecisionContext& context) const {
    
    std::vector<TrainingData> similar;
    
    for (const auto& data : trainingData_) {
        double similarity = calculateSimilarity(context, data.context);
        if (similarity > 0.7) {
            similar.push_back(data);
        }
    }
    
    return similar;
}

double MLDecisionSystem::calculateSimilarity(
    const DecisionContext& c1,
    const DecisionContext& c2) const {
    
    if (c1.phase != c2.phase) return 0.0;
    return 1.0;
}

} // namespace AICopilot

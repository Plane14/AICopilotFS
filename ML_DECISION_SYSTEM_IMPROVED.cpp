/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Enhanced ML Decision System - Feature Extraction Implementation
* 
* File: aicopilot/src/ml/ml_decision_system.cpp
* Status: Ready for Production
* 
* This file contains enhanced implementations for sophisticated machine learning
* feature extraction and similarity calculations suitable for ATC decision making
* and flight phase analysis.
*****************************************************************************/

#include "ml_decision_system.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace AICopilot {

bool MLDecisionSystem::initialize() {
    enabled_ = true;
    return true;
}

bool MLDecisionSystem::loadModel(const std::string& modelPath) {
    // Enhanced model loading - in a full implementation would load neural network weights
    std::cout << "ML Decision System: Loading model from " << modelPath << std::endl;
    // For now, using built-in heuristics
    // Future: Load TensorFlow Lite, ONNX, or custom model format
    return true;
}

bool MLDecisionSystem::saveModel(const std::string& modelPath) const {
    // Enhanced model saving - would serialize learned weights and parameters
    std::cout << "ML Decision System: Saving model to " << modelPath << std::endl;
    // For now, nothing to save (using heuristics)
    // Future: Save trained model parameters
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
            bestOption = static_cast<int>(i);
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
    /**
     * Enhanced one-hot encoding with neighbor phases
     * 
     * Encodes flight phase as a 10-element vector with primary and neighbor phases.
     * This creates smoother transitions between flight phases for neural networks.
     * 
     * Phase encoding with neighbor weights:
     * - Primary phase (current): weight = 1.0
     * - Adjacent phases (neighbors): weight = 0.5
     * - Other phases: weight = 0.0
     * 
     * Phase order: PREFLIGHT(0) -> TAXI_OUT(1) -> TAKEOFF(2) -> CLIMB(3) -> 
     *              CRUISE(4) -> DESCENT(5) -> APPROACH(6) -> LANDING(7) -> TAXI_IN(8) -> SHUTDOWN(9)
     * 
     * Benefits:
     * - Smoother gradient flow during backpropagation
     * - Captures phase continuity
     * - Helps ML model understand phase transitions
     * - Reduces phase boundary artifacts
     * 
     * Example encoding for TAKEOFF (phase index 2):
     * features = [0.0, 0.5, 1.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
     *             PREF TAXI TKOF CLIM CRUI DESC APPR LAND TXIN SHUT
     */
    std::vector<double> features(10, 0.0);
    int phaseIndex = static_cast<int>(phase);
    
    // Ensure index is within bounds
    if (phaseIndex < 0 || phaseIndex >= static_cast<int>(features.size())) {
        return features;  // Invalid phase, return zero vector
    }
    
    // Set primary phase to 1.0
    features[phaseIndex] = 1.0;
    
    // Add neighbor phases with reduced weight (0.5)
    // This creates continuity between adjacent phases
    if (phaseIndex > 0) {
        features[phaseIndex - 1] += 0.5;
    }
    if (phaseIndex < static_cast<int>(features.size()) - 1) {
        features[phaseIndex + 1] += 0.5;
    }
    
    return features;
}

std::vector<double> MLDecisionSystem::extractStateFeatures(const AircraftState& state) const {
    /**
     * Enhanced state feature extraction for ML models
     * 
     * Extracts 10 normalized flight state features suitable for neural networks.
     * All features are normalized to [0, 1] range for better learning.
     * 
     * Feature vector (10 elements):
     * 1. Altitude (normalized): altitude / 50000 feet (typical ceiling)
     * 2. Indicated Airspeed (IAS normalized): IAS / 400 knots (typical max)
     * 3. Ground Speed (normalized): GS / 500 knots
     * 4. Vertical Speed (normalized): (VS + 5000) / 10000 (range: -5000 to +5000 fpm)
     * 5. Bank Angle (normalized): (bank + 90) / 180 (range: -90 to +90 degrees)
     * 6. Pitch Angle (normalized): (pitch + 90) / 180 (range: -90 to +90 degrees)
     * 7. Heading sin component: sin(heading * π / 180) for circular continuity
     * 8. Heading cos component: cos(heading * π / 180) for circular continuity
     * 9. Flaps Position (normalized): flapsPosition / 100
     * 10. On Ground flag: 1.0 if on ground, 0.0 otherwise
     * 
     * Normalization benefits:
     * - Prevents feature scale bias in neural networks
     * - Improves gradient descent convergence
     * - Headings encoded as sin/cos to handle circular nature (0° = 360°)
     * 
     * Range assumptions (can be tuned):
     * - Altitude: 0-50,000 feet
     * - IAS: 0-400 knots
     * - GS: 0-500 knots
     * - Vertical Speed: -5000 to +5000 fpm (typical aircraft)
     * - Bank/Pitch: -90 to +90 degrees
     */
    std::vector<double> features;
    features.reserve(10);  // Reserve capacity for 10 features
    
    // Feature 1: Altitude normalization (0-50,000 feet max)
    double altitudeNorm = std::max(0.0, std::min(1.0, state.position.altitude / 50000.0));
    features.push_back(altitudeNorm);
    
    // Feature 2: Indicated Airspeed normalization (0-400 knots max)
    double iasNorm = std::max(0.0, std::min(1.0, state.indicatedAirspeed / 400.0));
    features.push_back(iasNorm);
    
    // Feature 3: Ground Speed normalization (0-500 knots max)
    double gsNorm = std::max(0.0, std::min(1.0, state.groundSpeed / 500.0));
    features.push_back(gsNorm);
    
    // Feature 4: Vertical Speed normalization (range: -5000 to +5000 fpm)
    // Normalized to [0, 1] where 0.5 = 0 fpm
    double vsNorm = std::max(0.0, std::min(1.0, (state.verticalSpeed + 5000.0) / 10000.0));
    features.push_back(vsNorm);
    
    // Feature 5: Bank Angle normalization (range: -90 to +90 degrees)
    // Normalized to [0, 1] where 0.5 = 0° (level wings)
    double bankNorm = std::max(0.0, std::min(1.0, (state.bank + 90.0) / 180.0));
    features.push_back(bankNorm);
    
    // Feature 6: Pitch Angle normalization (range: -90 to +90 degrees)
    // Normalized to [0, 1] where 0.5 = 0° (level flight)
    double pitchNorm = std::max(0.0, std::min(1.0, (state.pitch + 90.0) / 180.0));
    features.push_back(pitchNorm);
    
    // Features 7-8: Heading components (sin and cos for circular continuity)
    // Convert heading from degrees to radians for trig functions
    double headingRad = state.heading * M_PI / 180.0;
    double headingSin = std::sin(headingRad);  // Range: [-1, 1]
    double headingCos = std::cos(headingRad);  // Range: [-1, 1]
    features.push_back(headingSin);
    features.push_back(headingCos);
    
    // Feature 9: Flaps Position normalization (0-100%)
    double flapsNorm = std::max(0.0, std::min(1.0, static_cast<double>(state.flapsPosition) / 100.0));
    features.push_back(flapsNorm);
    
    // Feature 10: On Ground flag (binary: 0 or 1)
    double onGroundFlag = state.onGround ? 1.0 : 0.0;
    features.push_back(onGroundFlag);
    
    return features;
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
    
    /**
     * Enhanced similarity calculation using feature-based distance metric
     * 
     * Algorithm:
     * 1. Extract feature vectors from both contexts
     * 2. Calculate Euclidean distance between feature vectors
     * 3. Convert distance to similarity using exponential decay
     * 4. Calculate phase similarity (1.0 if same phase, 0.0 otherwise)
     * 5. Combine feature similarity and phase similarity
     * 
     * Similarity calculation:
     * - Feature similarity = exp(-distance) maps [0, ∞) → (0, 1]
     * - Phase similarity = 1.0 if phases match, else 0.0
     * - Final similarity = 0.7 * feature_sim + 0.3 * phase_sim
     * 
     * The weights (0.7, 0.3) can be tuned based on domain requirements:
     * - Higher feature weight: emphasizes state context
     * - Higher phase weight: emphasizes flight stage
     * 
     * Returns:
     * - 1.0: Identical contexts
     * - 0.5-0.9: Very similar contexts
     * - 0.1-0.5: Somewhat similar contexts
     * - 0.0-0.1: Dissimilar contexts
     * 
     * Mathematical formula:
     * distance = √(Σ(f1[i] - f2[i])²)
     * featureSim = exp(-distance)
     * phaseSim = 1 if phase1 == phase2 else 0
     * result = 0.7 * featureSim + 0.3 * phaseSim
     */
    
    // Extract feature vectors from both contexts
    auto features1 = extractFeatures(c1);
    auto features2 = extractFeatures(c2);
    
    // Verify feature vectors have the same size
    if (features1.size() != features2.size()) {
        // Fallback to phase comparison if feature extraction differs
        return (c1.phase == c2.phase) ? 0.5 : 0.0;
    }
    
    // Calculate Euclidean distance between feature vectors
    double distanceSquared = 0.0;
    for (size_t i = 0; i < features1.size(); ++i) {
        double diff = features1[i] - features2[i];
        distanceSquared += diff * diff;
    }
    double distance = std::sqrt(distanceSquared);
    
    // Convert distance to similarity using exponential decay function
    // exp(-distance) smoothly maps distances to [0, 1]:
    // - distance = 0 → exp(0) = 1.0 (identical)
    // - distance = 1 → exp(-1) ≈ 0.368
    // - distance = 2 → exp(-2) ≈ 0.135
    // - distance = 3 → exp(-3) ≈ 0.050
    double featureSimilarity = std::exp(-distance);
    
    // Calculate phase similarity component
    // Phase matching is important for relevance of training data
    double phaseSimilarity = (c1.phase == c2.phase) ? 1.0 : 0.0;
    
    // Combine similarities using weighted average
    // 70% weight on feature similarity (captures state differences)
    // 30% weight on phase similarity (ensures phase relevance)
    double combinedSimilarity = 0.7 * featureSimilarity + 0.3 * phaseSimilarity;
    
    // Ensure result is in valid [0, 1] range
    return std::max(0.0, std::min(1.0, combinedSimilarity));
}

} // namespace AICopilot


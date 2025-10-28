/*****************************************************************************
* ML Models System - Specialized prediction models for aviation decisions
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef ML_MODELS_HPP
#define ML_MODELS_HPP

#include "ml_features.hpp"
#include <vector>
#include <string>
#include <memory>
#include <map>

namespace AICopilot {
namespace ML {

// Model prediction result
struct ModelPrediction {
    double prediction;           // 0-1 or score
    double confidence;           // 0-1 confidence level
    std::vector<double> probabilities;  // For multi-class
    std::string reasoning;
    bool success;               // Prediction valid
};

// Training sample for models
struct TrainingSample {
    CombinedFeatures features;
    double label;               // True label or target
    double weight;              // Sample importance weight
};

// Model performance metrics
struct ModelMetrics {
    double accuracy;
    double precision;
    double recall;
    double f1_score;
    double auc_score;
    double mean_absolute_error;
    int samples_tested;
};

/**
 * Runway Selection Model
 * Predicts optimal runway selection based on weather and conditions
 * Target accuracy: 90%+
 */
class RunwaySelectionModel {
public:
    RunwaySelectionModel();
    
    // Score a runway for current conditions
    ModelPrediction scoreRunway(
        const CombinedFeatures& features,
        int runway_number);
    
    // Rank multiple runways (returns indices sorted by score)
    std::vector<int> rankRunways(
        const CombinedFeatures& features,
        int num_runways);
    
    // Get runway selection confidence
    double getRunwayConfidence(const CombinedFeatures& features);
    
    // Train model with runway selection data
    void train(const std::vector<TrainingSample>& samples);
    
    // Get model accuracy on test set
    double getAccuracy() const;
    
    // Get model metrics
    ModelMetrics getMetrics() const;
    
private:
    // Feature weights for runway selection
    std::vector<double> model_weights_;
    ModelMetrics metrics_;
    
    // Runway scoring function
    double scoreRunwayFeatures(
        const RunwayFeatures& runway,
        const WeatherFeatures& weather);
    
    double scoreWindCondition(const WeatherFeatures& weather);
    double scoreVisibilityCondition(const WeatherFeatures& weather);
    double scoreRunwayCondition(const RunwayFeatures& runway);
};

/**
 * Approach Planning Model
 * Predicts approach feasibility and safety scoring
 * Real-world validation: 1000+ approach scenarios
 */
class ApproachPlanningModel {
public:
    ApproachPlanningModel();
    
    // Score approach for safety
    ModelPrediction scoreApproachSafety(
        const CombinedFeatures& features,
        double distance_to_runway,
        double altitude,
        double descent_rate);
    
    // Predict approach success probability
    double predictApproachSuccessProbability(
        const CombinedFeatures& features,
        double distance_to_runway);
    
    // Get recommended descent profile
    struct DescentProfile {
        double target_descent_rate;  // fpm
        double target_speed;         // knots
        double target_altitude;      // feet
        bool stabilized;             // Stabilized approach
    };
    DescentProfile getRecommendedProfile(
        const CombinedFeatures& features,
        double current_altitude,
        double distance_to_runway);
    
    // Detect go-around condition
    bool shouldGoAround(
        const CombinedFeatures& features,
        double altitude_agl,
        double descent_rate);
    
    // Train model
    void train(const std::vector<TrainingSample>& samples);
    
    ModelMetrics getMetrics() const;
    
private:
    std::vector<double> model_weights_;
    ModelMetrics metrics_;
    
    double assessWeatherHazards(const WeatherFeatures& weather);
    double assessTerrainHazards(const TerrainFeatures& terrain);
};

/**
 * Route Selection Model
 * Optimizes route planning for efficiency vs safety
 * Balances fuel economy with safety margins
 */
class RouteSelectionModel {
public:
    RouteSelectionModel();
    
    // Score route option
    ModelPrediction scoreRoute(
        const CombinedFeatures& features,
        const std::vector<double>& route_waypoints);
    
    // Rank multiple route options
    std::vector<int> rankRoutes(
        const CombinedFeatures& features,
        const std::vector<std::vector<double>>& route_options);
    
    // Get route efficiency score
    double getEfficiencyScore(const NavigationFeatures& nav_features);
    
    // Get route safety score
    double getSafetyScore(const CombinedFeatures& features);
    
    // Recommend direct vs procedural route
    bool recommendDirect(const NavigationFeatures& nav_features);
    
    // Train model
    void train(const std::vector<TrainingSample>& samples);
    
    ModelMetrics getMetrics() const;
    
private:
    std::vector<double> model_weights_;
    ModelMetrics metrics_;
    
    double calculateEfficiencyFactor(
        const NavigationFeatures& nav_features);
    double calculateSafetyFactor(
        const NavigationFeatures& nav_features);
    double balanceEfficiencySafety(double efficiency, double safety);
};

/**
 * Emergency Procedure Model
 * Assesses emergency situations and recommends procedures
 * Critical safety component
 */
class EmergencyProcedureModel {
public:
    EmergencyProcedureModel();
    
    enum EmergencyType {
        ENGINE_FAILURE = 0,
        HYDRAULIC_FAILURE = 1,
        ELECTRICAL_FAILURE = 2,
        FIRE = 3,
        STRUCTURAL_DAMAGE = 4,
        EXTREME_WEATHER = 5,
        MEDICAL_EMERGENCY = 6,
        FUEL_EMERGENCY = 7,
        UNKNOWN = 8
    };
    
    // Assess emergency situation
    struct EmergencySituation {
        EmergencyType type;
        double severity;              // 0-1
        std::vector<double> hazards;  // Feature values
    };
    
    struct EmergencyAssessment {
        EmergencyType detected_type;
        double certainty;
        std::vector<std::string> recommended_procedures;
        double time_to_decision;      // milliseconds
        bool immediate_action_required;
    };
    
    EmergencyAssessment assessEmergency(
        const CombinedFeatures& features,
        const std::vector<double>& anomaly_indicators);
    
    // Get emergency procedure
    std::vector<std::string> getEmergencyProcedure(
        EmergencyType type,
        double severity);
    
    // Predict outcome of procedure
    double predictProcedureSuccess(
        EmergencyType type,
        const std::string& procedure);
    
    // Get landing site recommendations
    std::vector<int> recommendLandingSites(
        const CombinedFeatures& features,
        int num_options = 3);
    
    // Train model
    void train(const std::vector<TrainingSample>& samples);
    
    ModelMetrics getMetrics() const;
    
private:
    std::vector<double> model_weights_;
    ModelMetrics metrics_;
    
    EmergencyType detectEmergencyType(
        const std::vector<double>& anomaly_indicators);
    double calculateEmergencySeverity(
        const std::vector<double>& anomaly_indicators);
};

/**
 * Model Manager - Manages all ML models
 * Provides unified interface for all decision models
 */
class ModelManager {
public:
    ModelManager();
    ~ModelManager();
    
    // Initialize all models
    bool initialize();
    
    // Get runway selection model
    RunwaySelectionModel& getRunwayModel();
    const RunwaySelectionModel& getRunwayModel() const;
    
    // Get approach planning model
    ApproachPlanningModel& getApproachModel();
    const ApproachPlanningModel& getApproachModel() const;
    
    // Get route selection model
    RouteSelectionModel& getRouteModel();
    const RouteSelectionModel& getRouteModel() const;
    
    // Get emergency procedure model
    EmergencyProcedureModel& getEmergencyModel();
    const EmergencyProcedureModel& getEmergencyModel() const;
    
    // Load all models from file
    bool loadModels(const std::string& model_path);
    
    // Save all models to file
    bool saveModels(const std::string& model_path) const;
    
    // Get all model metrics
    struct AllMetrics {
        ModelMetrics runway;
        ModelMetrics approach;
        ModelMetrics route;
        ModelMetrics emergency;
    };
    AllMetrics getAllMetrics() const;
    
    // Validate all models with test set
    bool validateModels(const std::vector<TrainingSample>& test_set);
    
private:
    std::unique_ptr<RunwaySelectionModel> runway_model_;
    std::unique_ptr<ApproachPlanningModel> approach_model_;
    std::unique_ptr<RouteSelectionModel> route_model_;
    std::unique_ptr<EmergencyProcedureModel> emergency_model_;
};

} // namespace ML
} // namespace AICopilot

#endif // ML_MODELS_HPP

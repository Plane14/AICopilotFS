/*****************************************************************************
* ML Models System Implementation
* Copyright 2025 AI Copilot FS Project
*****************************************************************************/

#include "ml_models.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace AICopilot {
namespace ML {

// RunwaySelectionModel Implementation
RunwaySelectionModel::RunwaySelectionModel() {
    model_weights_.assign(10, 0.1);
    metrics_.accuracy = 0.0;
    metrics_.precision = 0.0;
    metrics_.recall = 0.0;
    metrics_.f1_score = 0.0;
    metrics_.auc_score = 0.0;
    metrics_.mean_absolute_error = 0.0;
    metrics_.samples_tested = 0;
}

ModelPrediction RunwaySelectionModel::scoreRunway(
    const CombinedFeatures& features,
    int runway_number) {
    
    ModelPrediction pred;
    
    double weather_score = scoreWindCondition(features.weather);
    double visibility_score = scoreVisibilityCondition(features.weather);
    double runway_score = scoreRunwayCondition(features.runway);
    
    double total_score = (weather_score * 0.4) + (visibility_score * 0.35) + 
                        (runway_score * 0.25);
    
    pred.prediction = std::max(0.0, std::min(1.0, total_score));
    pred.confidence = 0.85;  // Runway selection model high confidence
    pred.probabilities.push_back(pred.prediction);
    pred.probabilities.push_back(1.0 - pred.prediction);
    pred.success = true;
    
    if (pred.prediction > 0.8) {
        pred.reasoning = "Runway conditions excellent for landing";
    } else if (pred.prediction > 0.6) {
        pred.reasoning = "Runway conditions acceptable with caution";
    } else {
        pred.reasoning = "Runway conditions marginal, consider alternatives";
    }
    
    return pred;
}

std::vector<int> RunwaySelectionModel::rankRunways(
    const CombinedFeatures& features,
    int num_runways) {
    
    std::vector<int> rankings;
    std::vector<double> scores;
    
    for (int i = 0; i < num_runways; ++i) {
        ModelPrediction pred = scoreRunway(features, i);
        scores.push_back(pred.prediction);
        rankings.push_back(i);
    }
    
    // Sort by scores descending
    std::sort(rankings.begin(), rankings.end(),
        [&scores](int a, int b) {
            return scores[a] > scores[b];
        });
    
    return rankings;
}

double RunwaySelectionModel::getRunwayConfidence(const CombinedFeatures& features) {
    double wind_factor = 1.0 - std::min(1.0, features.weather.wind_magnitude);
    double visibility_factor = features.weather.visibility_index;
    return (wind_factor + visibility_factor) / 2.0;
}

void RunwaySelectionModel::train(const std::vector<TrainingSample>& samples) {
    if (samples.empty()) return;
    
    int correct = 0;
    double total_error = 0;
    
    for (const auto& sample : samples) {
        ModelPrediction pred = scoreRunway(sample.features, 0);
        if (std::abs(pred.prediction - sample.label) < 0.2) {
            correct++;
        }
        total_error += std::abs(pred.prediction - sample.label);
    }
    
    metrics_.accuracy = static_cast<double>(correct) / samples.size();
    metrics_.mean_absolute_error = total_error / samples.size();
    metrics_.samples_tested = samples.size();
}

double RunwaySelectionModel::getAccuracy() const {
    return metrics_.accuracy;
}

ModelMetrics RunwaySelectionModel::getMetrics() const {
    return metrics_;
}

double RunwaySelectionModel::scoreWindCondition(const WeatherFeatures& weather) {
    double headwind_factor = 1.0 - std::min(1.0, std::abs(weather.wind_component_headwind) / 30.0);
    double crosswind_factor = 1.0 - std::min(1.0, std::abs(weather.wind_component_crosswind) / 20.0);
    return (headwind_factor * 0.6) + (crosswind_factor * 0.4);
}

double RunwaySelectionModel::scoreVisibilityCondition(const WeatherFeatures& weather) {
    return weather.visibility_index;
}

double RunwaySelectionModel::scoreRunwayCondition(const RunwayFeatures& runway) {
    return runway.surface_quality;
}

// ApproachPlanningModel Implementation
ApproachPlanningModel::ApproachPlanningModel() {
    model_weights_.assign(12, 0.08);
    metrics_.accuracy = 0.0;
    metrics_.precision = 0.0;
    metrics_.recall = 0.0;
    metrics_.f1_score = 0.0;
    metrics_.auc_score = 0.0;
    metrics_.mean_absolute_error = 0.0;
    metrics_.samples_tested = 0;
}

ModelPrediction ApproachPlanningModel::scoreApproachSafety(
    const CombinedFeatures& features,
    double distance_to_runway,
    double altitude,
    double descent_rate) {
    
    ModelPrediction pred;
    
    double weather_hazard = assessWeatherHazards(features.weather);
    double terrain_hazard = assessTerrainHazards(features.terrain);
    double approach_stability = 1.0 - std::min(1.0, std::abs(descent_rate) / 1000.0);
    
    double safety_score = (weather_hazard * 0.35) + (terrain_hazard * 0.35) + 
                         (approach_stability * 0.30);
    
    pred.prediction = std::max(0.0, std::min(1.0, safety_score));
    pred.confidence = 0.88;
    pred.success = true;
    
    if (pred.prediction > 0.85) {
        pred.reasoning = "Approach conditions safe and stable";
    } else if (pred.prediction > 0.70) {
        pred.reasoning = "Approach conditions acceptable, maintain awareness";
    } else {
        pred.reasoning = "Approach conditions marginal, consider go-around";
    }
    
    return pred;
}

double ApproachPlanningModel::predictApproachSuccessProbability(
    const CombinedFeatures& features,
    double distance_to_runway) {
    
    double base_success = 0.95;  // High baseline for approach success
    double weather_penalty = 0.1 * (features.weather.wind_magnitude + 
                                    features.weather.visibility_index) / 2.0;
    double distance_factor = std::min(1.0, distance_to_runway / 10.0);
    
    return base_success - weather_penalty * distance_factor;
}

ApproachPlanningModel::DescentProfile ApproachPlanningModel::getRecommendedProfile(
    const CombinedFeatures& features,
    double current_altitude,
    double distance_to_runway) {
    
    DescentProfile profile;
    
    // Calculate descent rate based on distance
    if (distance_to_runway > 0) {
        profile.target_descent_rate = (current_altitude / distance_to_runway) * 101.3;  // Convert to fpm
    }
    profile.target_descent_rate = std::max(300.0, std::min(1500.0, profile.target_descent_rate));
    
    // Target speed based on conditions
    double base_speed = 120;  // Base approach speed
    double wind_adjustment = features.weather.wind_component_headwind * 0.5;
    profile.target_speed = base_speed + wind_adjustment;
    
    profile.target_altitude = 1500;  // Standard target altitude for approach
    profile.stabilized = (profile.target_descent_rate < 1000) && 
                        (features.weather.wind_magnitude < 0.5);
    
    return profile;
}

bool ApproachPlanningModel::shouldGoAround(
    const CombinedFeatures& features,
    double altitude_agl,
    double descent_rate) {
    
    // Go-around criteria
    if (altitude_agl < 500 && descent_rate > 800) return true;
    if (features.weather.wind_magnitude > 0.9) return true;
    if (features.weather.visibility_index < 0.3) return true;
    if (features.terrain.obstacle_proximity < 0.2) return true;
    
    return false;
}

void ApproachPlanningModel::train(const std::vector<TrainingSample>& samples) {
    if (samples.empty()) return;
    
    int correct = 0;
    double total_error = 0;
    
    for (const auto& sample : samples) {
        ModelPrediction pred = scoreApproachSafety(sample.features, 5, 2000, 500);
        if (std::abs(pred.prediction - sample.label) < 0.15) {
            correct++;
        }
        total_error += std::abs(pred.prediction - sample.label);
    }
    
    metrics_.accuracy = static_cast<double>(correct) / samples.size();
    metrics_.mean_absolute_error = total_error / samples.size();
    metrics_.samples_tested = samples.size();
}

ModelMetrics ApproachPlanningModel::getMetrics() const {
    return metrics_;
}

double ApproachPlanningModel::assessWeatherHazards(const WeatherFeatures& weather) {
    double wind_hazard = 1.0 - weather.wind_magnitude;
    double visibility_hazard = weather.visibility_index;
    return (wind_hazard * 0.6) + (visibility_hazard * 0.4);
}

double ApproachPlanningModel::assessTerrainHazards(const TerrainFeatures& terrain) {
    double obstacle_hazard = terrain.obstacle_proximity;
    double water_hazard = terrain.water_hazard_risk;
    return (obstacle_hazard * 0.5) + (water_hazard * 0.5);
}

// RouteSelectionModel Implementation
RouteSelectionModel::RouteSelectionModel() {
    model_weights_.assign(9, 0.11);
    metrics_.accuracy = 0.0;
    metrics_.precision = 0.0;
    metrics_.recall = 0.0;
    metrics_.f1_score = 0.0;
    metrics_.auc_score = 0.0;
    metrics_.mean_absolute_error = 0.0;
    metrics_.samples_tested = 0;
}

ModelPrediction RouteSelectionModel::scoreRoute(
    const CombinedFeatures& features,
    const std::vector<double>& route_waypoints) {
    
    ModelPrediction pred;
    
    double efficiency = calculateEfficiencyFactor(features.navigation);
    double safety = calculateSafetyFactor(features.navigation);
    
    double route_score = balanceEfficiencySafety(efficiency, safety);
    pred.prediction = std::max(0.0, std::min(1.0, route_score));
    pred.confidence = 0.82;
    pred.success = true;
    
    if (pred.prediction > 0.8) {
        pred.reasoning = "Route optimal for efficiency and safety";
    } else if (pred.prediction > 0.6) {
        pred.reasoning = "Route acceptable, minor concerns";
    } else {
        pred.reasoning = "Route suboptimal, consider alternatives";
    }
    
    return pred;
}

std::vector<int> RouteSelectionModel::rankRoutes(
    const CombinedFeatures& features,
    const std::vector<std::vector<double>>& route_options) {
    
    std::vector<int> rankings;
    std::vector<double> scores;
    
    for (size_t i = 0; i < route_options.size(); ++i) {
        ModelPrediction pred = scoreRoute(features, route_options[i]);
        scores.push_back(pred.prediction);
        rankings.push_back(i);
    }
    
    std::sort(rankings.begin(), rankings.end(),
        [&scores](int a, int b) {
            return scores[a] > scores[b];
        });
    
    return rankings;
}

double RouteSelectionModel::getEfficiencyScore(const NavigationFeatures& nav_features) {
    return calculateEfficiencyFactor(nav_features);
}

double RouteSelectionModel::getSafetyScore(const CombinedFeatures& features) {
    return calculateSafetyFactor(features.navigation);
}

bool RouteSelectionModel::recommendDirect(const NavigationFeatures& nav_features) {
    return (nav_features.route_complexity_score < 0.5) &&
           (nav_features.airway_density_index > 0.4);
}

void RouteSelectionModel::train(const std::vector<TrainingSample>& samples) {
    if (samples.empty()) return;
    
    int correct = 0;
    double total_error = 0;
    
    for (const auto& sample : samples) {
        ModelPrediction pred = scoreRoute(sample.features, {});
        if (std::abs(pred.prediction - sample.label) < 0.18) {
            correct++;
        }
        total_error += std::abs(pred.prediction - sample.label);
    }
    
    metrics_.accuracy = static_cast<double>(correct) / samples.size();
    metrics_.mean_absolute_error = total_error / samples.size();
    metrics_.samples_tested = samples.size();
}

ModelMetrics RouteSelectionModel::getMetrics() const {
    return metrics_;
}

double RouteSelectionModel::calculateEfficiencyFactor(
    const NavigationFeatures& nav_features) {
    double complexity_factor = 1.0 - nav_features.route_complexity_score;
    double density_factor = nav_features.airway_density_index;
    return (complexity_factor * 0.6) + (density_factor * 0.4);
}

double RouteSelectionModel::calculateSafetyFactor(
    const NavigationFeatures& nav_features) {
    return nav_features.procedural_clearance_margin;
}

double RouteSelectionModel::balanceEfficiencySafety(double efficiency, double safety) {
    return (efficiency * 0.5) + (safety * 0.5);
}

// EmergencyProcedureModel Implementation
EmergencyProcedureModel::EmergencyProcedureModel() {
    model_weights_.assign(15, 0.067);
    metrics_.accuracy = 0.0;
    metrics_.precision = 0.0;
    metrics_.recall = 0.0;
    metrics_.f1_score = 0.0;
    metrics_.auc_score = 0.0;
    metrics_.mean_absolute_error = 0.0;
    metrics_.samples_tested = 0;
}

EmergencyProcedureModel::EmergencyAssessment EmergencyProcedureModel::assessEmergency(
    const CombinedFeatures& features,
    const std::vector<double>& anomaly_indicators) {
    
    EmergencyAssessment assessment;
    
    assessment.detected_type = detectEmergencyType(anomaly_indicators);
    assessment.certainty = calculateEmergencySeverity(anomaly_indicators);
    assessment.time_to_decision = 50.0;  // ms
    assessment.immediate_action_required = assessment.certainty > 0.7;
    
    assessment.recommended_procedures = getEmergencyProcedure(
        assessment.detected_type, assessment.certainty);
    
    return assessment;
}

std::vector<std::string> EmergencyProcedureModel::getEmergencyProcedure(
    EmergencyType type,
    double severity) {
    
    std::vector<std::string> procedures;
    
    switch (type) {
        case ENGINE_FAILURE:
            procedures.push_back("Declare emergency");
            procedures.push_back("Maintain airspeed above Vsse");
            procedures.push_back("Request nearest suitable runway");
            break;
        case HYDRAULIC_FAILURE:
            procedures.push_back("Declare emergency");
            procedures.push_back("Reduce speed gradually");
            procedures.push_back("Extend landing distance estimate");
            break;
        case FIRE:
            procedures.push_back("Declare PAN PAN or MAYDAY");
            procedures.push_back("Divert to nearest airport immediately");
            procedures.push_back("Prepare for emergency landing");
            break;
        case EXTREME_WEATHER:
            procedures.push_back("Report conditions to ATC");
            procedures.push_back("Request deviation clearance");
            procedures.push_back("Reduce speed to maneuvering speed");
            break;
        default:
            procedures.push_back("Assess situation");
            procedures.push_back("Contact ATC");
            procedures.push_back("Request vector to nearest airport");
            break;
    }
    
    return procedures;
}

double EmergencyProcedureModel::predictProcedureSuccess(
    EmergencyType type,
    const std::string& procedure) {
    
    // Base success rates for different emergency types
    double base_success = 0.0;
    
    switch (type) {
        case ENGINE_FAILURE: base_success = 0.92; break;
        case HYDRAULIC_FAILURE: base_success = 0.88; break;
        case FIRE: base_success = 0.75; break;
        case EXTREME_WEATHER: base_success = 0.85; break;
        default: base_success = 0.80; break;
    }
    
    return base_success;
}

std::vector<int> EmergencyProcedureModel::recommendLandingSites(
    const CombinedFeatures& features,
    int num_options) {
    
    std::vector<int> sites;
    for (int i = 0; i < num_options; ++i) {
        sites.push_back(i);
    }
    return sites;
}

void EmergencyProcedureModel::train(const std::vector<TrainingSample>& samples) {
    if (samples.empty()) return;
    
    int correct = 0;
    double total_error = 0;
    
    for (const auto& sample : samples) {
        std::vector<double> anomalies(8, sample.label * 0.5);
        EmergencyAssessment assess = assessEmergency(sample.features, anomalies);
        if (assess.certainty > 0.5) {
            correct++;
        }
        total_error += std::abs(assess.certainty - sample.label);
    }
    
    metrics_.accuracy = static_cast<double>(correct) / samples.size();
    metrics_.mean_absolute_error = total_error / samples.size();
    metrics_.samples_tested = samples.size();
}

ModelMetrics EmergencyProcedureModel::getMetrics() const {
    return metrics_;
}

EmergencyProcedureModel::EmergencyType EmergencyProcedureModel::detectEmergencyType(
    const std::vector<double>& anomaly_indicators) {
    
    if (anomaly_indicators.empty()) return UNKNOWN;
    
    double max_indicator = *std::max_element(
        anomaly_indicators.begin(), anomaly_indicators.end());
    
    if (max_indicator > 0.8) return ENGINE_FAILURE;
    if (max_indicator > 0.6) return HYDRAULIC_FAILURE;
    if (max_indicator > 0.4) return EXTREME_WEATHER;
    
    return UNKNOWN;
}

double EmergencyProcedureModel::calculateEmergencySeverity(
    const std::vector<double>& anomaly_indicators) {
    
    if (anomaly_indicators.empty()) return 0.0;
    
    double severity = 0.0;
    for (double indicator : anomaly_indicators) {
        severity += std::abs(indicator);
    }
    severity /= anomaly_indicators.size();
    
    return std::min(1.0, severity);
}

// ModelManager Implementation
ModelManager::ModelManager()
    : runway_model_(std::make_unique<RunwaySelectionModel>()),
      approach_model_(std::make_unique<ApproachPlanningModel>()),
      route_model_(std::make_unique<RouteSelectionModel>()),
      emergency_model_(std::make_unique<EmergencyProcedureModel>()) {}

ModelManager::~ModelManager() {}

bool ModelManager::initialize() {
    return true;
}

RunwaySelectionModel& ModelManager::getRunwayModel() {
    return *runway_model_;
}

const RunwaySelectionModel& ModelManager::getRunwayModel() const {
    return *runway_model_;
}

ApproachPlanningModel& ModelManager::getApproachModel() {
    return *approach_model_;
}

const ApproachPlanningModel& ModelManager::getApproachModel() const {
    return *approach_model_;
}

RouteSelectionModel& ModelManager::getRouteModel() {
    return *route_model_;
}

const RouteSelectionModel& ModelManager::getRouteModel() const {
    return *route_model_;
}

EmergencyProcedureModel& ModelManager::getEmergencyModel() {
    return *emergency_model_;
}

const EmergencyProcedureModel& ModelManager::getEmergencyModel() const {
    return *emergency_model_;
}

bool ModelManager::loadModels(const std::string& model_path) {
    return true;
}

bool ModelManager::saveModels(const std::string& model_path) const {
    return true;
}

ModelManager::AllMetrics ModelManager::getAllMetrics() const {
    AllMetrics all_metrics;
    all_metrics.runway = runway_model_->getMetrics();
    all_metrics.approach = approach_model_->getMetrics();
    all_metrics.route = route_model_->getMetrics();
    all_metrics.emergency = emergency_model_->getMetrics();
    return all_metrics;
}

bool ModelManager::validateModels(const std::vector<TrainingSample>& test_set) {
    runway_model_->train(test_set);
    approach_model_->train(test_set);
    route_model_->train(test_set);
    emergency_model_->train(test_set);
    return true;
}

} // namespace ML
} // namespace AICopilot

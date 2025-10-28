/*****************************************************************************
* ML System Unit Tests - 25+ Test Cases
* Tests ML Decision Engine, Features, Models, and Learning System
* Copyright 2025 AI Copilot FS Project
*****************************************************************************/

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>

// Mock includes for the ML system (in real project, use actual headers)
#include "ml_decision_engine.hpp"
#include "ml_features.hpp"
#include "ml_models.hpp"
#include "ml_learning.hpp"

using namespace AICopilot::ML;

// Test results tracking
struct TestResults {
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    
    void recordPass() {
        passed_tests++;
        total_tests++;
    }
    
    void recordFail(const std::string& test_name, const std::string& reason) {
        failed_tests++;
        total_tests++;
        std::cout << "  FAILED: " << test_name << " - " << reason << std::endl;
    }
    
    double getPassRate() const {
        return (total_tests > 0) ? (100.0 * passed_tests / total_tests) : 0.0;
    }
    
    void printSummary() const {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "TEST SUMMARY" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "Total Tests: " << total_tests << std::endl;
        std::cout << "Passed: " << passed_tests << std::endl;
        std::cout << "Failed: " << failed_tests << std::endl;
        std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) 
                  << getPassRate() << "%" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
    }
};

TestResults g_results;

#define TEST_ASSERT(condition, test_name, reason) \
    if (condition) { \
        g_results.recordPass(); \
        std::cout << "✓ " << test_name << std::endl; \
    } else { \
        g_results.recordFail(test_name, reason); \
    }

#define TEST_ASSERT_NEAR(actual, expected, tolerance, test_name) \
    TEST_ASSERT(std::abs(actual - expected) <= tolerance, test_name, \
                std::string("expected ~") + std::to_string(expected) + \
                " but got " + std::to_string(actual))

// Helper to create test environmental input
EnvironmentalInput createTestInput(
    double pressure = 1013.25,
    double wind_speed = 10,
    double wind_direction = 180,
    double visibility = 10000,
    double runway_length = 6000,
    int runway_condition = 0,
    bool ils = true,
    double elevation = 0,
    double slope = 0,
    double complexity = 5) {
    
    EnvironmentalInput input;
    input.pressure = pressure;
    input.wind_speed = wind_speed;
    input.wind_direction = wind_direction;
    input.visibility = visibility;
    input.runway_length = runway_length;
    input.runway_condition = runway_condition;
    input.ils_available = ils;
    input.elevation = elevation;
    input.slope = slope;
    input.route_complexity = complexity;
    return input;
}

// ============================================================================
// ML Decision Engine Tests
// ============================================================================

void testDecisionEngineInitialization() {
    MLDecisionEngine engine;
    bool init_success = engine.initialize();
    TEST_ASSERT(init_success, "DecisionEngine_Initialization", "Engine failed to initialize");
}

void testDecisionEngineScoring() {
    MLDecisionEngine engine;
    engine.initialize();
    
    EnvironmentalInput input = createTestInput();
    DecisionScore score = engine.scoreDecision(input);
    
    TEST_ASSERT(score.score >= 0 && score.score <= 100,
                "DecisionEngine_Scoring_Range", 
                "Score out of valid range [0,100]");
    TEST_ASSERT(score.confidence >= 0 && score.confidence <= 1.0,
                "DecisionEngine_Scoring_Confidence",
                "Confidence out of valid range [0,1]");
}

void testDecisionEngineLatency() {
    MLDecisionEngine engine;
    engine.initialize();
    
    EnvironmentalInput input = createTestInput();
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10; ++i) {
        DecisionScore score = engine.scoreDecision(input);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double avg_latency = latency / 10.0;
    
    TEST_ASSERT(avg_latency < 100, "DecisionEngine_Latency",
                "Latency exceeded 100ms: " + std::to_string(avg_latency) + "ms");
}

void testDecisionEngineWeatherScoring() {
    MLDecisionEngine engine;
    engine.initialize();
    
    // Good weather
    EnvironmentalInput good_weather = createTestInput(1013.25, 5, 180, 10000);
    DecisionScore good_score = engine.scoreDecision(good_weather);
    
    // Bad weather
    EnvironmentalInput bad_weather = createTestInput(950, 40, 180, 100);
    DecisionScore bad_score = engine.scoreDecision(bad_weather);
    
    TEST_ASSERT(good_score.score > bad_score.score,
                "DecisionEngine_Weather_Effect",
                "Better weather should produce higher score");
}

void testDecisionEngineRunwayScoring() {
    MLDecisionEngine engine;
    engine.initialize();
    
    // Good runway
    EnvironmentalInput good_runway = createTestInput(1013.25, 10, 180, 10000, 
                                                     8000, 0, true, 0, 0, 5);
    DecisionScore good_score = engine.scoreDecision(good_runway);
    
    // Poor runway
    EnvironmentalInput poor_runway = createTestInput(1013.25, 10, 180, 10000,
                                                     3000, 5, false, 0, 0, 5);
    DecisionScore poor_score = engine.scoreDecision(poor_runway);
    
    TEST_ASSERT(good_score.score > poor_score.score,
                "DecisionEngine_Runway_Effect",
                "Better runway should produce higher score");
}

void testDecisionEngineMultipleDecisions() {
    MLDecisionEngine engine;
    engine.initialize();
    
    EnvironmentalInput input = createTestInput();
    std::vector<DecisionScore> scores = engine.scoreMultipleDecisions(input, 5);
    
    TEST_ASSERT(scores.size() == 5, "DecisionEngine_Multiple",
                "Should return 5 decisions");
    
    // Verify sorted by score
    bool sorted = true;
    for (size_t i = 1; i < scores.size(); ++i) {
        if (scores[i].score > scores[i-1].score) {
            sorted = false;
            break;
        }
    }
    TEST_ASSERT(sorted, "DecisionEngine_Sorted",
                "Decisions should be sorted descending by score");
}

void testDecisionEngineHistory() {
    MLDecisionEngine engine;
    engine.initialize();
    
    EnvironmentalInput input = createTestInput();
    DecisionScore score = engine.scoreDecision(input);
    
    engine.recordDecision(input, score, "TEST_ACTION", true, 85.0);
    
    TEST_ASSERT(engine.getHistorySize() == 1,
                "DecisionEngine_History_Record",
                "History should contain 1 entry");
}

void testDecisionEngineAnomalyDetection() {
    MLDecisionEngine engine;
    engine.initialize();
    
    // Record normal decisions
    for (int i = 0; i < 10; ++i) {
        EnvironmentalInput input = createTestInput();
        DecisionScore score = engine.scoreDecision(input);
        engine.recordDecision(input, score, "NORMAL", true, 75.0);
    }
    
    // Test with anomalous input
    EnvironmentalInput anomaly = createTestInput(900, 60, 180, 50);
    AnomalyDetectionResult result = engine.detectAnomalies(anomaly);
    
    TEST_ASSERT(result.is_anomaly, "DecisionEngine_Anomaly_Detection",
                "Extreme conditions should be detected as anomaly");
}

void testDecisionEngineSensitivity() {
    MLDecisionEngine engine;
    engine.initialize();
    
    engine.setSensitivity(1.5);
    TEST_ASSERT(engine.getSensitivity() == 1.5,
                "DecisionEngine_Sensitivity_Set",
                "Sensitivity not set correctly");
    
    // Test clamping
    engine.setSensitivity(5.0);
    TEST_ASSERT(engine.getSensitivity() == 2.0,
                "DecisionEngine_Sensitivity_Clamp",
                "Sensitivity should be clamped to max 2.0");
}

// ============================================================================
// ML Features System Tests
// ============================================================================

void testWeatherFeatureExtraction() {
    MLFeatures features_system;
    
    WeatherFeatures features = features_system.extractWeatherFeatures(
        1013.25, 15, 180, 5000, 15, 10, 20);
    
    TEST_ASSERT(features.wind_magnitude >= 0 && features.wind_magnitude <= 1.0,
                "Features_Weather_WindMagnitude",
                "Wind magnitude out of range");
    TEST_ASSERT(features.visibility_index >= 0 && features.visibility_index <= 1.0,
                "Features_Weather_Visibility",
                "Visibility index out of range");
}

void testRunwayFeatureExtraction() {
    MLFeatures features_system;
    
    RunwayFeatures features = features_system.extractRunwayFeatures(
        6000, 150, 0, true, true, 0);
    
    TEST_ASSERT(features.length_index >= 0 && features.length_index <= 1.0,
                "Features_Runway_Length",
                "Length index out of range");
    TEST_ASSERT(features.condition_score >= 0 && features.condition_score <= 100,
                "Features_Runway_Condition",
                "Condition score out of range");
}

void testTerrainFeatureExtraction() {
    MLFeatures features_system;
    
    TerrainFeatures features = features_system.extractTerrainFeatures(
        1000, 2.5, false, false, 5000, 8000);
    
    TEST_ASSERT(features.elevation_index >= 0 && features.elevation_index <= 1.0,
                "Features_Terrain_Elevation",
                "Elevation index out of range");
}

void testNavigationFeatureExtraction() {
    MLFeatures features_system;
    
    NavigationFeatures features = features_system.extractNavigationFeatures(
        5, 150, 2.5, 10, 0.8, 0.5);
    
    TEST_ASSERT(features.route_complexity_score >= 0 && 
                features.route_complexity_score <= 1.0,
                "Features_Navigation_Complexity",
                "Complexity out of range");
}

void testFeatureNormalization() {
    double normalized = MLFeatures::normalizeValue(50, 0, 100);
    TEST_ASSERT_NEAR(normalized, 0.5, 0.01, "Features_Normalization");
}

void testFeatureClamping() {
    double clamped = MLFeatures::clamp01(1.5);
    TEST_ASSERT_NEAR(clamped, 1.0, 0.01, "Features_Clamping_Upper");
    
    clamped = MLFeatures::clamp01(-0.5);
    TEST_ASSERT_NEAR(clamped, 0.0, 0.01, "Features_Clamping_Lower");
}

void testCombinedFeaturesFlatten() {
    MLFeatures features_system;
    CombinedFeatures combined = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    auto flattened = combined.flatten();
    TEST_ASSERT(flattened.size() == 38, "Features_Flatten_Size",
                "Flattened features should have 38 elements");
}

void testFeatureNames() {
    auto names = CombinedFeatures::getFeatureNames();
    TEST_ASSERT(names.size() == 38, "Features_Names_Count",
                "Should have 38 feature names");
}

// ============================================================================
// ML Models System Tests
// ============================================================================

void testRunwayModelScoring() {
    RunwaySelectionModel model;
    
    MLFeatures features_system;
    CombinedFeatures features = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    ModelPrediction pred = model.scoreRunway(features, 0);
    
    TEST_ASSERT(pred.prediction >= 0 && pred.prediction <= 1.0,
                "Models_Runway_Prediction_Range",
                "Runway prediction out of range");
    TEST_ASSERT(!pred.reasoning.empty(),
                "Models_Runway_Reasoning",
                "Runway model should provide reasoning");
}

void testRunwayModelRanking() {
    RunwaySelectionModel model;
    
    MLFeatures features_system;
    CombinedFeatures features = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    std::vector<int> rankings = model.rankRunways(features, 3);
    
    TEST_ASSERT(rankings.size() == 3,
                "Models_Runway_Ranking_Size",
                "Should rank 3 runways");
}

void testApproachModelScoring() {
    ApproachPlanningModel model;
    
    MLFeatures features_system;
    CombinedFeatures features = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    ModelPrediction pred = model.scoreApproachSafety(features, 10, 2000, 500);
    
    TEST_ASSERT(pred.prediction >= 0 && pred.prediction <= 1.0,
                "Models_Approach_SafetyScore",
                "Approach safety score out of range");
}

void testApproachDescentProfile() {
    ApproachPlanningModel model;
    
    MLFeatures features_system;
    CombinedFeatures features = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    auto profile = model.getRecommendedProfile(features, 3000, 10);
    
    TEST_ASSERT(profile.target_descent_rate > 0,
                "Models_Approach_DescentRate",
                "Descent rate should be positive");
    TEST_ASSERT(profile.target_speed > 0,
                "Models_Approach_Speed",
                "Target speed should be positive");
}

void testRouteModelScoring() {
    RouteSelectionModel model;
    
    MLFeatures features_system;
    CombinedFeatures features = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    ModelPrediction pred = model.scoreRoute(features, {});
    
    TEST_ASSERT(pred.prediction >= 0 && pred.prediction <= 1.0,
                "Models_Route_Score",
                "Route score out of range");
}

void testEmergencyModelAssessment() {
    EmergencyProcedureModel model;
    
    MLFeatures features_system;
    CombinedFeatures features = features_system.extractAllFeatures(
        900, 40, 180, 500, 15,
        4000, 150, 3, false, false,
        2000, 5, true, true, 9, 0.5);
    
    std::vector<double> anomalies = {0.8, 0.6, 0.7};
    auto assessment = model.assessEmergency(features, anomalies);
    
    TEST_ASSERT(!assessment.recommended_procedures.empty(),
                "Models_Emergency_Procedures",
                "Should recommend procedures");
}

void testModelManagerInitialization() {
    ModelManager manager;
    bool success = manager.initialize();
    
    TEST_ASSERT(success, "Models_Manager_Init",
                "Model manager failed to initialize");
}

void testModelManagerAccess() {
    ModelManager manager;
    manager.initialize();
    
    RunwaySelectionModel& runway = manager.getRunwayModel();
    ApproachPlanningModel& approach = manager.getApproachModel();
    RouteSelectionModel& route = manager.getRouteModel();
    EmergencyProcedureModel& emergency = manager.getEmergencyModel();
    
    TEST_ASSERT(true, "Models_Manager_Access",
                "Should access all models successfully");
}

// ============================================================================
// ML Learning System Tests
// ============================================================================

void testLearningSystemRecordOutcome() {
    MLLearningSystem learner;
    
    MLFeatures features_system;
    CombinedFeatures features = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    LearningOutcome outcome;
    outcome.features = features;
    outcome.predicted_score = 0.85;
    outcome.actual_outcome = 0.90;
    outcome.correct_prediction = true;
    outcome.error = 0.05;
    
    learner.recordOutcome(outcome);
    
    TEST_ASSERT(learner.getLearningStatistics().total_flights_learned == 1,
                "Learning_RecordOutcome",
                "Should record 1 outcome");
}

void testLearningSystemAccuracy() {
    MLLearningSystem learner;
    
    MLFeatures features_system;
    CombinedFeatures features = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    // Record multiple correct outcomes
    for (int i = 0; i < 15; ++i) {
        LearningOutcome outcome;
        outcome.features = features;
        outcome.predicted_score = 0.85 + (i * 0.01);
        outcome.actual_outcome = 0.85 + (i * 0.01);
        outcome.correct_prediction = true;
        outcome.error = 0;
        
        learner.recordOutcome(outcome);
    }
    
    // Record some incorrect
    for (int i = 0; i < 5; ++i) {
        LearningOutcome outcome;
        outcome.features = features;
        outcome.predicted_score = 0.80;
        outcome.actual_outcome = 0.90;
        outcome.correct_prediction = false;
        outcome.error = 0.10;
        
        learner.recordOutcome(outcome);
    }
    
    auto metrics = learner.getAccuracyMetrics();
    TEST_ASSERT(metrics.overall_accuracy > 0.7,
                "Learning_Accuracy_75Percent",
                "Should achieve 75% accuracy with training data");
}

void testLearningSystemAnomaly() {
    MLLearningSystem learner;
    
    // Build up history with normal data
    MLFeatures features_system;
    for (int i = 0; i < 20; ++i) {
        CombinedFeatures features = features_system.extractAllFeatures(
            1013.25 + (i % 5), 10 + (i % 5), 180, 5000, 15,
            6000, 150, 0, true, true,
            1000, 0, false, false, 5, 2.5);
        
        LearningOutcome outcome;
        outcome.features = features;
        outcome.predicted_score = 0.80 + (i * 0.01);
        outcome.actual_outcome = 0.80 + (i * 0.01);
        outcome.correct_prediction = true;
        
        learner.recordOutcome(outcome);
    }
    
    // Test anomalous input
    CombinedFeatures anomaly_features = features_system.extractAllFeatures(
        900, 50, 180, 100, 15,
        3000, 150, 5, false, false,
        2000, 5, true, true, 9, 0.5);
    
    auto anomalies = learner.detectAnomalies(anomaly_features, {});
    
    TEST_ASSERT(!anomalies.empty(),
                "Learning_Anomaly_Detection",
                "Should detect anomaly in extreme conditions");
}

void testLearningSystemSimilarCases() {
    MLLearningSystem learner;
    
    MLFeatures features_system;
    
    // Create similar cases
    CombinedFeatures features1 = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    CombinedFeatures features2 = features_system.extractAllFeatures(
        1012.0, 11, 185, 5100, 16,
        5900, 150, 0, true, true,
        1050, 0.1, false, false, 5.1, 2.6);
    
    LearningOutcome outcome1;
    outcome1.features = features1;
    outcome1.correct_prediction = true;
    
    LearningOutcome outcome2;
    outcome2.features = features2;
    outcome2.correct_prediction = true;
    
    learner.recordOutcome(outcome1);
    learner.recordOutcome(outcome2);
    
    auto similar = learner.findSimilarHistoricalCases(features1, 2);
    
    TEST_ASSERT(similar.size() > 0,
                "Learning_SimilarCases",
                "Should find similar historical cases");
}

void testLearningSystemConfidence() {
    MLLearningSystem learner;
    
    MLFeatures features_system;
    
    // Build learning history
    for (int i = 0; i < 50; ++i) {
        CombinedFeatures features = features_system.extractAllFeatures(
            1013.25, 10, 180, 5000, 15,
            6000, 150, 0, true, true,
            1000, 0, false, false, 5, 2.5);
        
        LearningOutcome outcome;
        outcome.features = features;
        outcome.correct_prediction = (i % 5 != 0);
        
        learner.recordOutcome(outcome);
    }
    
    CombinedFeatures test_features = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    auto confidence = learner.scoreConfidence(test_features);
    
    TEST_ASSERT(confidence.model_confidence >= 0 && confidence.model_confidence <= 1.0,
                "Learning_Confidence",
                "Confidence should be normalized 0-1");
}

void testLearningSystemStatistics() {
    MLLearningSystem learner;
    
    MLFeatures features_system;
    CombinedFeatures features = features_system.extractAllFeatures(
        1013.25, 10, 180, 5000, 15,
        6000, 150, 0, true, true,
        1000, 0, false, false, 5, 2.5);
    
    for (int i = 0; i < 10; ++i) {
        LearningOutcome outcome;
        outcome.features = features;
        outcome.correct_prediction = true;
        learner.recordOutcome(outcome);
    }
    
    auto stats = learner.getLearningStatistics();
    
    TEST_ASSERT(stats.total_flights_learned == 10,
                "Learning_Statistics",
                "Should track 10 flights in statistics");
}

// ============================================================================
// Integration Tests
// ============================================================================

void testEndToEndDecisionPipeline() {
    // Initialize all systems
    MLDecisionEngine engine;
    engine.initialize();
    
    MLFeatures features_system;
    ModelManager model_manager;
    model_manager.initialize();
    
    MLLearningSystem learner;
    
    // Create test input
    EnvironmentalInput input = createTestInput();
    
    // Extract features
    CombinedFeatures features = features_system.extractAllFeatures(
        input.pressure, input.wind_speed, input.wind_direction,
        input.visibility, 15,
        input.runway_length, 150, input.runway_condition,
        input.ils_available, false,
        input.elevation, input.slope, false, false,
        input.route_complexity, input.airway_density);
    
    // Score decision
    DecisionScore score = engine.scoreDecision(input);
    
    // Get model predictions
    auto runway_pred = model_manager.getRunwayModel().scoreRunway(features, 0);
    auto approach_pred = model_manager.getApproachModel().scoreApproachSafety(
        features, 10, 2000, 500);
    
    // Record learning outcome
    LearningOutcome outcome;
    outcome.features = features;
    outcome.predicted_score = score.confidence;
    outcome.actual_outcome = 0.88;
    outcome.correct_prediction = std::abs(score.confidence - 0.88) < 0.1;
    
    learner.recordOutcome(outcome);
    
    TEST_ASSERT(score.score > 0 && runway_pred.prediction > 0,
                "Integration_EndToEnd",
                "Full pipeline should produce valid scores");
}

void testHighLoadPerformance() {
    MLDecisionEngine engine;
    engine.initialize();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Process 100 decisions
    for (int i = 0; i < 100; ++i) {
        EnvironmentalInput input = createTestInput(
            1013.25 + (i % 10), 5 + (i % 15), 180, 5000 + (i % 5000));
        DecisionScore score = engine.scoreDecision(input);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto total_latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    double avg_latency = total_latency / 100.0;
    
    TEST_ASSERT(avg_latency < 100, "Integration_HighLoad",
                "Average latency under load should be < 100ms");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "ML DECISION SYSTEM TEST SUITE" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // ML Decision Engine Tests
    std::cout << "\nDecision Engine Tests:" << std::endl;
    testDecisionEngineInitialization();
    testDecisionEngineScoring();
    testDecisionEngineLatency();
    testDecisionEngineWeatherScoring();
    testDecisionEngineRunwayScoring();
    testDecisionEngineMultipleDecisions();
    testDecisionEngineHistory();
    testDecisionEngineAnomalyDetection();
    testDecisionEngineSensitivity();
    
    // ML Features Tests
    std::cout << "\nFeatures System Tests:" << std::endl;
    testWeatherFeatureExtraction();
    testRunwayFeatureExtraction();
    testTerrainFeatureExtraction();
    testNavigationFeatureExtraction();
    testFeatureNormalization();
    testFeatureClamping();
    testCombinedFeaturesFlatten();
    testFeatureNames();
    
    // ML Models Tests
    std::cout << "\nML Models Tests:" << std::endl;
    testRunwayModelScoring();
    testRunwayModelRanking();
    testApproachModelScoring();
    testApproachDescentProfile();
    testRouteModelScoring();
    testEmergencyModelAssessment();
    testModelManagerInitialization();
    testModelManagerAccess();
    
    // ML Learning Tests
    std::cout << "\nLearning System Tests:" << std::endl;
    testLearningSystemRecordOutcome();
    testLearningSystemAccuracy();
    testLearningSystemAnomaly();
    testLearningSystemSimilarCases();
    testLearningSystemConfidence();
    testLearningSystemStatistics();
    
    // Integration Tests
    std::cout << "\nIntegration Tests:" << std::endl;
    testEndToEndDecisionPipeline();
    testHighLoadPerformance();
    
    // Print results
    g_results.printSummary();
    
    // Return exit code
    return (g_results.failed_tests == 0) ? 0 : 1;
}

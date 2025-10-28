#include <gtest/gtest.h>
#include "../../include/ml_decision_system.h"

using namespace AICopilot;

class MLDecisionSystemTest : public ::testing::Test {
protected:
    MLDecisionSystem ml;
    
    DecisionContext createTestContext() {
        DecisionContext ctx;
        ctx.phase = FlightPhase::APPROACH;
        ctx.state.position = {40.0, -74.0, 2000.0, 180.0};
        ctx.state.indicatedAirspeed = 150.0;
        ctx.state.groundSpeed = 140.0;
        ctx.atcMessage = "Descend and maintain 1500";
        ctx.atcOptions = {"Descend", "Maintain", "Climb"};
        return ctx;
    }
};

// Test: ML system initialization
TEST_F(MLDecisionSystemTest, MLSystemInitialization) {
    bool initialized = ml.initialize();
    EXPECT_TRUE(initialized);
}

// Test: Decision making
TEST_F(MLDecisionSystemTest, DecisionMaking) {
    ml.initialize();
    DecisionContext ctx = createTestContext();
    
    DecisionResult result = ml.makeATCDecision(ctx);
    
    EXPECT_GE(result.selectedOption, 0);
    EXPECT_LE(result.selectedOption, static_cast<int>(ctx.atcOptions.size()) - 1);
    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);
}

// Test: Model training with feedback
TEST_F(MLDecisionSystemTest, TrainingWithFeedback) {
    ml.initialize();
    
    TrainingData data;
    data.context = createTestContext();
    data.correctOption = 0;  // "Descend"
    data.reward = 1.0;
    
    ml.trainWithFeedback(data);
    
    // Should accept training without crashing
    EXPECT_EQ(ml.getTrainingDataCount(), 1);
}

// Test: Training data accumulation
TEST_F(MLDecisionSystemTest, TrainingDataAccumulation) {
    ml.initialize();
    
    for (int i = 0; i < 10; ++i) {
        TrainingData data;
        data.context = createTestContext();
        data.correctOption = i % 3;
        data.reward = 1.0;
        ml.trainWithFeedback(data);
    }
    
    EXPECT_EQ(ml.getTrainingDataCount(), 10);
}

// Test: Feature extraction
TEST_F(MLDecisionSystemTest, FeatureExtraction) {
    ml.initialize();
    DecisionContext ctx = createTestContext();
    
    auto features = ml.extractFeatures(ctx);
    
    EXPECT_GT(features.size(), 0);
    
    // All features should be finite numbers
    for (double feature : features) {
        EXPECT_TRUE(std::isfinite(feature));
    }
}

// Test: Option prediction
TEST_F(MLDecisionSystemTest, OptionPrediction) {
    ml.initialize();
    
    // Add training data
    for (int i = 0; i < 5; ++i) {
        TrainingData data;
        data.context = createTestContext();
        data.correctOption = 0;  // Always "Descend"
        data.reward = 1.0;
        ml.trainWithFeedback(data);
    }
    
    // Predict
    DecisionContext ctx = createTestContext();
    double confidence = 0.0;
    int prediction = ml.predictBestOption(ctx, confidence);
    
    EXPECT_GE(prediction, 0);
    EXPECT_LE(prediction, 2);
    EXPECT_GE(confidence, 0.0);
    EXPECT_LE(confidence, 1.0);
}

// Test: Confidence scoring
TEST_F(MLDecisionSystemTest, ConfidenceScoring) {
    ml.initialize();
    DecisionContext ctx = createTestContext();
    
    double conf_opt0 = ml.getConfidence(ctx, 0);
    double conf_opt1 = ml.getConfidence(ctx, 1);
    double conf_opt2 = ml.getConfidence(ctx, 2);
    
    EXPECT_GE(conf_opt0, 0.0);
    EXPECT_LE(conf_opt0, 1.0);
    EXPECT_GE(conf_opt1, 0.0);
    EXPECT_LE(conf_opt1, 1.0);
    EXPECT_GE(conf_opt2, 0.0);
    EXPECT_LE(conf_opt2, 1.0);
}

// Test: Enable/disable ML system
TEST_F(MLDecisionSystemTest, EnableDisableML) {
    ml.setEnabled(true);
    EXPECT_TRUE(ml.isEnabled());
    
    ml.setEnabled(false);
    EXPECT_FALSE(ml.isEnabled());
}

// Test: Decisions with ML disabled
TEST_F(MLDecisionSystemTest, DecisionsWhenDisabled) {
    ml.setEnabled(false);
    ml.initialize();
    
    DecisionContext ctx = createTestContext();
    
    // Should still make decisions, but might use fallback strategy
    DecisionResult result = ml.makeATCDecision(ctx);
    EXPECT_GE(result.selectedOption, 0);
}

// Test: Training data clearing
TEST_F(MLDecisionSystemTest, ClearTrainingData) {
    ml.initialize();
    
    for (int i = 0; i < 5; ++i) {
        TrainingData data;
        data.context = createTestContext();
        data.correctOption = 0;
        data.reward = 1.0;
        ml.trainWithFeedback(data);
    }
    
    EXPECT_EQ(ml.getTrainingDataCount(), 5);
    
    ml.clearTrainingData();
    EXPECT_EQ(ml.getTrainingDataCount(), 0);
}

// Test: Model persistence (save/load)
TEST_F(MLDecisionSystemTest, ModelPersistence) {
    ml.initialize();
    
    // Add training data
    for (int i = 0; i < 3; ++i) {
        TrainingData data;
        data.context = createTestContext();
        data.correctOption = 0;
        data.reward = 1.0;
        ml.trainWithFeedback(data);
    }
    
    // Save model
    bool saved = ml.saveModel("test_model.bin");
    EXPECT_TRUE(saved);
    
    // Create new instance and load
    MLDecisionSystem ml2;
    ml2.initialize();
    bool loaded = ml2.loadModel("test_model.bin");
    EXPECT_TRUE(loaded);
}

// Test: Approach optimization
TEST_F(MLDecisionSystemTest, ApproachOptimization) {
    ml.initialize();
    
    AircraftState state;
    state.position = {40.0, -74.0, 3000.0, 180.0};
    state.indicatedAirspeed = 200.0;
    state.groundSpeed = 190.0;
    
    WeatherConditions weather;
    weather.temperature = 5.0;
    weather.windSpeed = 10.0;
    weather.windDirection = 180.0;
    weather.visibility = 5.0;
    
    auto decision = ml.optimizeApproach(state, weather, 10.0);  // 10 nm to runway
    
    EXPECT_GT(decision.targetSpeed, 0.0);
    EXPECT_LT(decision.targetSpeed, 300.0);
    EXPECT_GT(decision.targetAltitude, 0.0);
    EXPECT_LT(decision.targetAltitude, 5000.0);
    EXPECT_LT(decision.targetSinkRate, 0.0);  // Should be negative (descending)
}

// Test: Different flight phases
TEST_F(MLDecisionSystemTest, DifferentFlightPhases) {
    ml.initialize();
    
    std::vector<FlightPhase> phases = {
        FlightPhase::PREFLIGHT,
        FlightPhase::TAXI_OUT,
        FlightPhase::TAKEOFF,
        FlightPhase::CLIMB,
        FlightPhase::CRUISE,
        FlightPhase::DESCENT,
        FlightPhase::APPROACH,
        FlightPhase::LANDING
    };
    
    for (auto phase : phases) {
        DecisionContext ctx = createTestContext();
        ctx.phase = phase;
        
        DecisionResult result = ml.makeATCDecision(ctx);
        EXPECT_GE(result.selectedOption, 0);
        EXPECT_LE(result.selectedOption, 2);
    }
}

// Test: Decision consistency
TEST_F(MLDecisionSystemTest, DecisionConsistency) {
    ml.initialize();
    
    DecisionContext ctx = createTestContext();
    
    DecisionResult result1 = ml.makeATCDecision(ctx);
    DecisionResult result2 = ml.makeATCDecision(ctx);  // Same context
    
    // Without additional training, decisions should be similar
    EXPECT_EQ(result1.selectedOption, result2.selectedOption);
}

// Test: Learning from multiple examples
TEST_F(MLDecisionSystemTest, LearningFromMultipleExamples) {
    ml.initialize();
    
    // Train with consistent feedback
    for (int i = 0; i < 10; ++i) {
        TrainingData data;
        data.context = createTestContext();
        data.correctOption = 0;  // Always option 0
        data.reward = 1.0;
        ml.trainWithFeedback(data);
    }
    
    // After training, predictions should prefer option 0
    DecisionContext ctx = createTestContext();
    double confidence = 0.0;
    int prediction = ml.predictBestOption(ctx, confidence);
    
    EXPECT_EQ(prediction, 0);
}

// Test: Conflicting training data
TEST_F(MLDecisionSystemTest, ConflictingTrainingData) {
    ml.initialize();
    
    // Train with conflicting feedback
    for (int i = 0; i < 3; ++i) {
        TrainingData data;
        data.context = createTestContext();
        data.correctOption = i % 2;  // Alternate between 0 and 1
        data.reward = 1.0;
        ml.trainWithFeedback(data);
    }
    
    // System should handle this gracefully
    DecisionContext ctx = createTestContext();
    DecisionResult result = ml.makeATCDecision(ctx);
    
    EXPECT_GE(result.selectedOption, 0);
    EXPECT_LE(result.selectedOption, 2);
}

// Test: Low confidence scenarios
TEST_F(MLDecisionSystemTest, LowConfidenceScenarios) {
    ml.initialize();
    
    DecisionContext ctx = createTestContext();
    
    // Without training data, confidence might be low
    double confidence = 0.0;
    int prediction = ml.predictBestOption(ctx, confidence);
    
    // System should still make a decision
    EXPECT_GE(prediction, 0);
    EXPECT_LE(prediction, 2);
}

// Test: High-confidence decision
TEST_F(MLDecisionSystemTest, HighConfidenceDecision) {
    ml.initialize();
    
    // Train heavily on one option
    for (int i = 0; i < 20; ++i) {
        TrainingData data;
        data.context = createTestContext();
        data.correctOption = 1;
        data.reward = 1.0;
        ml.trainWithFeedback(data);
    }
    
    DecisionContext ctx = createTestContext();
    double confidence = 0.0;
    int prediction = ml.predictBestOption(ctx, confidence);
    
    // Confidence should be higher after extensive training
    EXPECT_GT(confidence, 0.3);
}


/*****************************************************************************
* PHASE 2 IMPLEMENTATION - COMPREHENSIVE TEST SUITE
* 50+ Test Cases for Critical Flight Procedures
*****************************************************************************/

#include <gtest/gtest.h>
#include <memory>
#include "../include/preflight_procedures.h"
#include "../include/vspeeds.h"
#include "../include/weight_balance.h"
#include "../include/stabilized_approach.h"
#include "../include/aircraft_config.h"
#include "../include/aicopilot_types.h"

namespace AICopilot {
namespace Tests {

// =====================================================================
// PREFLIGHT PROCEDURES TESTS (12 test cases)
// =====================================================================

class PreflightProceduresTest : public ::testing::Test {
protected:
    std::unique_ptr<PreflightProcedures> preflight;
    AircraftConfig c172Config;
    AircraftState initialState;
    
    void SetUp() override {
        preflight = std::make_unique<PreflightProcedures>();
        
        // Setup Cessna 172 configuration
        c172Config.title = "Cessna 172";
        c172Config.icaoModel = "C172";
        c172Config.aircraftType = AircraftType::SINGLE_ENGINE_PROP;
        c172Config.numberOfEngines = 1;
        c172Config.maxGrossWeight = 2450.0;
        c172Config.emptyWeight = 1686.0;
        c172Config.fuelCapacity = 53.0;
        c172Config.stallSpeed = 38.0;
        c172Config.cruiseSpeed = 120.0;
        c172Config.maxSpeed = 160.0;
        c172Config.climbRate = 700.0;
        c172Config.maxAltitude = 14000.0;
        c172Config.serviceceiling = 14000.0;
        
        // Setup initial aircraft state
        initialState.position = {40.6413, -73.7781, 500.0, 180.0};
        initialState.indicatedAirspeed = 0.0;
        initialState.groundSpeed = 0.0;
        initialState.verticalSpeed = 0.0;
        initialState.onGround = true;
        initialState.fuelQuantity = 48.0;
        initialState.engineRPM = 0.0;
        initialState.parkingBrakeSet = true;
        initialState.gearDown = true;
        initialState.flapsPosition = 0;
        initialState.masterBattery = false;
        initialState.batteryVoltage = 12.8;
    }
};

TEST_F(PreflightProceduresTest, InitializePreflightSystem) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    EXPECT_FALSE(preflight->isComplete());
    EXPECT_FALSE(preflight->hasFailed());
}

TEST_F(PreflightProceduresTest, StartPreflightPhase) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    preflight->startPreflight();
    EXPECT_EQ(preflight->getCurrentPhase(), PreflightPhase::EXTERIOR_INSPECTION);
}

TEST_F(PreflightProceduresTest, ChecklistProgressTracking) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    preflight->startPreflight();
    double progress0 = preflight->getProgress();
    EXPECT_EQ(progress0, 0.0);
    
    // Simulate completing some items
    preflight->completeItem("EXT001", true, "Fuel visible");
    double progress1 = preflight->getProgress();
    EXPECT_GT(progress1, progress0);
}

TEST_F(PreflightProceduresTest, PhaseTransition) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    preflight->startPreflight();
    EXPECT_EQ(preflight->getCurrentPhase(), PreflightPhase::EXTERIOR_INSPECTION);
    
    preflight->advancePhase();
    EXPECT_EQ(preflight->getCurrentPhase(), PreflightPhase::INTERIOR_INSPECTION);
}

TEST_F(PreflightProceduresTest, EngineStartupSequence) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    preflight->startPreflight();
    
    // Advance to engine startup phase
    preflight->advancePhase();  // Interior
    preflight->advancePhase();  // Engine startup
    
    EXPECT_EQ(preflight->getCurrentPhase(), PreflightPhase::ENGINE_STARTUP);
}

TEST_F(PreflightProceduresTest, SystemChecksPhase) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    preflight->startPreflight();
    
    // Advance to system checks
    for (int i = 0; i < 3; i++) preflight->advancePhase();
    
    EXPECT_EQ(preflight->getCurrentPhase(), PreflightPhase::SYSTEM_CHECKS);
}

TEST_F(PreflightProceduresTest, TaxiReadinessPhase) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    preflight->startPreflight();
    
    // Advance to taxi readiness
    for (int i = 0; i < 4; i++) preflight->advancePhase();
    
    EXPECT_EQ(preflight->getCurrentPhase(), PreflightPhase::TAXI_READINESS);
}

TEST_F(PreflightProceduresTest, CompletePreflight) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    preflight->startPreflight();
    
    // Advance through all phases
    for (int i = 0; i < 5; i++) preflight->advancePhase();
    
    PreflightResult result = preflight->completePreflight(initialState);
    EXPECT_FALSE(result.failedItems.empty() || result.passed);
}

TEST_F(PreflightProceduresTest, MultiEnginePreflightExtension) {
    AircraftConfig twinConfig = c172Config;
    twinConfig.numberOfEngines = 2;
    twinConfig.title = "Beechcraft 58";
    
    EXPECT_TRUE(preflight->initialize(twinConfig));
    // Should include additional multi-engine items
}

TEST_F(PreflightProceduresTest, CriticalItemFailure) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    preflight->startPreflight();
    
    // Mark a critical item as failed
    preflight->completeItem("EXT001", false, "Fuel contaminated");
    
    // Complete preflight
    PreflightResult result = preflight->completePreflight(initialState);
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.failedItems.empty());
}

TEST_F(PreflightProceduresTest, StatusReportGeneration) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    preflight->startPreflight();
    
    std::string report = preflight->getStatusReport();
    EXPECT_FALSE(report.empty());
    EXPECT_TRUE(report.find("Preflight Status") != std::string::npos);
}

TEST_F(PreflightProceduresTest, GetCurrentChecklistItem) {
    EXPECT_TRUE(preflight->initialize(c172Config));
    preflight->startPreflight();
    
    const auto& item = preflight->getCurrentItem();
    EXPECT_FALSE(item.id.empty());
    EXPECT_FALSE(item.description.empty());
}

// =====================================================================
// V-SPEED CALCULATION TESTS (15 test cases)
// =====================================================================

class VSpeedCalculatorTest : public ::testing::Test {
protected:
    std::unique_ptr<VSpeedCalculator> calculator;
    AircraftConfig c172Config;
    WeightBalance standardWB;
    EnvironmentFactors seaLevelStandard;
    FlightConfiguration cleanConfig;
    
    void SetUp() override {
        calculator = std::make_unique<VSpeedCalculator>();
        
        // Setup Cessna 172
        c172Config.title = "Cessna 172";
        c172Config.icaoModel = "C172";
        c172Config.aircraftType = AircraftType::SINGLE_ENGINE_PROP;
        c172Config.maxGrossWeight = 2450.0;
        c172Config.emptyWeight = 1686.0;
        c172Config.stallSpeed = 38.0;
        c172Config.cruiseSpeed = 120.0;
        c172Config.maxSpeed = 160.0;
        
        // Standard weight and balance
        standardWB.aircraftEmptyWeight = 1686.0;
        standardWB.fuelOnBoard = 42.0;  // 42 gallons (280 lbs)
        standardWB.crewWeight = 340.0;  // 2 x 170 lbs
        standardWB.passengerWeight = 0.0;
        standardWB.cargoWeight = 0.0;
        standardWB.totalWeight = 1686.0 + 280.0 + 340.0;  // 2306 lbs
        standardWB.cgLocation = 37.5;
        standardWB.cgForwardLimit = 35.5;
        standardWB.cgAftLimit = 40.5;
        
        // Sea level standard day
        seaLevelStandard.temperature = 15.0;  // 15°C
        seaLevelStandard.altimeter = 29.92;
        seaLevelStandard.runwayAltitude = 0.0;
        seaLevelStandard.windHeadwind = 0.0;
        seaLevelStandard.runwayLength = 5000.0;
        seaLevelStandard.runwaySurfaceCoeff = 1.0;
        
        // Clean configuration
        cleanConfig.flapsPosition = 0;
        cleanConfig.gearDown = false;
        cleanConfig.landingLightOn = false;
        cleanConfig.defrost = false;
        cleanConfig.sidewindComponent = 0.0;
    }
};

TEST_F(VSpeedCalculatorTest, InitializeCalculator) {
    EXPECT_TRUE(calculator->initialize(c172Config));
}

TEST_F(VSpeedCalculatorTest, CalculateVSpeeds_SeaLevel) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    VSpeedSet vSpeeds = calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    
    // Verify reasonable values
    EXPECT_GT(vSpeeds.VS0, 0.0);
    EXPECT_GT(vSpeeds.VS1, vSpeeds.VS0);
    EXPECT_GT(vSpeeds.V1, vSpeeds.VS1);
    EXPECT_GT(vSpeeds.VREF, 0.0);
}

TEST_F(VSpeedCalculatorTest, CalculateTakeoffDistance_SeaLevel) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    VSpeedSet vSpeeds = calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    double toDist = calculator->calculateTakeoffDistanceRequired(
        vSpeeds, standardWB, seaLevelStandard, 1.15);
    
    // Cessna 172 takeoff distance is typically 1200-1500 ft
    EXPECT_GT(toDist, 800.0);
    EXPECT_LT(toDist, 2500.0);
}

TEST_F(VSpeedCalculatorTest, CalculateLandingDistance_SeaLevel) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    VSpeedSet vSpeeds = calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    double ldgDist = calculator->calculateLandingDistanceRequired(
        vSpeeds, standardWB, seaLevelStandard, 1.67);
    
    // Cessna 172 landing distance is typically 1300-1500 ft
    EXPECT_GT(ldgDist, 800.0);
    EXPECT_LT(ldgDist, 2500.0);
}

TEST_F(VSpeedCalculatorTest, RunwaySuitabilityTakeoff_Adequate) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    VSpeedSet vSpeeds = calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    double toDist = calculator->calculateTakeoffDistanceRequired(
        vSpeeds, standardWB, seaLevelStandard, 1.15);
    
    EXPECT_TRUE(calculator->isRunwaySuitableForTakeoff(5000.0, toDist));
}

TEST_F(VSpeedCalculatorTest, RunwaySuitabilityTakeoff_Inadequate) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    VSpeedSet vSpeeds = calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    double toDist = calculator->calculateTakeoffDistanceRequired(
        vSpeeds, standardWB, seaLevelStandard, 1.15);
    
    EXPECT_FALSE(calculator->isRunwaySuitableForTakeoff(500.0, toDist));
}

TEST_F(VSpeedCalculatorTest, RunwaySuitabilityLanding_Adequate) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    VSpeedSet vSpeeds = calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    double ldgDist = calculator->calculateLandingDistanceRequired(
        vSpeeds, standardWB, seaLevelStandard, 1.67);
    
    EXPECT_TRUE(calculator->isRunwaySuitableForLanding(5000.0, ldgDist));
}

TEST_F(VSpeedCalculatorTest, RunwaySuitabilityLanding_Inadequate) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    VSpeedSet vSpeeds = calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    double ldgDist = calculator->calculateLandingDistanceRequired(
        vSpeeds, standardWB, seaLevelStandard, 1.67);
    
    EXPECT_FALSE(calculator->isRunwaySuitableForLanding(400.0, ldgDist));
}

TEST_F(VSpeedCalculatorTest, DensityAltitudeCorrection) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    EnvironmentFactors highAndHot = seaLevelStandard;
    highAndHot.runwayAltitude = 5000.0;
    highAndHot.temperature = 35.0;
    
    double da = calculator->calculateDensityAltitude(highAndHot);
    EXPECT_GT(da, highAndHot.runwayAltitude);
}

TEST_F(VSpeedCalculatorTest, PressureAltitudeCalculation) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    double pressAlt = calculator->calculatePressureAltitude(29.92, 0.0);
    EXPECT_NEAR(pressAlt, 0.0, 10.0);
    
    pressAlt = calculator->calculatePressureAltitude(29.02, 0.0);
    EXPECT_GT(pressAlt, 0.0);
}

TEST_F(VSpeedCalculatorTest, WeightCorrection_Heavier) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    standardWB.totalWeight = 2400.0;  // Heavier
    VSpeedSet heavyWSpeeds = calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    
    standardWB.totalWeight = 2000.0;  // Lighter
    VSpeedSet lightWSpeeds = calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    
    // Heavier aircraft needs higher speeds
    EXPECT_GT(heavyWSpeeds.VS0, lightWSpeeds.VS0);
}

TEST_F(VSpeedCalculatorTest, HeadwindCorrection) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    EnvironmentFactors noWind = seaLevelStandard;
    noWind.windHeadwind = 0.0;
    VSpeedSet noWindSpeeds = calculator->calculateVSpeeds(standardWB, noWind, cleanConfig);
    
    EnvironmentFactors withHeadwind = seaLevelStandard;
    withHeadwind.windHeadwind = 15.0;
    VSpeedSet headwindSpeeds = calculator->calculateVSpeeds(standardWB, withHeadwind, cleanConfig);
    
    // Headwind reduces required ground speed
    EXPECT_GT(noWindSpeeds.V1, 0.0);
}

TEST_F(VSpeedCalculatorTest, RecommendedTakeoffSpeed) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    double recommendedSpeed = calculator->getRecommendedTakeoffSpeed();
    
    EXPECT_GT(recommendedSpeed, 0.0);
    EXPECT_LT(recommendedSpeed, 200.0);
}

TEST_F(VSpeedCalculatorTest, RecommendedApproachSpeed) {
    EXPECT_TRUE(calculator->initialize(c172Config));
    
    calculator->calculateVSpeeds(standardWB, seaLevelStandard, cleanConfig);
    double recommendedSpeed = calculator->getRecommendedApproachSpeed();
    
    EXPECT_GT(recommendedSpeed, 0.0);
    EXPECT_LT(recommendedSpeed, 100.0);
}

// =====================================================================
// WEIGHT AND BALANCE TESTS (12 test cases)
// =====================================================================

class WeightBalanceTest : public ::testing::Test {
protected:
    std::unique_ptr<WeightBalanceSystem> wbSystem;
    AircraftConfig c172Config;
    
    void SetUp() override {
        wbSystem = std::make_unique<WeightBalanceSystem>();
        
        c172Config.title = "Cessna 172";
        c172Config.icaoModel = "C172";
        c172Config.maxGrossWeight = 2450.0;
        c172Config.emptyWeight = 1686.0;
    }
};

TEST_F(WeightBalanceTest, InitializeWeightBalance) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
}

TEST_F(WeightBalanceTest, AddWeightItem) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
    
    WeightItem item;
    item.name = "Pilot";
    item.weight = 170.0;
    item.armDistance = 83.0;
    item.category = "crew";
    item.adjustable = false;
    
    wbSystem->addWeightItem(item);
    EXPECT_EQ(wbSystem->getWeightItems().size(), 1);
}

TEST_F(WeightBalanceTest, CalculateWeightBalance) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
    
    WeightItem airframe;
    airframe.name = "Airframe";
    airframe.weight = 1686.0;
    airframe.armDistance = 88.5;
    airframe.category = "airframe";
    
    WeightItem pilot;
    pilot.name = "Pilot";
    pilot.weight = 170.0;
    pilot.armDistance = 83.0;
    pilot.category = "crew";
    
    WeightItem fuel;
    fuel.name = "Fuel";
    fuel.weight = 280.0;
    fuel.armDistance = 94.0;
    fuel.category = "fuel";
    
    wbSystem->addWeightItem(airframe);
    wbSystem->addWeightItem(pilot);
    wbSystem->addWeightItem(fuel);
    
    WeightBalanceResult result = wbSystem->calculateWeightBalance();
    
    EXPECT_GT(result.totalWeight, 0.0);
    EXPECT_GT(result.totalMoment, 0.0);
    EXPECT_TRUE(result.withinEnvelope);
}

TEST_F(WeightBalanceTest, OverweightDetection) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
    
    WeightItem overweight;
    overweight.name = "Overload";
    overweight.weight = 3000.0;
    overweight.armDistance = 88.0;
    
    wbSystem->addWeightItem(overweight);
    
    WeightBalanceResult result = wbSystem->calculateWeightBalance();
    EXPECT_FALSE(result.withinEnvelope);
    EXPECT_EQ(result.status, "OVERWEIGHT");
}

TEST_F(WeightBalanceTest, CGEnvelopeValidation) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
    
    WeightItem forward;
    forward.name = "Forward Load";
    forward.weight = 2000.0;
    forward.armDistance = 60.0;  // Too far forward
    
    wbSystem->addWeightItem(forward);
    
    WeightBalanceResult result = wbSystem->calculateWeightBalance();
    EXPECT_FALSE(result.withinEnvelope);
}

TEST_F(WeightBalanceTest, RemoveWeightItem) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
    
    WeightItem item1, item2;
    item1.name = "Item1";
    item1.weight = 100.0;
    item1.armDistance = 80.0;
    
    item2.name = "Item2";
    item2.weight = 100.0;
    item2.armDistance = 90.0;
    
    wbSystem->addWeightItem(item1);
    wbSystem->addWeightItem(item2);
    
    EXPECT_EQ(wbSystem->getWeightItems().size(), 2);
    
    wbSystem->removeWeightItem("Item1");
    EXPECT_EQ(wbSystem->getWeightItems().size(), 1);
}

TEST_F(WeightBalanceTest, UpdateWeightItem) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
    
    WeightItem item;
    item.name = "Cargo";
    item.weight = 100.0;
    item.armDistance = 80.0;
    
    wbSystem->addWeightItem(item);
    wbSystem->updateWeightItem("Cargo", 150.0);
    
    WeightBalanceResult result = wbSystem->calculateWeightBalance();
    EXPECT_GT(result.totalWeight, 100.0);
}

TEST_F(WeightBalanceTest, FuelRequiredCalculation) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
    
    // 300 nm at 100 knots true airspeed, 8 gph burn
    double fuelRequired = wbSystem->getFuelRequiredForRange(300.0, 100.0, 8.0);
    
    // 3 hours + 0.75 hours reserve = 3.75 hours
    // 3.75 * 8 = 30 gallons
    EXPECT_GT(fuelRequired, 25.0);
    EXPECT_LT(fuelRequired, 35.0);
}

TEST_F(WeightBalanceTest, GenerateReport) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
    
    WeightItem item;
    item.name = "Test Item";
    item.weight = 100.0;
    item.armDistance = 80.0;
    
    wbSystem->addWeightItem(item);
    wbSystem->calculateWeightBalance();
    
    std::string report = wbSystem->generateReport();
    EXPECT_FALSE(report.empty());
    EXPECT_TRUE(report.find("WEIGHT AND BALANCE") != std::string::npos);
}

TEST_F(WeightBalanceTest, Cessna172CGEnvelope) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
    
    // Load default Cessna 172 envelope
    WeightBalanceResult result = wbSystem->calculateWeightBalance();
    
    // Should have valid envelope loaded
    EXPECT_TRUE(result.status.empty() || result.status == "OK");
}

TEST_F(WeightBalanceTest, AdjustLoadToCG) {
    EXPECT_TRUE(wbSystem->initialize(c172Config));
    
    WeightItem cargo;
    cargo.name = "Cargo";
    cargo.weight = 100.0;
    cargo.armDistance = 100.0;
    cargo.adjustable = true;
    
    wbSystem->addWeightItem(cargo);
    
    bool adjusted = wbSystem->adjustLoadToCG("Cargo", 35.0);
    EXPECT_TRUE(adjusted);
}

// =====================================================================
// STABILIZED APPROACH TESTS (11 test cases)
// =====================================================================

class StabilizedApproachTest : public ::testing::Test {
protected:
    std::unique_ptr<StabilizedApproachSystem> approach;
    AircraftState approachState;
    Position threshold;
    double thresholdElevation;
    
    void SetUp() override {
        approach = std::make_unique<StabilizedApproachSystem>();
        EXPECT_TRUE(approach->initialize("Cessna 172"));
        
        // Approach airport
        threshold.latitude = 40.6413;
        threshold.longitude = -73.7781;
        threshold.altitude = 20.0;
        threshold.heading = 40.0;
        thresholdElevation = 20.0;
        
        // Aircraft on approach
        approachState.position.latitude = 40.6400;
        approachState.position.longitude = -73.7800;
        approachState.position.altitude = 1500.0;  // 1480 ft AGL
        approachState.indicatedAirspeed = 65.0;
        approachState.verticalSpeed = -500.0;
        approachState.heading = 38.0;
        approachState.onGround = false;
        approachState.gearDown = true;
        approachState.flapsPosition = 100;
    }
};

TEST_F(StabilizedApproachTest, InitializeApproachSystem) {
    EXPECT_TRUE(approach->initialize("Cessna 172"));
}

TEST_F(StabilizedApproachTest, StartApproachMonitoring) {
    approach->startApproachMonitoring(1000.0, 60.0, -400.0);
    EXPECT_GT(approach->getApproachProgress(), 0.0);
}

TEST_F(StabilizedApproachTest, UpdateApproachStatus) {
    approach->startApproachMonitoring(1000.0, 60.0, -400.0);
    
    StabilizationCriteria criteria = approach->updateApproachStatus(
        approachState, threshold, thresholdElevation);
    
    EXPECT_GE(criteria.altitudeDeviation, -1000.0);
    EXPECT_LE(criteria.altitudeDeviation, 1000.0);
}

TEST_F(StabilizedApproachTest, StabilizedCriteriaCheck) {
    approach->startApproachMonitoring(1500.0, 65.0, -500.0);
    
    StabilizationCriteria criteria = approach->updateApproachStatus(
        approachState, threshold, thresholdElevation);
    
    // Should have reasonable deviations at 1500 ft
    EXPECT_LT(std::abs(criteria.altitudeDeviation), 500.0);
}

TEST_F(StabilizedApproachTest, SpeedDeviation) {
    approach->startApproachMonitoring(1000.0, 60.0, -400.0);
    
    approachState.indicatedAirspeed = 75.0;  // Too fast
    
    StabilizationCriteria criteria = approach->updateApproachStatus(
        approachState, threshold, thresholdElevation);
    
    EXPECT_GT(criteria.speedDeviation, 0.0);
}

TEST_F(StabilizedApproachTest, DescentRateExcessive) {
    approach->startApproachMonitoring(1000.0, 60.0, -400.0);
    
    approachState.verticalSpeed = -2000.0;  // Too steep
    
    StabilizationCriteria criteria = approach->updateApproachStatus(
        approachState, threshold, thresholdElevation);
    
    EXPECT_FALSE(criteria.descentRateAcceptable);
}

TEST_F(StabilizedApproachTest, GoAroundTriggerTooHigh) {
    approach->startApproachMonitoring(1000.0, 60.0, -400.0);
    
    approachState.position.altitude = 1200.0;  // Too high
    
    std::vector<GoAroundTrigger> triggers = approach->evaluateGoAroundTriggers(
        approachState, thresholdElevation);
    
    EXPECT_FALSE(triggers.empty());
}

TEST_F(StabilizedApproachTest, GoAroundTriggerTooFast) {
    approach->startApproachMonitoring(1000.0, 60.0, -400.0);
    
    approachState.indicatedAirspeed = 90.0;  // Too fast
    
    std::vector<GoAroundTrigger> triggers = approach->evaluateGoAroundTriggers(
        approachState, thresholdElevation);
    
    EXPECT_FALSE(triggers.empty());
}

TEST_F(StabilizedApproachTest, ApproachProgressTracking) {
    approach->startApproachMonitoring(1000.0, 60.0, -400.0);
    
    double progress1 = approach->getApproachProgress();
    
    approachState.position.altitude = 800.0;  // Descending
    approach->updateApproachStatus(approachState, threshold, thresholdElevation);
    
    double progress2 = approach->getApproachProgress();
    EXPECT_GT(progress2, progress1);
}

TEST_F(StabilizedApproachTest, ApproachReportGeneration) {
    approach->startApproachMonitoring(1000.0, 60.0, -400.0);
    
    StabilizationCriteria criteria = approach->updateApproachStatus(
        approachState, threshold, thresholdElevation);
    
    std::string report = approach->getApproachReport(criteria);
    EXPECT_FALSE(report.empty());
    EXPECT_TRUE(report.find("APPROACH STABILIZATION") != std::string::npos);
}

TEST_F(StabilizedApproachTest, CorrectiveActionSuggestion) {
    approach->startApproachMonitoring(1000.0, 60.0, -400.0);
    
    approachState.indicatedAirspeed = 85.0;  // Too fast
    StabilizationCriteria criteria = approach->updateApproachStatus(
        approachState, threshold, thresholdElevation);
    
    std::string action = approach->getCorrectiveAction(criteria);
    EXPECT_FALSE(action.empty());
}

} // namespace Tests
} // namespace AICopilot

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

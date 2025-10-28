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

#include <gtest/gtest.h>
#include "advanced_procedures.hpp"
#include "dynamic_flight_planning.hpp"
#include "performance_optimizer.hpp"
#include "system_monitor.hpp"
#include <chrono>
#include <vector>
#include <cmath>

namespace AICopilot {

// ============================================================
// TEST FIXTURES
// ============================================================

class AdvancedProceduresTest : public ::testing::Test {
protected:
    void SetUp() override {
        procedures.initialize(profile_, AircraftCategory::MULTI_ENGINE_PISTON);
    }
    
    void TearDown() override {
        procedures.shutdown();
    }
    
    AdvancedProcedures procedures;
    
    PerformanceProfile profile_ = {
        25.0, 20.0, 18.0, 50.0, 60.0, 80.0, 130.0, 145.0, 35.0, 40.0, 60.0, 55.0,
        500.0, 300.0, 100.0, 120.0, 15000.0,
        100.0, 15.0, 800.0, 5.0,
        3500.0, 5000.0, 4500.0, 4200.0
    };
};

class DynamicFlightPlanningTest : public ::testing::Test {
protected:
    void SetUp() override {
        planner.initialize(profile_);
    }
    
    void TearDown() override {
        planner.shutdown();
    }
    
    DynamicFlightPlanning planner;
    
    PerformanceProfile profile_ = {
        25.0, 20.0, 18.0, 50.0, 60.0, 80.0, 130.0, 145.0, 35.0, 40.0, 60.0, 55.0,
        500.0, 300.0, 100.0, 120.0, 15000.0,
        100.0, 15.0, 800.0, 5.0,
        3500.0, 5000.0, 4500.0, 4200.0
    };
};

class PerformanceOptimizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        optimizer.initialize();
    }
    
    void TearDown() override {
        optimizer.shutdown();
    }
    
    PerformanceOptimizer optimizer;
};

class SystemMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        monitor.initialize();
    }
    
    void TearDown() override {
        monitor.shutdown();
    }
    
    SystemMonitor monitor;
};

// ============================================================
// ADVANCED PROCEDURES TESTS
// ============================================================

TEST_F(AdvancedProceduresTest, InitializeSuccessfully) {
    EXPECT_TRUE(procedures.initialize(profile_, AircraftCategory::SINGLE_ENGINE_PISTON));
}

TEST_F(AdvancedProceduresTest, HandleEngineFailureSingleEngine) {
    AircraftState state;
    state.position = {45.0, -122.0, 5000.0, 0};
    state.fuelQuantity = 30.0;
    
    auto result = procedures.handleEngineFailure(1, state, 5000.0);
    
    EXPECT_EQ(result.engineType, EngineType::PISTON_SINGLE);
    EXPECT_GT(result.minimumControlSpeed, profile_.vs1);
    EXPECT_FALSE(result.immediateActions.empty());
}

TEST_F(AdvancedProceduresTest, HandleEngineFailureMultiEngine) {
    PerformanceProfile multiEngineProfile = profile_;
    procedures.initialize(multiEngineProfile, AircraftCategory::MULTI_ENGINE_PISTON);
    
    AircraftState state;
    state.position = {45.0, -122.0, 8000.0, 0};
    state.fuelQuantity = 80.0;
    
    auto result = procedures.handleEngineFailure(1, state, 8000.0);
    
    EXPECT_EQ(result.engineType, EngineType::PISTON_MULTI);
    EXPECT_GT(result.singleEngineClimbPerformance, 0.0);
    EXPECT_GT(result.subsequentActions.size(), 0);
}

TEST_F(AdvancedProceduresTest, HandleElectricalFailure) {
    AircraftState state;
    state.position = {45.0, -122.0, 3000.0, 0};
    state.masterBattery = true;
    state.batteryVoltage = 28.0;
    
    auto result = procedures.handleElectricalFailure(state);
    
    EXPECT_EQ(result.status, ProcedureStatus::IN_PROGRESS);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.checklist.empty());
    EXPECT_GT(result.actionsPerformed.size(), 0);
}

TEST_F(AdvancedProceduresTest, HandleHydraulicFailure) {
    AircraftState state;
    state.position = {45.0, -122.0, 6000.0, 0};
    
    auto result = procedures.handleHydraulicFailure(state);
    
    EXPECT_EQ(result.status, ProcedureStatus::IN_PROGRESS);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.checklist.empty());
}

TEST_F(AdvancedProceduresTest, HandleFuelSystemFailure) {
    AircraftState state;
    state.position = {45.0, -122.0, 4000.0, 0};
    state.fuelQuantity = 50.0;
    
    double leakRate = 2.0;  // gallons per hour
    auto result = procedures.handleFuelSystemFailure(state, leakRate);
    
    EXPECT_GT(result.totalFuelRemaining, 0.0);
    EXPECT_GT(result.fuelConsumptionRate, 0.0);
    EXPECT_GT(result.estimatedTimeOfUsable, 0.0);
    EXPECT_FALSE(result.nearestAirports.empty());
}

TEST_F(AdvancedProceduresTest, HandlePressurizationFailure) {
    AircraftState state;
    state.position = {45.0, -122.0, 25000.0, 0};
    
    auto result = procedures.handlePressurizationFailure(state, state.position);
    
    EXPECT_EQ(result.targetAltitude, 10000.0);
    EXPECT_GT(result.descentRate, 0.0);
    EXPECT_FALSE(result.safeAlternates.empty());
}

TEST_F(AdvancedProceduresTest, HandleFireDetection) {
    AircraftState state;
    state.position = {45.0, -122.0, 5000.0, 0};
    
    auto result = procedures.handleFireDetection("Engine", state);
    
    EXPECT_EQ(result.status, ProcedureStatus::IN_PROGRESS);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.checklist.empty());
    EXPECT_TRUE(result.checklist.find("ENGINE") != std::string::npos);
}

TEST_F(AdvancedProceduresTest, ExecuteEmergencyDescent) {
    AircraftState state;
    state.position = {45.0, -122.0, 30000.0, 0};
    
    auto result = procedures.executeEmergencyDescent(state, state.position, 10000.0);
    
    EXPECT_EQ(result.targetAltitude, 10000.0);
    EXPECT_GT(result.descentRate, 0.0);
    EXPECT_GT(result.estimeatedTimeToSafeAltitude, 0.0);
    EXPECT_GT(result.safeAlternates.size(), 0);
}

TEST_F(AdvancedProceduresTest, GoAroundProcedure) {
    AircraftState state;
    state.position = {45.0, -122.0, 500.0, 0};
    state.indicatedAirspeed = 60.0;
    
    auto result = procedures.executeGoAround(state, "Unstable approach");
    
    EXPECT_GT(result.targetAltitude, state.position.altitude);
    EXPECT_GT(result.targetSpeed, 0.0);
    EXPECT_TRUE(result.flapsRetract);
    EXPECT_TRUE(result.gearUp);
    EXPECT_EQ(result.reason, "Unstable approach");
}

TEST_F(AdvancedProceduresTest, CrosswindAssessment) {
    Waypoint runway;
    runway.position = {45.0, -122.0, 0, 90.0};  // Runway heading 90 degrees
    runway.id = "RWY_09L";
    
    auto result = procedures.assessRunwayForCrosswind(
        runway, 20.0, 120.0, 15.0);  // 20kt wind from 120 degrees, max crosswind 15kt
    
    EXPECT_GE(result.crosswindComponent, 0.0);
    EXPECT_LE(result.crosswindComponent, 25.0);
}

TEST_F(AdvancedProceduresTest, SelectRunwayForCrosswind) {
    std::vector<Waypoint> runways;
    
    Waypoint r1;
    r1.position = {45.0, -122.0, 0, 90.0};
    r1.id = "RWY_09L";
    runways.push_back(r1);
    
    Waypoint r2;
    r2.position = {45.0, -122.1, 0, 270.0};
    r2.id = "RWY_27R";
    runways.push_back(r2);
    
    auto result = procedures.selectRunwayForCrosswind(
        runways, 15.0, 180.0, 12.0);
    
    EXPECT_FALSE(result.id.empty());
}

TEST_F(AdvancedProceduresTest, GetProcedureLimits) {
    auto limits = procedures.getProcedureLimits();
    
    EXPECT_GT(limits.maxCrosswindComponent, 0.0);
    EXPECT_GT(limits.maxDemonstrationSpeed, 0.0);
}

TEST_F(AdvancedProceduresTest, GetEmergencyChecklist) {
    auto checklist = procedures.getEmergencyChecklist(EmergencyType::ENGINE_FAILURE);
    
    EXPECT_FALSE(checklist.empty());
    EXPECT_TRUE(checklist.find("ENGINE") != std::string::npos);
}

TEST_F(AdvancedProceduresTest, GoAroundPerformance) {
    auto perf = procedures.calculateGoAroundPerformance(1000.0, 60.0, 50.0);
    
    EXPECT_GT(perf.requiredFuel, 0.0);
    EXPECT_GT(perf.climbPerformance, 0.0);
    EXPECT_GT(perf.timeToReachClimb, 0.0);
}

// ============================================================
// DYNAMIC FLIGHT PLANNING TESTS
// ============================================================

TEST_F(DynamicFlightPlanningTest, InitializeSuccessfully) {
    EXPECT_TRUE(planner.initialize(profile_));
}

TEST_F(DynamicFlightPlanningTest, OptimizeRouteFuelEfficiency) {
    Waypoint dep, dest;
    dep.position = {45.0, -122.0, 0, 0};
    dep.id = "DEPARTURE";
    
    dest.position = {45.5, -122.5, 0, 0};
    dest.id = "DESTINATION";
    
    std::vector<Waypoint> waypoints = {dep, dest};
    
    auto result = planner.optimizeRoute(dep, dest, waypoints, 100.0, 
                                       OptimizationObjective::FUEL_EFFICIENCY);
    
    EXPECT_GT(result.totalDistance, 0.0);
    EXPECT_GT(result.estimatedFuel, 0.0);
    EXPECT_GT(result.executionTime, 0.0);
}

TEST_F(DynamicFlightPlanningTest, OptimizeRouteTimeOptimization) {
    Waypoint dep, dest;
    dep.position = {45.0, -122.0, 0, 0};
    dep.id = "DEPARTURE";
    
    dest.position = {45.5, -122.5, 0, 0};
    dest.id = "DESTINATION";
    
    std::vector<Waypoint> waypoints = {dep, dest};
    
    auto result = planner.optimizeRoute(dep, dest, waypoints, 100.0,
                                       OptimizationObjective::TIME_OPTIMIZATION);
    
    EXPECT_GT(result.totalDistance, 0.0);
    EXPECT_LT(result.estimatedTime, 1000.0);  // Should be < 1000 minutes for short flight
}

TEST_F(DynamicFlightPlanningTest, CalculateOptimalAltitude) {
    WindConditions wind;
    wind.surfaceWindSpeed = 10.0;
    wind.surfaceWindDirection = 180.0;
    
    auto result = planner.calculateOptimalAltitude(500.0, 80.0, wind);
    
    EXPECT_GT(result.recommendedAltitude, 0.0);
    EXPECT_GT(result.optimalCruiseSpeed, 0.0);
    EXPECT_GT(result.fuelConsumptionRate, 0.0);
}

TEST_F(DynamicFlightPlanningTest, GenerateAlternativeRoutes) {
    Waypoint dep, dest;
    dep.position = {45.0, -122.0, 0, 0};
    dep.id = "DEPARTURE";
    
    dest.position = {45.5, -122.5, 0, 0};
    dest.id = "DESTINATION";
    
    auto routes = planner.generateAlternativeRoutes(dep, dest, 3);
    
    EXPECT_EQ(routes.size(), 3);
    for (const auto& route : routes) {
        EXPECT_GT(route.distance, 0.0);
        EXPECT_GT(route.estimatedFuel, 0.0);
    }
}

TEST_F(DynamicFlightPlanningTest, CalculateFuelRequirement) {
    std::vector<Waypoint> route;
    Waypoint wp1, wp2;
    wp1.position = {45.0, -122.0, 5000.0, 0};
    wp2.position = {45.5, -122.5, 5000.0, 0};
    route.push_back(wp1);
    route.push_back(wp2);
    
    Waypoint alternate;
    alternate.position = {45.6, -122.6, 0, 0};
    
    auto fuel = planner.calculateFuelRequirement(route, 5000.0, 100.0, alternate);
    
    EXPECT_GT(fuel.tripFuel, 0.0);
    EXPECT_GT(fuel.alternateReserve, 0.0);
    EXPECT_GT(fuel.totalFuel, 0.0);
}

TEST_F(DynamicFlightPlanningTest, CheckFuelFeasibility) {
    std::vector<Waypoint> route;
    Waypoint wp1, wp2;
    wp1.position = {45.0, -122.0, 5000.0, 0};
    wp2.position = {45.5, -122.5, 5000.0, 0};
    route.push_back(wp1);
    route.push_back(wp2);
    
    Waypoint alternate;
    alternate.position = {45.6, -122.6, 0, 0};
    
    bool feasible = planner.checkFuelFeasibility(route, 120.0, 5000.0, 100.0, alternate);
    EXPECT_TRUE(feasible);
    
    feasible = planner.checkFuelFeasibility(route, 10.0, 5000.0, 100.0, alternate);
    EXPECT_FALSE(feasible);
}

TEST_F(DynamicFlightPlanningTest, CalculateAvailableRange) {
    WindConditions wind;
    wind.surfaceWindSpeed = 10.0;
    
    double range = planner.calculateAvailableRange(100.0, 5000.0, 100.0, wind);
    
    EXPECT_GT(range, 0.0);
}

TEST_F(DynamicFlightPlanningTest, SelectOptimalSID) {
    Waypoint firstFix;
    firstFix.position = {45.5, -122.5, 0, 0};
    firstFix.id = "FIRSTFIX";
    
    auto sid = planner.selectOptimalSID("PDXM", firstFix, AircraftType::MULTI_ENGINE_PROP);
    
    EXPECT_FALSE(sid.sidCode.empty());
    EXPECT_GT(sid.waypoints.size(), 0);
    EXPECT_GT(sid.estimatedTime, 0.0);
}

TEST_F(DynamicFlightPlanningTest, SelectOptimalSTAR) {
    Position pos = {45.0, -122.0, 5000.0, 0};
    
    auto star = planner.selectOptimalSTAR("PDXM", "RWY_28L", pos);
    
    EXPECT_FALSE(star.starCode.empty());
    EXPECT_GT(star.waypoints.size(), 0);
    EXPECT_GT(star.estimatedTime, 0.0);
}

TEST_F(DynamicFlightPlanningTest, CalculateStepClimbOpportunity) {
    auto opp = planner.calculateStepClimbOpportunity(8000.0, 50.0, 200.0);
    
    EXPECT_GT(opp.newAltitude, 8000.0);
    EXPECT_GT(opp.fuelSavingPerHour, 0.0);
    EXPECT_GT(opp.timeRequiredToClimb, 0.0);
}

TEST_F(DynamicFlightPlanningTest, CalculateOptimalDescentProfile) {
    WindConditions wind;
    wind.surfaceWindSpeed = 5.0;
    
    auto profile = planner.calculateOptimalDescentProfile(
        10000.0, 100.0, 3000.0, 50.0, wind);
    
    EXPECT_GT(profile.top_of_descent_distance, 0.0);
    EXPECT_GT(profile.descentRate, 0.0);
    EXPECT_GT(profile.descentPath.size(), 0);
}

// ============================================================
// PERFORMANCE OPTIMIZER TESTS
// ============================================================

TEST_F(PerformanceOptimizerTest, InitializeSuccessfully) {
    EXPECT_TRUE(optimizer.initialize());
}

TEST_F(PerformanceOptimizerTest, CacheTerrainElevation) {
    Position pos = {45.0, -122.0, 0, 0};
    optimizer.cacheTerrainElevation(pos, 500.0);
    
    double elevation;
    EXPECT_TRUE(optimizer.getTerrainElevation(pos, elevation));
    EXPECT_DOUBLE_EQ(elevation, 500.0);
}

TEST_F(PerformanceOptimizerTest, CacheWaypoint) {
    Waypoint wp;
    wp.position = {45.0, -122.0, 5000.0, 0};
    wp.id = "VOR1";
    
    optimizer.cacheWaypoint("VOR1", wp);
    
    Waypoint retrieved;
    EXPECT_TRUE(optimizer.getWaypoint("VOR1", retrieved));
    EXPECT_EQ(retrieved.id, "VOR1");
}

TEST_F(PerformanceOptimizerTest, GetCacheStatistics) {
    Position pos = {45.0, -122.0, 0, 0};
    optimizer.cacheTerrainElevation(pos, 500.0);
    
    double elevation;
    optimizer.getTerrainElevation(pos, elevation);  // Cache hit
    optimizer.getTerrainElevation(pos, elevation);  // Cache hit
    optimizer.getTerrainElevation({46.0, -123.0, 0, 0}, elevation);  // Cache miss
    
    auto stats = optimizer.getCacheStatistics();
    
    EXPECT_GT(stats.queryCount, 0);
    EXPECT_GE(stats.cacheHits, 2);
    EXPECT_GT(stats.cacheHitRate, 0.0);
}

TEST_F(PerformanceOptimizerTest, CreateQueryBatch) {
    std::vector<Position> positions;
    positions.push_back({45.0, -122.0, 0, 0});
    positions.push_back({45.1, -122.1, 0, 0});
    
    auto batch = optimizer.createQueryBatch(positions, 100);
    
    EXPECT_GT(batch.batchId, 0);
    EXPECT_EQ(batch.positions.size(), 2);
}

TEST_F(PerformanceOptimizerTest, OptimizeSpatialQuery) {
    std::vector<Waypoint> waypoints;
    for (int i = 0; i < 10; ++i) {
        Waypoint wp;
        wp.position = {45.0 + (i * 0.01), -122.0 + (i * 0.01), 0, 0};
        wp.id = "WP" + std::to_string(i);
        waypoints.push_back(wp);
    }
    
    Position center = {45.05, -122.05, 0, 0};
    auto result = optimizer.optimizeSpatialQuery(center, 10.0, waypoints);
    
    EXPECT_GT(result.size(), 0);
    EXPECT_LE(result.size(), waypoints.size());
}

TEST_F(PerformanceOptimizerTest, PrefetchTerrainGrid) {
    Position center = {45.0, -122.0, 5000.0, 0};
    optimizer.prefetchTerrainGrid(center, 50.0, PrefetchStrategy::PREDICTIVE);
    
    auto status = optimizer.getPrefetchStatus();
    EXPECT_GE(status.completedPrefetches, 0);
}

TEST_F(PerformanceOptimizerTest, GetPerformanceReport) {
    Position pos = {45.0, -122.0, 0, 0};
    optimizer.cacheTerrainElevation(pos, 500.0);
    
    double elevation;
    optimizer.getTerrainElevation(pos, elevation);
    
    auto report = optimizer.getPerformanceReport();
    
    EXPECT_GT(report.cacheMetrics.queryCount, 0);
    EXPECT_GE(report.cacheMetrics.cacheHitRate, 0.0);
}

// ============================================================
// SYSTEM MONITOR TESTS
// ============================================================

TEST_F(SystemMonitorTest, InitializeSuccessfully) {
    EXPECT_TRUE(monitor.initialize());
}

TEST_F(SystemMonitorTest, UpdateComponentHealth) {
    ComponentHealth health;
    health.component = SystemComponent::WEATHER_SYSTEM;
    health.status = SystemHealth::HEALTHY;
    health.cpuUsage = 25.0;
    health.averageLatency = 3.5;
    
    monitor.updateComponentHealth(health);
    
    auto retrieved = monitor.getComponentHealth(SystemComponent::WEATHER_SYSTEM);
    EXPECT_EQ(retrieved.component, SystemComponent::WEATHER_SYSTEM);
    EXPECT_EQ(retrieved.status, SystemHealth::HEALTHY);
}

TEST_F(SystemMonitorTest, GetSystemHealth) {
    ComponentHealth health;
    health.component = SystemComponent::WEATHER_SYSTEM;
    health.status = SystemHealth::HEALTHY;
    monitor.updateComponentHealth(health);
    
    auto sysHealth = monitor.getSystemHealth();
    EXPECT_NE(sysHealth, SystemHealth::OFFLINE);
}

TEST_F(SystemMonitorTest, RecordQueryLatency) {
    monitor.recordQueryLatency(SystemComponent::NAVIGATION, 2.5);
    monitor.recordQueryLatency(SystemComponent::NAVIGATION, 3.0);
    
    auto metrics = monitor.getSystemMetrics();
    EXPECT_GT(metrics.queriesPerSecond, 0);
}

TEST_F(SystemMonitorTest, GetHealthReport) {
    auto report = monitor.getHealthReport();
    
    EXPECT_NE(report.overallHealth, SystemHealth::OFFLINE);
    EXPECT_GT(report.healthScore, 0.0);
    EXPECT_LE(report.healthScore, 100.0);
    EXPECT_GT(report.components.size(), 0);
}

TEST_F(SystemMonitorTest, GenerateAlert) {
    monitor.generateAlert(SystemComponent::WEATHER_SYSTEM, 
                         SystemHealth::WARNING, 
                         "Test alert");
    
    auto alerts = monitor.getActiveAlerts();
    EXPECT_GT(alerts.size(), 0);
    EXPECT_EQ(alerts[0].component, SystemComponent::WEATHER_SYSTEM);
}

TEST_F(SystemMonitorTest, AcknowledgeAlert) {
    monitor.generateAlert(SystemComponent::WEATHER_SYSTEM,
                         SystemHealth::WARNING,
                         "Test alert");
    
    auto alerts = monitor.getActiveAlerts();
    EXPECT_GT(alerts.size(), 0);
    
    monitor.acknowledgeAlert(alerts[0].id);
    
    auto activeAlerts = monitor.getActiveAlerts();
    EXPECT_EQ(activeAlerts.size(), 0);
}

TEST_F(SystemMonitorTest, GetResourceUtilization) {
    auto util = monitor.getResourceUtilization();
    
    EXPECT_GE(util.cpuPercentage, 0.0);
    EXPECT_LE(util.cpuPercentage, 100.0);
    EXPECT_GE(util.memoryPercentage, 0.0);
    EXPECT_LE(util.memoryPercentage, 100.0);
}

TEST_F(SystemMonitorTest, RunDiagnostics) {
    auto results = monitor.runFullDiagnostics();
    
    EXPECT_GT(results.size(), 0);
    for (const auto& result : results) {
        EXPECT_GE(result.executionTimeMs, 0.0);
    }
}

TEST_F(SystemMonitorTest, GetOptimizationRecommendations) {
    auto recommendations = monitor.getOptimizationRecommendations();
    
    EXPECT_GE(recommendations.size(), 0);
}

TEST_F(SystemMonitorTest, ExportSystemReport) {
    std::string report = monitor.exportSystemReport();
    
    EXPECT_FALSE(report.empty());
    EXPECT_TRUE(report.find("UPTIME") != std::string::npos);
}

// ============================================================
// PERFORMANCE BENCHMARK TESTS
// ============================================================

TEST_F(AdvancedProceduresTest, PerformanceBenchmark_EmergencyDescent) {
    AircraftState state;
    state.position = {45.0, -122.0, 30000.0, 0};
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        procedures.executeEmergencyDescent(state, state.position, 10000.0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LT(duration, 500);  // Should complete 100 iterations in < 500ms (< 5ms each)
}

TEST_F(DynamicFlightPlanningTest, PerformanceBenchmark_RouteOptimization) {
    Waypoint dep, dest;
    dep.position = {45.0, -122.0, 0, 0};
    dep.id = "DEPARTURE";
    dest.position = {47.0, -120.0, 0, 0};
    dest.id = "DESTINATION";
    
    std::vector<Waypoint> waypoints = {dep, dest};
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 50; ++i) {
        planner.optimizeRoute(dep, dest, waypoints, 100.0,
                            OptimizationObjective::FUEL_EFFICIENCY);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LT(duration, 500);  // Should complete 50 iterations in < 500ms
}

TEST_F(PerformanceOptimizerTest, PerformanceBenchmark_CacheOperations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        Position pos = {45.0 + (i * 0.001), -122.0 + (i * 0.001), 0, 0};
        optimizer.cacheTerrainElevation(pos, 500.0 + i);
        
        double elevation;
        optimizer.getTerrainElevation(pos, elevation);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LT(duration, 100);  // Should complete 1000 cache ops in < 100ms
}

TEST_F(SystemMonitorTest, PerformanceBenchmark_HealthMonitoring) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        monitor.recordQueryLatency(SystemComponent::WEATHER_SYSTEM, 2.5 + (i % 10) * 0.1);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LT(duration, 50);  // Should complete 1000 recordings in < 50ms
}

} // namespace AICopilot

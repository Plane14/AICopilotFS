/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Integration Testing Framework Implementation
* Core functionality for cross-system testing infrastructure
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "integration_framework.hpp"
#include <iostream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <cmath>

namespace AICopilot {
namespace Tests {

IntegrationTestBase::IntegrationTestBase()
    : threadPool_(std::make_unique<ThreadPool>(8)) {
}

IntegrationTestBase::~IntegrationTestBase() = default;

void IntegrationTestBase::SetUp() {
    // Initialize weather database
    weatherDb_ = std::make_shared<WeatherDatabase>();
    weatherDb_->initialize();
    
    // Initialize runway selector
    runwaySelector_ = std::make_shared<RunwaySelector>();
    
    // Initialize terrain database with mock data path
    terrainDb_ = std::make_shared<TerrainDatabase>();
    // Note: Initialize with test data or mock as needed
    
    // Initialize navigation database
    navDb_ = std::make_shared<NavigationDatabase>();
    
    // Clear performance tracking
    performanceMetrics_.clear();
    dataFlows_.clear();
}

void IntegrationTestBase::TearDown() {
    // Validate cache coherency
    if (!validateCacheCoherency()) {
        std::cerr << "WARNING: Cache coherency validation failed\n";
    }
    
    // Print performance summary
    printPerformanceSummary();
    
    // Clean up systems
    if (weatherDb_) {
        weatherDb_->shutdown();
    }
    if (terrainDb_) {
        terrainDb_->shutdown();
    }
}

PerformanceMetrics IntegrationTestBase::measurePerformance(
    const std::string& testName,
    std::function<void()> operation,
    double expectedMaxMs) {
    
    PerformanceMetrics metrics;
    metrics.testName = testName;
    metrics.operationCount = 1;
    
    auto start = std::chrono::high_resolution_clock::now();
    operation();
    auto end = std::chrono::high_resolution_clock::now();
    
    double executionTime = std::chrono::duration<double, std::milli>(end - start).count();
    metrics.executionTimeMs = executionTime;
    metrics.minLatencyMs = executionTime;
    metrics.maxLatencyMs = executionTime;
    metrics.averageLatencyMs = executionTime;
    metrics.latencySamples.push_back(executionTime);
    metrics.passedThreshold = executionTime <= expectedMaxMs;
    
    performanceMetrics_.push_back(metrics);
    return metrics;
}

PerformanceMetrics IntegrationTestBase::measurePerformanceIterations(
    const std::string& testName,
    std::function<void()> operation,
    int iterations,
    double expectedMaxMs) {
    
    PerformanceMetrics metrics;
    metrics.testName = testName;
    metrics.operationCount = iterations;
    
    std::vector<double> samples;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto opStart = std::chrono::high_resolution_clock::now();
        operation();
        auto opEnd = std::chrono::high_resolution_clock::now();
        
        double latency = std::chrono::duration<double, std::milli>(opEnd - opStart).count();
        samples.push_back(latency);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    metrics.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    metrics.latencySamples = samples;
    
    metrics.minLatencyMs = *std::min_element(samples.begin(), samples.end());
    metrics.maxLatencyMs = *std::max_element(samples.begin(), samples.end());
    metrics.averageLatencyMs = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
    metrics.passedThreshold = metrics.averageLatencyMs <= expectedMaxMs;
    
    performanceMetrics_.push_back(metrics);
    return metrics;
}

void IntegrationTestBase::recordDataFlow(
    const std::string& source,
    const std::string& target,
    const std::string& dataType,
    bool validated) {
    
    DataFlowRecord flow;
    flow.sourceSystem = source;
    flow.targetSystem = target;
    flow.dataType = dataType;
    flow.timestampMs = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
    flow.validated = validated;
    
    dataFlows_.push_back(flow);
}

bool IntegrationTestBase::validateDataFlow(
    const std::string& source,
    const std::string& target) {
    
    for (const auto& flow : dataFlows_) {
        if (flow.sourceSystem == source && flow.targetSystem == target) {
            return flow.validated;
        }
    }
    return false;
}

std::vector<DataFlowRecord> IntegrationTestBase::getDataFlowsFrom(
    const std::string& system) {
    
    std::vector<DataFlowRecord> results;
    for (const auto& flow : dataFlows_) {
        if (flow.sourceSystem == system) {
            results.push_back(flow);
        }
    }
    return results;
}

PerformanceMetrics IntegrationTestBase::stressTestConcurrentAccess(
    const std::string& systemName,
    int operationCount,
    std::function<void()> operation) {
    
    PerformanceMetrics metrics;
    metrics.testName = systemName + "_stress_test";
    metrics.operationCount = operationCount;
    
    std::vector<double> latencies;
    std::mutex latenciesMutex;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < operationCount; ++i) {
        threadPool_->enqueue([this, &operation, &latencies, &latenciesMutex]() {
            auto opStart = std::chrono::high_resolution_clock::now();
            operation();
            auto opEnd = std::chrono::high_resolution_clock::now();
            
            double latency = std::chrono::duration<double, std::milli>(opEnd - opStart).count();
            {
                std::unique_lock<std::mutex> lock(latenciesMutex);
                latencies.push_back(latency);
            }
        });
    }
    
    threadPool_->waitAll();
    
    auto end = std::chrono::high_resolution_clock::now();
    metrics.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    metrics.latencySamples = latencies;
    
    if (!latencies.empty()) {
        metrics.minLatencyMs = *std::min_element(latencies.begin(), latencies.end());
        metrics.maxLatencyMs = *std::max_element(latencies.begin(), latencies.end());
        metrics.averageLatencyMs = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    }
    
    metrics.passedThreshold = metrics.averageLatencyMs <= 50.0;
    performanceMetrics_.push_back(metrics);
    
    return metrics;
}

bool IntegrationTestBase::validateCacheCoherency() {
    // Validate that all system caches are consistent
    // This is a placeholder for actual cache validation logic
    // In production, this would check:
    // 1. Weather cache consistency
    // 2. Terrain tile cache consistency
    // 3. Navigation database cache consistency
    // 4. Cross-system cache dependencies
    
    if (weatherDb_) {
        int cacheSize = weatherDb_->getCacheSize();
        if (cacheSize > WeatherDatabase::METAR_CACHE_SIZE) {
            return false;
        }
    }
    
    if (terrainDb_) {
        auto [tilesInCache, cacheSizeMb] = terrainDb_->getCacheStats();
        if (tilesInCache > TerrainDatabase::MAX_CACHE_TILES) {
            return false;
        }
    }
    
    return true;
}

void IntegrationTestBase::printPerformanceSummary() {
    if (performanceMetrics_.empty()) return;
    
    std::cout << "\n========== PERFORMANCE SUMMARY ==========\n";
    std::cout << std::left << std::setw(40) << "Test Name"
              << std::right << std::setw(12) << "Exec (ms)"
              << std::setw(12) << "Min (ms)"
              << std::setw(12) << "Avg (ms)"
              << std::setw(12) << "Max (ms)"
              << std::setw(10) << "Status\n";
    std::cout << std::string(98, '-') << "\n";
    
    for (const auto& metrics : performanceMetrics_) {
        std::string status = metrics.passedThreshold ? "PASS" : "FAIL";
        std::cout << std::left << std::setw(40) << metrics.testName.substr(0, 40)
                  << std::right << std::setw(12) << std::fixed << std::setprecision(2) << metrics.executionTimeMs
                  << std::setw(12) << metrics.minLatencyMs
                  << std::setw(12) << metrics.averageLatencyMs
                  << std::setw(12) << metrics.maxLatencyMs
                  << std::setw(10) << status << "\n";
    }
    
    std::cout << "\nData Flows Recorded: " << dataFlows_.size() << "\n";
    std::cout << "=========================================\n";
}

double IntegrationTestBase::getAveragePerformance() const {
    if (performanceMetrics_.empty()) return 0.0;
    
    double sum = 0.0;
    for (const auto& metrics : performanceMetrics_) {
        sum += metrics.executionTimeMs;
    }
    return sum / performanceMetrics_.size();
}

bool IntegrationTestBase::verifyPerformanceThreshold() {
    for (const auto& metrics : performanceMetrics_) {
        if (!metrics.passedThreshold) {
            return false;
        }
    }
    return true;
}

} // namespace Tests
} // namespace AICopilot

/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Integration Testing Framework - Multi-System Test Harness
* Provides production-ready integration testing infrastructure for Phase 2 systems
*
* Features:
* - Base test harness for cross-system testing
* - Performance measurement utilities with sub-millisecond precision
* - Thread pool for concurrent system validation
* - Data flow tracking between systems
* - Performance benchmarking with latency analysis
* - Cache coherency validation
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef INTEGRATION_FRAMEWORK_HPP
#define INTEGRATION_FRAMEWORK_HPP

#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <map>
#include <atomic>
#include "../include/weather_database.hpp"
#include "../include/runway_selector.hpp"
#include "../include/terrain_database.hpp"
#include "../include/navdata_database.hpp"

namespace AICopilot {
namespace Tests {

/**
 * Performance measurement result
 * Tracks execution time and latency statistics
 */
struct PerformanceMetrics {
    std::string testName;
    double executionTimeMs;      // Total execution time in milliseconds
    double minLatencyMs;          // Minimum latency observed
    double maxLatencyMs;          // Maximum latency observed
    double averageLatencyMs;      // Average latency
    int operationCount;           // Number of operations executed
    bool passedThreshold;         // true if execution time < 50ms
    std::vector<double> latencySamples;  // Individual sample latencies
    
    // Statistics
    double getStandardDeviation() const {
        if (latencySamples.size() < 2) return 0.0;
        double mean = averageLatencyMs;
        double variance = 0.0;
        for (double sample : latencySamples) {
            variance += (sample - mean) * (sample - mean);
        }
        variance /= latencySamples.size();
        return std::sqrt(variance);
    }
    
    double getPercentile(double p) const {
        if (latencySamples.empty()) return 0.0;
        std::vector<double> sorted = latencySamples;
        std::sort(sorted.begin(), sorted.end());
        size_t index = static_cast<size_t>(sorted.size() * p / 100.0);
        if (index >= sorted.size()) index = sorted.size() - 1;
        return sorted[index];
    }
};

/**
 * Cross-system data flow tracker
 * Validates data propagation between systems
 */
struct DataFlowRecord {
    std::string sourceSystem;     // System that originated data
    std::string targetSystem;     // System that received data
    std::string dataType;         // Type of data transferred
    long long timestampMs;        // When transfer occurred
    bool validated;               // Whether data was validated
    std::string validationDetails; // Details if validation failed
};

/**
 * Thread pool for concurrent testing
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads) : shutdown_(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    condition_.wait(lock, [this] { return !taskQueue_.empty() || shutdown_; });
                    
                    if (shutdown_ && taskQueue_.empty()) break;
                    
                    if (!taskQueue_.empty()) {
                        auto task = std::move(taskQueue_.front());
                        taskQueue_.pop();
                        lock.unlock();
                        task();
                    }
                }
            });
        }
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            shutdown_ = true;
        }
        condition_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }
    
    template <class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            taskQueue_.emplace(std::forward<F>(f));
        }
        condition_.notify_one();
    }
    
    void waitAll() {
        while (true) {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (taskQueue_.empty()) break;
            std::this_thread::yield();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> taskQueue_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> shutdown_;
};

/**
 * Base Integration Test Fixture
 * Provides common setup/teardown and utilities for all integration tests
 */
class IntegrationTestBase : public ::testing::Test {
public:
    IntegrationTestBase();
    ~IntegrationTestBase() override;
    
    /**
     * Set up test fixtures - called before each test
     * Initializes all Phase 2 systems
     */
    void SetUp() override;
    
    /**
     * Tear down test fixtures - called after each test
     * Cleans up all systems and validates cache coherency
     */
    void TearDown() override;

protected:
    // System instances
    std::shared_ptr<WeatherDatabase> weatherDb_;
    std::shared_ptr<RunwaySelector> runwaySelector_;
    std::shared_ptr<TerrainDatabase> terrainDb_;
    std::shared_ptr<NavigationDatabase> navDb_;
    
    // Performance tracking
    std::vector<PerformanceMetrics> performanceMetrics_;
    std::vector<DataFlowRecord> dataFlows_;
    
    // Testing utilities
    std::unique_ptr<ThreadPool> threadPool_;
    
    /**
     * Measure execution time of a function with high precision
     * @param testName Name of the test for reporting
     * @param operation Function to measure
     * @param expectedMaxMs Expected maximum execution time
     * @return Performance metrics
     */
    PerformanceMetrics measurePerformance(
        const std::string& testName,
        std::function<void()> operation,
        double expectedMaxMs = 50.0);
    
    /**
     * Measure performance over multiple iterations
     * @param testName Name of the test
     * @param operation Function to measure
     * @param iterations Number of iterations
     * @param expectedMaxMs Expected maximum per iteration
     * @return Performance metrics
     */
    PerformanceMetrics measurePerformanceIterations(
        const std::string& testName,
        std::function<void()> operation,
        int iterations = 100,
        double expectedMaxMs = 50.0);
    
    /**
     * Record data flow between systems
     * @param source Source system name
     * @param target Target system name
     * @param dataType Type of data transferred
     * @param validated Whether data was validated
     */
    void recordDataFlow(const std::string& source, const std::string& target,
                       const std::string& dataType, bool validated = true);
    
    /**
     * Validate data flow from source to target system
     * @param source Source system
     * @param target Target system
     * @return true if data flow is recorded and valid
     */
    bool validateDataFlow(const std::string& source, const std::string& target);
    
    /**
     * Get all data flows from a specific system
     * @param system System name
     * @return Vector of data flows originating from system
     */
    std::vector<DataFlowRecord> getDataFlowsFrom(const std::string& system);
    
    /**
     * Stress test concurrent access to a system
     * @param systemName Name of system to stress test
     * @param operationCount Number of concurrent operations
     * @param operation Function to execute concurrently
     * @return Performance metrics
     */
    PerformanceMetrics stressTestConcurrentAccess(
        const std::string& systemName,
        int operationCount,
        std::function<void()> operation);
    
    /**
     * Validate cache coherency across systems
     * @return true if all system caches are consistent
     */
    bool validateCacheCoherency();
    
    /**
     * Print performance summary for all tests
     */
    void printPerformanceSummary();
    
    /**
     * Get average performance across all tests
     * @return Average execution time in milliseconds
     */
    double getAveragePerformance() const;
    
    /**
     * Verify all tests met performance threshold
     * @return true if all tests completed within 50ms
     */
    bool verifyPerformanceThreshold();
};

/**
 * Integration test macros for concise test definition
 */
#define INTEGRATION_TEST(TestFixture, TestName) \
    TEST_F(TestFixture, TestName)

#define VALIDATE_PERFORMANCE(metrics, maxMs) \
    EXPECT_LE(metrics.executionTimeMs, maxMs) << \
        "Test exceeded performance threshold: " << metrics.executionTimeMs << "ms > " << maxMs << "ms"

#define VALIDATE_DATAFLOW(source, target) \
    EXPECT_TRUE(validateDataFlow(source, target)) << \
        "Data flow from " << source << " to " << target << " not validated"

#define STRESS_TEST(systemName, operations, lambda) \
    auto metrics = stressTestConcurrentAccess(systemName, operations, lambda); \
    VALIDATE_PERFORMANCE(metrics, 50.0)

} // namespace Tests
} // namespace AICopilot

#endif // INTEGRATION_FRAMEWORK_HPP

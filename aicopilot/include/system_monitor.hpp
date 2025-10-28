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

#ifndef SYSTEM_MONITOR_HPP
#define SYSTEM_MONITOR_HPP

#include "aicopilot_types.h"
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <functional>

namespace AICopilot {

/**
 * System Monitor and Health Tracking
 * 
 * Real-time monitoring of all Phase 1-2 systems, performance metrics,
 * resource utilization, and cross-system synchronization.
 */

// System health status
enum class SystemHealth {
    HEALTHY,
    DEGRADED,
    WARNING,
    CRITICAL,
    OFFLINE
};

// System component
enum class SystemComponent {
    WEATHER_SYSTEM,
    TERRAIN_AWARENESS,
    NAVIGATION,
    AIRCRAFT_SYSTEMS,
    TRAFFIC_MANAGEMENT,
    PERFORMANCE_OPTIMIZER,
    ADVANCED_PROCEDURES,
    DYNAMIC_PLANNING
};

// Component health
struct ComponentHealth {
    SystemComponent component;
    SystemHealth status;
    double cpuUsage;           // percentage
    uint64_t memoryUsage;      // bytes
    uint32_t queryCount;       // queries processed
    double averageLatency;     // milliseconds
    double errorRate;          // percentage
    std::string lastError;
    uint64_t uptime;           // milliseconds
};

// System metrics
struct SystemMetrics {
    double cpuLoad;            // percentage
    uint64_t memoryUsed;       // bytes
    uint64_t memoryAvailable;  // bytes
    uint32_t threadCount;
    double averageLatency;     // milliseconds
    uint32_t queriesPerSecond;
    double systemHealthScore;  // 0-100
    std::vector<ComponentHealth> componentMetrics;
};

// Alert
struct SystemAlert {
    uint32_t id;
    SystemComponent component;
    SystemHealth severity;
    std::string message;
    std::chrono::steady_clock::time_point timestamp;
    bool acknowledged;
};

// Performance profile
struct PerformanceProfile {
    double minLatency;         // milliseconds (best case)
    double maxLatency;         // milliseconds (worst case)
    double averageLatency;     // milliseconds
    double p95Latency;         // 95th percentile
    double p99Latency;         // 99th percentile
    uint64_t operationsPerSecond;
    double cacheHitRate;       // percentage
};

/**
 * System Monitor
 */
class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();
    
    /**
     * Initialize system monitor
     */
    bool initialize();
    
    /**
     * Shutdown monitor
     */
    void shutdown();
    
    // ============================================================
    // HEALTH MONITORING
    // ============================================================
    
    /**
     * Update component health
     */
    void updateComponentHealth(const ComponentHealth& health);
    
    /**
     * Get component health
     */
    ComponentHealth getComponentHealth(SystemComponent component) const;
    
    /**
     * Get overall system health
     */
    SystemHealth getSystemHealth() const;
    
    /**
     * Check if system is healthy enough for operations
     */
    bool isSystemOperational() const;
    
    /**
     * Get health report for all components
     */
    struct HealthReport {
        SystemHealth overallHealth;
        double healthScore;  // 0-100
        std::vector<ComponentHealth> components;
        std::vector<std::string> recommendations;
    };
    HealthReport getHealthReport() const;
    
    // ============================================================
    // PERFORMANCE TRACKING
    // ============================================================
    
    /**
     * Record query execution time
     */
    void recordQueryLatency(SystemComponent component, double latencyMs);
    
    /**
     * Record error event
     */
    void recordError(SystemComponent component, const std::string& errorMsg);
    
    /**
     * Get performance metrics
     */
    SystemMetrics getSystemMetrics() const;
    
    /**
     * Get component performance profile
     */
    PerformanceProfile getComponentPerformanceProfile(SystemComponent component) const;
    
    /**
     * Get performance trend
     */
    struct PerformanceTrend {
        std::vector<double> latencyOverTime;  // Last 60 seconds
        std::vector<uint32_t> queriesPerSecond;
        bool isImproving;
        bool isDegrading;
    };
    PerformanceTrend getPerformanceTrend(SystemComponent component) const;
    
    // ============================================================
    // RESOURCE MONITORING
    // ============================================================
    
    /**
     * Get resource utilization
     */
    struct ResourceUtilization {
        double cpuPercentage;
        double memoryPercentage;
        double diskIOPercentage;
        double networkIOPercentage;
        uint32_t threadCount;
        uint32_t activeQueries;
    };
    ResourceUtilization getResourceUtilization() const;
    
    /**
     * Get detailed memory breakdown
     */
    struct MemoryBreakdown {
        uint64_t cacheMemory;
        uint64_t bufferMemory;
        uint64_t queryMemory;
        uint64_t systemMemory;
        uint64_t overhead;
    };
    MemoryBreakdown getMemoryBreakdown() const;
    
    /**
     * Predict resource exhaustion
     */
    struct ExhaustionPrediction {
        bool memoryExhaustionRisk;
        double minutesUntilExhaustion;  // NaN if no risk
        bool cpuThrottleRisk;
        bool diskFullRisk;
    };
    ExhaustionPrediction predictResourceExhaustion() const;
    
    // ============================================================
    // CROSS-SYSTEM SYNCHRONIZATION
    // ============================================================
    
    /**
     * Synchronization state between systems
     */
    enum class SynchronizationState {
        SYNCHRONIZED,
        OUT_OF_SYNC,
        SYNCING,
        ERROR
    };
    
    /**
     * Get synchronization status
     */
    struct SynchronizationStatus {
        SynchronizationState state;
        double lastSyncTime;    // seconds ago
        uint32_t pendingUpdates;
        std::string primaryDataSource;
        std::vector<std::string> secondaryDataSources;
    };
    SynchronizationStatus getSynchronizationStatus() const;
    
    /**
     * Force system synchronization
     */
    bool forceSynchronization();
    
    /**
     * Register synchronization callback
     */
    using SyncCallback = std::function<void(SystemComponent, const std::string&)>;
    void registerSyncCallback(SyncCallback callback);
    
    // ============================================================
    // ALERT MANAGEMENT
    // ============================================================
    
    /**
     * Generate alert
     */
    void generateAlert(SystemComponent component,
                      SystemHealth severity,
                      const std::string& message);
    
    /**
     * Get active alerts
     */
    std::vector<SystemAlert> getActiveAlerts() const;
    
    /**
     * Acknowledge alert
     */
    void acknowledgeAlert(uint32_t alertId);
    
    /**
     * Clear acknowledged alerts
     */
    void clearAcknowledgedAlerts();
    
    /**
     * Register alert callback
     */
    using AlertCallback = std::function<void(const SystemAlert&)>;
    void registerAlertCallback(AlertCallback callback);
    
    // ============================================================
    // SYSTEM DIAGNOSTICS
    // ============================================================
    
    /**
     * Run diagnostic test
     */
    struct DiagnosticResult {
        bool passed;
        std::string componentName;
        std::string testName;
        double executionTimeMs;
        std::string message;
    };
    DiagnosticResult runDiagnostic(SystemComponent component);
    
    /**
     * Run full system diagnostics
     */
    std::vector<DiagnosticResult> runFullDiagnostics();
    
    /**
     * Get system uptime
     */
    uint64_t getSystemUptime() const;
    
    /**
     * Get system statistics
     */
    struct SystemStatistics {
        uint64_t totalQueries;
        uint64_t totalErrors;
        uint64_t totalAlerts;
        double averageHealth;
        uint64_t uptimeSeconds;
        double mtbf;  // Mean time between failures
        double mttr;  // Mean time to recovery
    };
    SystemStatistics getSystemStatistics() const;
    
    // ============================================================
    // OPTIMIZATION RECOMMENDATIONS
    // ============================================================
    
    /**
     * Get optimization recommendations
     */
    std::vector<std::string> getOptimizationRecommendations() const;
    
    /**
     * Analyze bottlenecks
     */
    struct Bottleneck {
        SystemComponent component;
        std::string bottleneckName;
        double impact;  // 0-100
        std::string recommendation;
    };
    std::vector<Bottleneck> analyzeBottlenecks() const;
    
    /**
     * Enable/disable detailed monitoring
     */
    void setDetailedMonitoring(bool enabled);
    
    /**
     * Export system report
     */
    std::string exportSystemReport() const;
    
private:
    bool initialized_;
    bool detailedMonitoring_;
    
    // Component health tracking
    std::vector<ComponentHealth> componentHealth_;
    
    // Performance metrics
    std::vector<SystemMetrics> metricsHistory_;
    std::vector<std::vector<double>> latencyHistory_;
    
    // Alerts
    std::vector<SystemAlert> alerts_;
    uint32_t nextAlertId_;
    
    // Callbacks
    AlertCallback alertCallback_;
    SyncCallback syncCallback_;
    
    // Timing
    std::chrono::steady_clock::time_point startTime_;
    uint64_t totalQueries_;
    uint64_t totalErrors_;
    
    // Helper methods
    double calculateHealthScore() const;
    SystemHealth determineOverallHealth() const;
    void updateMetricsHistory();
};

} // namespace AICopilot

#endif // SYSTEM_MONITOR_HPP

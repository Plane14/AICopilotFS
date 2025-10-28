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

#include "system_monitor.hpp"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <cmath>

namespace AICopilot {

SystemMonitor::SystemMonitor()
    : initialized_(false),
      detailedMonitoring_(false),
      nextAlertId_(1),
      totalQueries_(0),
      totalErrors_(0) {
    startTime_ = std::chrono::steady_clock::now();
    
    // Initialize component health tracking
    componentHealth_.resize(8);
    for (size_t i = 0; i < componentHealth_.size(); ++i) {
        componentHealth_[i].component = static_cast<SystemComponent>(i);
        componentHealth_[i].status = SystemHealth::HEALTHY;
        componentHealth_[i].cpuUsage = 0.0;
        componentHealth_[i].memoryUsage = 0;
        componentHealth_[i].queryCount = 0;
        componentHealth_[i].averageLatency = 0.0;
        componentHealth_[i].errorRate = 0.0;
    }
}

SystemMonitor::~SystemMonitor() {
    shutdown();
}

bool SystemMonitor::initialize() {
    initialized_ = true;
    startTime_ = std::chrono::steady_clock::now();
    return true;
}

void SystemMonitor::shutdown() {
    if (initialized_) {
        initialized_ = false;
        componentHealth_.clear();
        metricsHistory_.clear();
        latencyHistory_.clear();
        alerts_.clear();
    }
}

// ============================================================
// HEALTH MONITORING
// ============================================================

void SystemMonitor::updateComponentHealth(const ComponentHealth& health) {
    if (static_cast<size_t>(health.component) < componentHealth_.size()) {
        componentHealth_[static_cast<size_t>(health.component)] = health;
        
        // Check for degradation and generate alert if needed
        if (health.status != SystemHealth::HEALTHY) {
            generateAlert(health.component, health.status,
                         "Component status: " + health.lastError);
        }
    }
}

ComponentHealth SystemMonitor::getComponentHealth(SystemComponent component) const {
    size_t idx = static_cast<size_t>(component);
    if (idx < componentHealth_.size()) {
        return componentHealth_[idx];
    }
    
    ComponentHealth empty;
    empty.component = component;
    empty.status = SystemHealth::OFFLINE;
    return empty;
}

SystemHealth SystemMonitor::getSystemHealth() const {
    return determineOverallHealth();
}

bool SystemMonitor::isSystemOperational() const {
    SystemHealth health = getSystemHealth();
    return health != SystemHealth::OFFLINE && health != SystemHealth::CRITICAL;
}

SystemMonitor::HealthReport SystemMonitor::getHealthReport() const {
    HealthReport report;
    
    report.overallHealth = determineOverallHealth();
    report.healthScore = calculateHealthScore();
    report.components = componentHealth_;
    
    if (report.healthScore < 80.0) {
        report.recommendations.push_back("Monitor system performance closely");
        report.recommendations.push_back("Check for resource bottlenecks");
    }
    
    if (report.healthScore < 50.0) {
        report.recommendations.push_back("URGENT: System degradation detected");
        report.recommendations.push_back("Review component error logs immediately");
    }
    
    return report;
}

// ============================================================
// PERFORMANCE TRACKING
// ============================================================

void SystemMonitor::recordQueryLatency(SystemComponent component, double latencyMs) {
    totalQueries_++;
    
    size_t idx = static_cast<size_t>(component);
    if (idx < componentHealth_.size()) {
        componentHealth_[idx].queryCount++;
        
        // Update average latency (exponential moving average)
        double alpha = 0.1;  // Weight for new value
        componentHealth_[idx].averageLatency =
            (alpha * latencyMs) + ((1 - alpha) * componentHealth_[idx].averageLatency);
    }
    
    // Store in history for trending
    if (latencyHistory_.size() <= static_cast<size_t>(component)) {
        latencyHistory_.resize(static_cast<size_t>(component) + 1);
    }
    
    latencyHistory_[idx].push_back(latencyMs);
    if (latencyHistory_[idx].size() > 600) {  // Keep last 10 minutes at 1 sample/sec
        latencyHistory_[idx].erase(latencyHistory_[idx].begin());
    }
}

void SystemMonitor::recordError(SystemComponent component, const std::string& errorMsg) {
    totalErrors_++;
    
    size_t idx = static_cast<size_t>(component);
    if (idx < componentHealth_.size()) {
        componentHealth_[idx].lastError = errorMsg;
        componentHealth_[idx].errorRate = (totalErrors_ / static_cast<double>(totalQueries_ + 1)) * 100.0;
        
        if (componentHealth_[idx].errorRate > 5.0) {
            componentHealth_[idx].status = SystemHealth::WARNING;
        }
    }
    
    generateAlert(component, SystemHealth::WARNING, errorMsg);
}

SystemMetrics SystemMonitor::getSystemMetrics() const {
    SystemMetrics metrics = {0};
    
    // Calculate aggregate metrics
    double totalCpu = 0.0;
    uint64_t totalMemory = 0;
    uint32_t totalQueries = 0;
    double totalLatency = 0.0;
    uint32_t componentCount = 0;
    
    for (const auto& comp : componentHealth_) {
        totalCpu += comp.cpuUsage;
        totalMemory += comp.memoryUsage;
        totalQueries += comp.queryCount;
        totalLatency += comp.averageLatency;
        componentCount++;
    }
    
    metrics.cpuLoad = (componentCount > 0) ? totalCpu / componentCount : 0.0;
    metrics.memoryUsed = totalMemory;
    metrics.memoryAvailable = 2000000000;  // 2GB available (estimated)
    metrics.threadCount = 16;  // System dependent
    metrics.averageLatency = (componentCount > 0) ? totalLatency / componentCount : 0.0;
    metrics.queriesPerSecond = totalQueries;
    metrics.systemHealthScore = calculateHealthScore();
    metrics.componentMetrics = componentHealth_;
    
    return metrics;
}

PerformanceProfile SystemMonitor::getComponentPerformanceProfile(SystemComponent component) const {
    PerformanceProfile profile = {0};
    
    size_t idx = static_cast<size_t>(component);
    if (idx >= latencyHistory_.size() || latencyHistory_[idx].empty()) {
        return profile;
    }
    
    const auto& latencies = latencyHistory_[idx];
    
    profile.minLatency = *std::min_element(latencies.begin(), latencies.end());
    profile.maxLatency = *std::max_element(latencies.begin(), latencies.end());
    profile.averageLatency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    
    // Calculate percentiles
    std::vector<double> sorted = latencies;
    std::sort(sorted.begin(), sorted.end());
    
    profile.p95Latency = sorted[static_cast<size_t>(sorted.size() * 0.95)];
    profile.p99Latency = sorted[static_cast<size_t>(sorted.size() * 0.99)];
    
    profile.operationsPerSecond = componentHealth_[idx].queryCount;
    profile.cacheHitRate = 65.0;  // Estimated
    
    return profile;
}

SystemMonitor::PerformanceTrend SystemMonitor::getPerformanceTrend(SystemComponent component) const {
    PerformanceTrend trend;
    
    size_t idx = static_cast<size_t>(component);
    if (idx >= latencyHistory_.size() || latencyHistory_[idx].empty()) {
        return trend;
    }
    
    trend.latencyOverTime = latencyHistory_[idx];
    
    // Determine if improving or degrading
    if (trend.latencyOverTime.size() >= 2) {
        double firstHalf = std::accumulate(
            trend.latencyOverTime.begin(),
            trend.latencyOverTime.begin() + trend.latencyOverTime.size() / 2,
            0.0) / (trend.latencyOverTime.size() / 2);
        
        double secondHalf = std::accumulate(
            trend.latencyOverTime.begin() + trend.latencyOverTime.size() / 2,
            trend.latencyOverTime.end(),
            0.0) / (trend.latencyOverTime.size() / 2);
        
        trend.isImproving = secondHalf < firstHalf;
        trend.isDegrading = secondHalf > firstHalf;
    }
    
    return trend;
}

// ============================================================
// RESOURCE MONITORING
// ============================================================

SystemMonitor::ResourceUtilization SystemMonitor::getResourceUtilization() const {
    ResourceUtilization util = {0};
    
    double totalCpu = 0.0;
    for (const auto& comp : componentHealth_) {
        totalCpu += comp.cpuUsage;
    }
    
    util.cpuPercentage = totalCpu;
    util.memoryPercentage = (100 * 52428800) / (100 * 2000000000);  // Used / Available
    util.diskIOPercentage = 15.0;  // Estimated
    util.networkIOPercentage = 8.0;  // Estimated
    util.threadCount = 16;
    util.activeQueries = totalQueries_;
    
    return util;
}

SystemMonitor::MemoryBreakdown SystemMonitor::getMemoryBreakdown() const {
    MemoryBreakdown breakdown = {0};
    
    uint64_t totalUsed = 0;
    for (const auto& comp : componentHealth_) {
        totalUsed += comp.memoryUsage;
    }
    
    breakdown.cacheMemory = (totalUsed * 40) / 100;      // 40% cache
    breakdown.bufferMemory = (totalUsed * 20) / 100;     // 20% buffers
    breakdown.queryMemory = (totalUsed * 25) / 100;      // 25% queries
    breakdown.systemMemory = (totalUsed * 10) / 100;     // 10% system
    breakdown.overhead = (totalUsed * 5) / 100;          // 5% overhead
    
    return breakdown;
}

SystemMonitor::ExhaustionPrediction SystemMonitor::predictResourceExhaustion() const {
    ExhaustionPrediction prediction;
    
    // Simple trend analysis
    ResourceUtilization util = getResourceUtilization();
    
    prediction.memoryExhaustionRisk = util.memoryPercentage > 85.0;
    prediction.minutesUntilExhaustion = (util.memoryPercentage > 85.0) ? 15.0 : NAN;
    prediction.cpuThrottleRisk = util.cpuPercentage > 90.0;
    prediction.diskFullRisk = util.diskIOPercentage > 95.0;
    
    return prediction;
}

// ============================================================
// CROSS-SYSTEM SYNCHRONIZATION
// ============================================================

SystemMonitor::SynchronizationStatus SystemMonitor::getSynchronizationStatus() const {
    SynchronizationStatus status;
    
    status.state = SynchronizationState::SYNCHRONIZED;
    status.lastSyncTime = 1.5;  // seconds ago
    status.pendingUpdates = 0;
    status.primaryDataSource = "LocalNavDatabase";
    status.secondaryDataSources = {"TerrainCache", "WeatherService"};
    
    return status;
}

bool SystemMonitor::forceSynchronization() {
    SynchronizationStatus status;
    status.state = SynchronizationState::SYNCING;
    
    if (syncCallback_) {
        syncCallback_(SystemComponent::NAVIGATION, "Synchronization started");
    }
    
    // Simulate sync completion
    status.state = SynchronizationState::SYNCHRONIZED;
    
    if (syncCallback_) {
        syncCallback_(SystemComponent::NAVIGATION, "Synchronization completed");
    }
    
    return true;
}

void SystemMonitor::registerSyncCallback(SyncCallback callback) {
    syncCallback_ = callback;
}

// ============================================================
// ALERT MANAGEMENT
// ============================================================

void SystemMonitor::generateAlert(SystemComponent component,
                                 SystemHealth severity,
                                 const std::string& message) {
    SystemAlert alert;
    alert.id = nextAlertId_++;
    alert.component = component;
    alert.severity = severity;
    alert.message = message;
    alert.timestamp = std::chrono::steady_clock::now();
    alert.acknowledged = false;
    
    alerts_.push_back(alert);
    
    if (alertCallback_) {
        alertCallback_(alert);
    }
    
    // Keep only last 100 alerts
    if (alerts_.size() > 100) {
        alerts_.erase(alerts_.begin());
    }
}

std::vector<SystemAlert> SystemMonitor::getActiveAlerts() const {
    std::vector<SystemAlert> active;
    
    for (const auto& alert : alerts_) {
        if (!alert.acknowledged) {
            active.push_back(alert);
        }
    }
    
    return active;
}

void SystemMonitor::acknowledgeAlert(uint32_t alertId) {
    for (auto& alert : alerts_) {
        if (alert.id == alertId) {
            alert.acknowledged = true;
            return;
        }
    }
}

void SystemMonitor::clearAcknowledgedAlerts() {
    alerts_.erase(
        std::remove_if(alerts_.begin(), alerts_.end(),
                      [](const SystemAlert& a) { return a.acknowledged; }),
        alerts_.end());
}

void SystemMonitor::registerAlertCallback(AlertCallback callback) {
    alertCallback_ = callback;
}

// ============================================================
// SYSTEM DIAGNOSTICS
// ============================================================

SystemMonitor::DiagnosticResult SystemMonitor::runDiagnostic(SystemComponent component) {
    DiagnosticResult result;
    
    result.componentName = "System";
    result.testName = "Component Health Check";
    result.passed = true;
    result.executionTimeMs = 25.5;
    result.message = "Component is operational";
    
    // Check component health
    ComponentHealth health = getComponentHealth(component);
    if (health.status != SystemHealth::HEALTHY) {
        result.passed = false;
        result.message = "Component degraded: " + health.lastError;
    }
    
    return result;
}

std::vector<SystemMonitor::DiagnosticResult> SystemMonitor::runFullDiagnostics() {
    std::vector<DiagnosticResult> results;
    
    for (size_t i = 0; i < 8; ++i) {
        DiagnosticResult result = runDiagnostic(static_cast<SystemComponent>(i));
        results.push_back(result);
    }
    
    return results;
}

uint64_t SystemMonitor::getSystemUptime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now - startTime_).count();
}

SystemMonitor::SystemStatistics SystemMonitor::getSystemStatistics() const {
    SystemStatistics stats;
    
    stats.totalQueries = totalQueries_;
    stats.totalErrors = totalErrors_;
    stats.totalAlerts = alerts_.size();
    stats.averageHealth = calculateHealthScore();
    stats.uptimeSeconds = getSystemUptime() / 1000;
    stats.mtbf = (stats.totalQueries > 0) ? (stats.uptimeSeconds / (stats.totalErrors + 1)) : 0.0;
    stats.mttr = (stats.totalErrors > 0) ? 15.0 : 0.0;  // Mean time to recover (estimated)
    
    return stats;
}

// ============================================================
// OPTIMIZATION RECOMMENDATIONS
// ============================================================

std::vector<std::string> SystemMonitor::getOptimizationRecommendations() const {
    std::vector<std::string> recommendations;
    
    ResourceUtilization util = getResourceUtilization();
    
    if (util.cpuPercentage > 75.0) {
        recommendations.push_back("CPU utilization high - consider load balancing");
    }
    
    if (util.memoryPercentage > 80.0) {
        recommendations.push_back("Memory utilization high - consider cache optimization");
    }
    
    if (util.activeQueries > 1000) {
        recommendations.push_back("High query load - consider query batching");
    }
    
    double cacheHit = 0.0;
    for (const auto& comp : componentHealth_) {
        if (comp.queryCount > 0) {
            cacheHit = comp.queryCount * 0.65;  // Estimated 65% hit rate
        }
    }
    
    if (cacheHit < 50.0) {
        recommendations.push_back("Low cache hit rate - consider prefetching strategy");
    }
    
    return recommendations;
}

std::vector<SystemMonitor::Bottleneck> SystemMonitor::analyzeBottlenecks() const {
    std::vector<Bottleneck> bottlenecks;
    
    for (const auto& comp : componentHealth_) {
        if (comp.averageLatency > 10.0) {
            Bottleneck bn;
            bn.component = comp.component;
            bn.bottleneckName = "High Latency";
            bn.impact = (comp.averageLatency / 50.0) * 100.0;
            bn.recommendation = "Review query patterns and caching";
            bottlenecks.push_back(bn);
        }
        
        if (comp.errorRate > 2.0) {
            Bottleneck bn;
            bn.component = comp.component;
            bn.bottleneckName = "High Error Rate";
            bn.impact = comp.errorRate * 2.0;
            bn.recommendation = "Check error logs and system stability";
            bottlenecks.push_back(bn);
        }
    }
    
    return bottlenecks;
}

void SystemMonitor::setDetailedMonitoring(bool enabled) {
    detailedMonitoring_ = enabled;
}

std::string SystemMonitor::exportSystemReport() const {
    std::stringstream report;
    
    SystemStatistics stats = getSystemStatistics();
    HealthReport health = getHealthReport();
    ResourceUtilization util = getResourceUtilization();
    
    report << "=== AI COPILOT FS - SYSTEM REPORT ===\n\n";
    report << "UPTIME: " << stats.uptimeSeconds << " seconds\n";
    report << "OVERALL HEALTH: " << static_cast<int>(health.healthScore) << "/100\n";
    report << "TOTAL QUERIES: " << stats.totalQueries << "\n";
    report << "TOTAL ERRORS: " << stats.totalErrors << "\n";
    report << "ACTIVE ALERTS: " << stats.totalAlerts << "\n\n";
    
    report << "RESOURCE UTILIZATION:\n";
    report << "  CPU: " << util.cpuPercentage << "%\n";
    report << "  Memory: " << util.memoryPercentage << "%\n";
    report << "  Threads: " << util.threadCount << "\n\n";
    
    report << "COMPONENT STATUS:\n";
    for (const auto& comp : health.components) {
        report << "  " << static_cast<int>(comp.component) << ": "
               << static_cast<int>(comp.status) << " "
               << "Latency:" << comp.averageLatency << "ms\n";
    }
    
    return report.str();
}

// ============================================================
// PRIVATE HELPER METHODS
// ============================================================

double SystemMonitor::calculateHealthScore() const {
    if (componentHealth_.empty()) {
        return 100.0;
    }
    
    double score = 0.0;
    for (const auto& comp : componentHealth_) {
        double componentScore = 100.0;
        
        // Deduct for errors
        componentScore -= (comp.errorRate * 10.0);
        
        // Deduct for latency
        if (comp.averageLatency > 5.0) {
            componentScore -= ((comp.averageLatency - 5.0) / 50.0) * 10.0;
        }
        
        // Status factor
        if (comp.status == SystemHealth::DEGRADED) {
            componentScore -= 10.0;
        } else if (comp.status == SystemHealth::WARNING) {
            componentScore -= 25.0;
        } else if (comp.status == SystemHealth::CRITICAL) {
            componentScore -= 50.0;
        }
        
        componentScore = std::max(0.0, componentScore);
        score += componentScore;
    }
    
    return score / componentHealth_.size();
}

SystemHealth SystemMonitor::determineOverallHealth() const {
    double score = calculateHealthScore();
    
    if (score >= 90.0) {
        return SystemHealth::HEALTHY;
    } else if (score >= 75.0) {
        return SystemHealth::DEGRADED;
    } else if (score >= 50.0) {
        return SystemHealth::WARNING;
    } else if (score > 0.0) {
        return SystemHealth::CRITICAL;
    } else {
        return SystemHealth::OFFLINE;
    }
}

void SystemMonitor::updateMetricsHistory() {
    SystemMetrics metrics = getSystemMetrics();
    metricsHistory_.push_back(metrics);
    
    if (metricsHistory_.size() > 600) {  // Keep 10 minutes of history
        metricsHistory_.erase(metricsHistory_.begin());
    }
}

} // namespace AICopilot

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

#ifndef PERFORMANCE_OPTIMIZER_HPP
#define PERFORMANCE_OPTIMIZER_HPP

#include "aicopilot_types.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <mutex>

namespace AICopilot {

/**
 * Performance Optimization System
 * 
 * Implements caching, query batching, prefetching, memory pooling,
 * and other optimization strategies for Phase 1-2 systems.
 */

// Performance metrics
struct PerformanceMetrics {
    uint64_t queryCount;           // Total queries
    uint64_t cacheHits;            // Successful cache hits
    double cacheHitRate;           // Hit rate percentage
    double averageQueryTime;       // milliseconds
    double maxQueryTime;           // milliseconds
    double minQueryTime;           // milliseconds
    uint64_t memoryUsed;           // bytes
    uint64_t memoryAllocated;      // bytes
};

// Cache entry
template<typename T>
struct CacheEntry {
    T value;
    std::chrono::steady_clock::time_point timestamp;
    uint32_t accessCount;
    double score;  // For LRU/LFU eviction
};

// Query batch
struct QueryBatch {
    uint32_t batchId;
    std::vector<Position> positions;
    std::chrono::steady_clock::time_point createdTime;
    size_t maxBatchSize;
    uint32_t timeoutMs;
    bool isReady;
};

// Prefetch strategy
enum class PrefetchStrategy {
    PREDICTIVE,          // Based on flight path prediction
    REACTIVE,            // Based on active requests
    SPATIAL,             // Spatial locality (grid-based)
    TEMPORAL,            // Temporal locality (recent accesses)
    HYBRID               // Combined strategy
};

// Prefetch request
struct PrefetchRequest {
    Position center;
    double radius;       // nautical miles
    PrefetchStrategy strategy;
    uint32_t priority;   // 0=low, 10=high
    bool completed;
};

/**
 * Query Result Cache with TTL management
 */
template<typename KeyType, typename ValueType>
class QueryCache {
public:
    QueryCache(size_t maxEntries = 10000, uint32_t ttlSeconds = 300)
        : maxEntries_(maxEntries), ttlSeconds_(ttlSeconds) {
    }
    
    /**
     * Put value in cache
     */
    void put(const KeyType& key, const ValueType& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        CacheEntry<ValueType> entry;
        entry.value = value;
        entry.timestamp = std::chrono::steady_clock::now();
        entry.accessCount = 1;
        entry.score = 1.0;
        
        cache_[key] = entry;
        
        if (cache_.size() > maxEntries_) {
            evictLRU();
        }
    }
    
    /**
     * Get value from cache (null if not found or expired)
     */
    bool get(const KeyType& key, ValueType& outValue) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return false;
        }
        
        // Check TTL
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.timestamp).count();
        
        if (age > ttlSeconds_) {
            cache_.erase(it);
            return false;
        }
        
        // Update access metrics
        it->second.accessCount++;
        it->second.score = it->second.accessCount;
        outValue = it->second.value;
        
        return true;
    }
    
    /**
     * Clear cache
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }
    
    /**
     * Get cache size
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }
    
    /**
     * Get metrics
     */
    PerformanceMetrics getMetrics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        PerformanceMetrics metrics = {0};
        metrics.memoryUsed = cache_.size() * sizeof(CacheEntry<ValueType>);
        return metrics;
    }
    
private:
    std::unordered_map<KeyType, CacheEntry<ValueType>, std::hash<KeyType>> cache_;
    size_t maxEntries_;
    uint32_t ttlSeconds_;
    mutable std::mutex mutex_;
    
    void evictLRU() {
        // Find entry with lowest score
        auto minIt = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.score < minIt->second.score) {
                minIt = it;
            }
        }
        cache_.erase(minIt);
    }
};

/**
 * Performance Optimizer
 */
class PerformanceOptimizer {
public:
    PerformanceOptimizer();
    ~PerformanceOptimizer();
    
    /**
     * Initialize optimizer
     */
    bool initialize();
    
    /**
     * Shutdown optimizer
     */
    void shutdown();
    
    // ============================================================
    // CACHING OPTIMIZATION
    // ============================================================
    
    /**
     * Cache terrain elevation data with TTL
     */
    void cacheTerrainElevation(const Position& pos, double elevation);
    
    /**
     * Get cached terrain elevation
     * Returns true if cache hit, false otherwise
     */
    bool getTerrainElevation(const Position& pos, double& elevation);
    
    /**
     * Cache navigation waypoint
     */
    void cacheWaypoint(const std::string& id, const Waypoint& waypoint);
    
    /**
     * Get cached waypoint
     */
    bool getWaypoint(const std::string& id, Waypoint& waypoint);
    
    /**
     * Cache weather data
     */
    void cacheWeatherData(const Position& pos, const WeatherConditions& weather);
    
    /**
     * Get cached weather
     */
    bool getWeatherData(const Position& pos, WeatherConditions& weather);
    
    /**
     * Clear all caches
     */
    void clearAllCaches();
    
    /**
     * Get cache statistics
     */
    PerformanceMetrics getCacheStatistics() const;
    
    // ============================================================
    // QUERY BATCHING
    // ============================================================
    
    /**
     * Create query batch for multiple positions
     */
    QueryBatch createQueryBatch(const std::vector<Position>& positions,
                               uint32_t timeoutMs = 100,
                               size_t maxBatchSize = 1000);
    
    /**
     * Add query to batch
     */
    void addQueryToBatch(uint32_t batchId, const Position& position);
    
    /**
     * Check if batch is ready for processing
     */
    bool isBatchReady(uint32_t batchId) const;
    
    /**
     * Process batch (for integrated terrain/nav queries)
     */
    std::vector<double> processBatch(const QueryBatch& batch);
    
    /**
     * Get batch statistics
     */
    struct BatchStatistics {
        uint64_t batchesCreated;
        uint64_t queriesOptimized;
        double percentageOptimized;
        double averageBatchSize;
        double averageSpeedupFactor;
    };
    BatchStatistics getBatchStatistics() const;
    
    // ============================================================
    // PREFETCHING
    // ============================================================
    
    /**
     * Prefetch terrain data in spatial grid
     */
    void prefetchTerrainGrid(const Position& centerPos,
                           double radius,  // nautical miles
                           PrefetchStrategy strategy);
    
    /**
     * Prefetch navigation data
     */
    void prefetchNavigationData(const Position& centerPos,
                               double radius,
                               PrefetchStrategy strategy);
    
    /**
     * Prefetch along flight path
     */
    void prefetchAlongFlightPath(const std::vector<Waypoint>& flightPath,
                                double lookAheadDistance);  // nautical miles
    
    /**
     * Get prefetch status
     */
    struct PrefetchStatus {
        uint32_t activePrefetches;
        uint32_t completedPrefetches;
        std::vector<std::string> pendingItems;
        double completionPercentage;
    };
    PrefetchStatus getPrefetchStatus() const;
    
    // ============================================================
    // MEMORY OPTIMIZATION
    // ============================================================
    
    /**
     * Object pool for frequently allocated objects
     */
    class MemoryPool {
    public:
        MemoryPool(size_t objectSize, size_t poolSize = 1000)
            : objectSize_(objectSize), poolSize_(poolSize) {
        }
        
        /**
         * Allocate from pool
         */
        void* allocate() {
            if (available_.empty()) {
                return malloc(objectSize_);
            }
            void* ptr = available_.back();
            available_.pop_back();
            return ptr;
        }
        
        /**
         * Return to pool
         */
        void deallocate(void* ptr) {
            if (available_.size() < poolSize_) {
                available_.push_back(ptr);
            } else {
                free(ptr);
            }
        }
        
    private:
        size_t objectSize_;
        size_t poolSize_;
        std::vector<void*> available_;
    };
    
    /**
     * Get memory usage statistics
     */
    struct MemoryStatistics {
        uint64_t totalAllocated;
        uint64_t totalUsed;
        uint64_t poolFragmentation;
        double utilizationPercentage;
    };
    MemoryStatistics getMemoryStatistics() const;
    
    /**
     * Optimize memory usage
     */
    void optimizeMemory();
    
    // ============================================================
    // QUERY OPTIMIZATION
    // ============================================================
    
    /**
     * Optimize spatial query (e.g., nearby waypoints)
     */
    std::vector<Waypoint> optimizeSpatialQuery(
        const Position& center,
        double radius,
        const std::vector<Waypoint>& allWaypoints);
    
    /**
     * Optimize range query
     */
    std::vector<Waypoint> optimizeRangeQuery(
        const Position& start,
        const Position& end,
        const std::vector<Waypoint>& allWaypoints);
    
    /**
     * Get query optimization statistics
     */
    struct QueryOptimizationStats {
        double averageQueryTimeMs;
        double averageUnoptimizedTimeMs;
        double speedupFactor;
        uint64_t queriesProcessed;
    };
    QueryOptimizationStats getQueryOptimizationStats() const;
    
    // ============================================================
    // SYSTEM MONITORING
    // ============================================================
    
    /**
     * Enable/disable performance tracking
     */
    void enablePerformanceTracking(bool enable);
    
    /**
     * Get overall performance report
     */
    struct PerformanceReport {
        PerformanceMetrics cacheMetrics;
        BatchStatistics batchStats;
        MemoryStatistics memoryStats;
        QueryOptimizationStats queryStats;
        uint64_t uptime;  // milliseconds
        double averageSystemLoad;
    };
    PerformanceReport getPerformanceReport() const;
    
    /**
     * Reset performance counters
     */
    void resetPerformanceCounters();
    
private:
    bool initialized_;
    bool trackingEnabled_;
    
    // Caches
    QueryCache<std::string, Waypoint> waypointCache_;
    QueryCache<std::string, double> elevationCache_;
    QueryCache<std::string, WeatherConditions> weatherCache_;
    
    // Query batching
    std::unordered_map<uint32_t, QueryBatch> activeBatches_;
    uint32_t nextBatchId_;
    
    // Prefetching
    std::vector<PrefetchRequest> prefetchRequests_;
    
    // Performance tracking
    PerformanceMetrics totalMetrics_;
    std::chrono::steady_clock::time_point startTime_;
    uint64_t queryCount_;
    uint64_t cacheHits_;
};

} // namespace AICopilot

#endif // PERFORMANCE_OPTIMIZER_HPP

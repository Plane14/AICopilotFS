/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Terrain Database - SRTM 30m elevation data integration
* Provides elevation lookups with LRU caching mechanism
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef TERRAIN_DATABASE_HPP
#define TERRAIN_DATABASE_HPP

#include "aicopilot_types.h"
#include <vector>
#include <map>
#include <memory>
#include <cstring>
#include <chrono>
#include <mutex>

namespace AICopilot {

/**
 * SRTM Terrain Database
 * 
 * Provides elevation data from SRTM 30m resolution tiles
 * Format: Tile files named N/S{LAT}E/W{LON}.hgt
 * Data: 3601x3601 int16 samples per 1° × 1° tile (big-endian)
 */
class TerrainDatabase {
public:
    // SRTM tile dimensions
    static constexpr int SRTM_SIZE = 3601;  // 3601x3601 samples
    static constexpr double SRTM_RESOLUTION = 1.0 / 3600.0;  // 1 arc-second
    static constexpr int MAX_CACHE_TILES = 16;  // LRU cache size
    static constexpr int TILE_SIZE_BYTES = SRTM_SIZE * SRTM_SIZE * 2;  // 25,934,402 bytes
    
    // Elevation tile data
    struct ElevationTile {
        int tileLatitude;           // Tile latitude (-60 to 60)
        int tileLongitude;          // Tile longitude (-180 to 179)
        std::vector<int16_t> data;  // 3601 x 3601 elevation samples
        bool isLoaded;
        std::chrono::system_clock::time_point lastAccessed;
        
        ElevationTile()
            : tileLatitude(0), tileLongitude(0), isLoaded(false),
              lastAccessed(std::chrono::system_clock::now()) {}
    };

    TerrainDatabase();
    ~TerrainDatabase();
    
    /**
     * Initialize terrain database with path to SRTM data
     * @param srtmDataPath Path to directory containing .hgt files
     * @return true if initialized successfully
     */
    bool initialize(const std::string& srtmDataPath);
    
    /**
     * Shutdown and clear cache
     */
    void shutdown();
    
    /**
     * Get elevation at latitude/longitude using bilinear interpolation
     * @param latitude Latitude in degrees (-90 to 90)
     * @param longitude Longitude in degrees (-180 to 180)
     * @return Elevation in feet (MSL), or 0 if data unavailable
     */
    double getElevationAt(double latitude, double longitude);
    
    /**
     * Get elevation at position
     * @param position Position with latitude and longitude
     * @return Elevation in feet (MSL)
     */
    double getElevationAt(const Position& position);
    
    /**
     * Get elevation profile along a path
     * @param start Start position
     * @param end End position
     * @param samples Number of samples along path
     * @return Vector of elevations
     */
    std::vector<double> getElevationProfile(
        const Position& start,
        const Position& end,
        int samples = 10);
    
    /**
     * Get terrain statistics for an area
     * @param center Center position
     * @param radiusNM Radius in nautical miles
     * @param minElev Output minimum elevation
     * @param maxElev Output maximum elevation
     * @param avgElev Output average elevation
     * @return true if data available
     */
    bool getTerrainStats(const Position& center, double radiusNM,
                        double& minElev, double& maxElev, double& avgElev);
    
    /**
     * Check if elevation data is available for position
     * @param latitude Latitude
     * @param longitude Longitude
     * @return true if SRTM data covers this location
     */
    bool isDataAvailable(double latitude, double longitude) const;
    
    /**
     * Clear LRU cache
     */
    void clearCache();
    
    /**
     * Get cache statistics
     * @return Pair of (tiles_in_cache, cache_size_mb)
     */
    std::pair<int, int> getCacheStats() const;

private:
    std::string srtmDataPath_;
    std::map<std::string, ElevationTile> tileCache_;
    std::vector<std::string> cacheOrder_;  // For LRU ordering
    mutable std::mutex cacheMutex_;
    
    // Helper methods
    std::string getTileFilename(int tileLatitude, int tileLongitude) const;
    bool loadTile(int tileLatitude, int tileLongitude);
    void evictOldestTile();
    int16_t getRawElevation(const ElevationTile& tile, int row, int col) const;
    double interpolateElevation(const ElevationTile& tile, 
                               double fracRow, double fracCol) const;
    static int16_t bytesToInt16BE(const unsigned char* bytes);
};

} // namespace AICopilot

#endif // TERRAIN_DATABASE_HPP

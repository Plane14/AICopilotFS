/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Terrain Database Implementation - SRTM elevation data
* Provides elevation lookups with LRU caching
*****************************************************************************/

#include "../include/terrain_database.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <sstream>

namespace AICopilot {

constexpr double FEET_PER_METER = 3.28084;
constexpr double EARTH_RADIUS_NM = 3440.065;
constexpr double PI = 3.14159265358979323846;

TerrainDatabase::TerrainDatabase() = default;

TerrainDatabase::~TerrainDatabase() {
    shutdown();
}

bool TerrainDatabase::initialize(const std::string& srtmDataPath) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    srtmDataPath_ = srtmDataPath;
    tileCache_.clear();
    cacheOrder_.clear();
    std::cout << "TerrainDatabase: Initialized with path: " << srtmDataPath << std::endl;
    return true;
}

void TerrainDatabase::shutdown() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    tileCache_.clear();
    cacheOrder_.clear();
}

double TerrainDatabase::getElevationAt(double latitude, double longitude) {
    // Check SRTM coverage (-60 to +60 latitude)
    if (latitude < -60.0 || latitude > 60.0) {
        return 0.0;  // No data outside SRTM coverage
    }
    
    // Handle edge cases
    if (latitude == 90.0) latitude = 89.999;
    if (latitude == -90.0) latitude = -89.999;
    
    // Calculate tile indices
    int tileLatitude = (latitude >= 0.0) ? static_cast<int>(latitude) 
                                         : static_cast<int>(latitude) - 1;
    int tileLongitude = (longitude >= 0.0) ? static_cast<int>(longitude) 
                                           : static_cast<int>(longitude) - 1;
    
    // Normalize longitude to -180 to 179 range
    while (tileLongitude < -180) tileLongitude += 360;
    while (tileLongitude >= 180) tileLongitude -= 360;
    
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        
        // Load tile if not in cache
        std::string tileKey = getTileFilename(tileLatitude, tileLongitude);
        if (tileCache_.find(tileKey) == tileCache_.end()) {
            if (!loadTile(tileLatitude, tileLongitude)) {
                return 0.0;  // Data not available
            }
        }
        
        auto& tile = tileCache_[tileKey];
        tile.lastAccessed = std::chrono::system_clock::now();
        
        // Move to end of LRU queue
        auto it = std::find(cacheOrder_.begin(), cacheOrder_.end(), tileKey);
        if (it != cacheOrder_.end()) {
            cacheOrder_.erase(it);
        }
        cacheOrder_.push_back(tileKey);
        
        // Calculate position within tile (0 to 1)
        double fracLat = latitude - tileLatitude;
        double fracLon = longitude - tileLongitude;
        
        // Convert to row/col (0 to 3600)
        double row = fracLat * 3600.0;
        double col = fracLon * 3600.0;
        
        // Clamp to valid range
        row = std::min(3600.0, std::max(0.0, row));
        col = std::min(3600.0, std::max(0.0, col));
        
        // Bilinear interpolation
        double elev = interpolateElevation(tile, row, col);
        
        // Convert from meters to feet
        return elev * FEET_PER_METER;
    }
}

double TerrainDatabase::getElevationAt(const Position& position) {
    return getElevationAt(position.latitude, position.longitude);
}

std::vector<double> TerrainDatabase::getElevationProfile(
    const Position& start,
    const Position& end,
    int samples) {
    
    std::vector<double> profile;
    
    for (int i = 0; i <= samples; ++i) {
        double fraction = (samples > 0) ? static_cast<double>(i) / samples : 0.0;
        
        // Linear interpolation (simplified - should use great circle for accuracy)
        double lat = start.latitude + (end.latitude - start.latitude) * fraction;
        double lon = start.longitude + (end.longitude - start.longitude) * fraction;
        
        profile.push_back(getElevationAt(lat, lon));
    }
    
    return profile;
}

bool TerrainDatabase::getTerrainStats(const Position& center, double radiusNM,
                                     double& minElev, double& maxElev, 
                                     double& avgElev) {
    // Convert radius from NM to degrees (approximate)
    double radiusDeg = radiusNM / 60.0;  // 1 degree ≈ 60 NM at equator
    
    minElev = std::numeric_limits<double>::max();
    maxElev = std::numeric_limits<double>::lowest();
    double sumElev = 0.0;
    int count = 0;
    
    // Sample grid within radius
    int sampleGridSize = 20;
    for (int i = 0; i <= sampleGridSize; ++i) {
        for (int j = 0; j <= sampleGridSize; ++j) {
            double lat = center.latitude + (i - sampleGridSize / 2) * radiusDeg / (sampleGridSize / 2);
            double lon = center.longitude + (j - sampleGridSize / 2) * radiusDeg / (sampleGridSize / 2);
            
            // Check if within radius
            double dist = std::sqrt(
                (lat - center.latitude) * (lat - center.latitude) +
                (lon - center.longitude) * (lon - center.longitude)
            );
            
            if (dist <= radiusDeg && isDataAvailable(lat, lon)) {
                double elev = getElevationAt(lat, lon);
                minElev = std::min(minElev, elev);
                maxElev = std::max(maxElev, elev);
                sumElev += elev;
                count++;
            }
        }
    }
    
    if (count == 0) return false;
    
    avgElev = sumElev / count;
    return true;
}

bool TerrainDatabase::isDataAvailable(double latitude, double longitude) const {
    // SRTM covers -60 to +60 latitude, -180 to +180 longitude
    return (latitude >= -60.0 && latitude <= 60.0 &&
            longitude >= -180.0 && longitude <= 180.0);
}

void TerrainDatabase::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    tileCache_.clear();
    cacheOrder_.clear();
}

std::pair<int, int> TerrainDatabase::getCacheStats() const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    int tilesInCache = tileCache_.size();
    int cacheSizeMB = tilesInCache * TILE_SIZE_BYTES / (1024 * 1024);
    return {tilesInCache, cacheSizeMB};
}

// Private methods

std::string TerrainDatabase::getTileFilename(int tileLatitude, int tileLongitude) const {
    std::ostringstream oss;
    
    if (tileLatitude >= 0) {
        oss << "N";
    } else {
        oss << "S";
        tileLatitude = -tileLatitude;
    }
    
    if (tileLatitude < 10) oss << "0";
    oss << tileLatitude;
    
    if (tileLongitude >= 0) {
        oss << "E";
    } else {
        oss << "W";
        tileLongitude = -tileLongitude;
    }
    
    if (tileLongitude < 10) oss << "0";
    if (tileLongitude < 100) oss << "0";
    oss << tileLongitude;
    
    oss << ".hgt";
    
    return oss.str();
}

bool TerrainDatabase::loadTile(int tileLatitude, int tileLongitude) {
    std::string filename = getTileFilename(tileLatitude, tileLongitude);
    std::string filepath = srtmDataPath_ + "/" + filename;
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        // Silently return false - data not available, but not an error
        return false;
    }
    
    ElevationTile tile;
    tile.tileLatitude = tileLatitude;
    tile.tileLongitude = tileLongitude;
    tile.data.resize(SRTM_SIZE * SRTM_SIZE);
    
    // Read big-endian int16 data
    unsigned char buffer[2];
    for (int i = 0; i < SRTM_SIZE * SRTM_SIZE; ++i) {
        if (!file.read(reinterpret_cast<char*>(buffer), 2)) {
            std::cerr << "TerrainDatabase: Error reading tile " << filename << std::endl;
            return false;
        }
        tile.data[i] = bytesToInt16BE(buffer);
    }
    
    file.close();
    tile.isLoaded = true;
    tile.lastAccessed = std::chrono::system_clock::now();
    
    // Check cache size and evict if necessary
    if (tileCache_.size() >= MAX_CACHE_TILES) {
        evictOldestTile();
    }
    
    std::string tileKey = filename;
    tileCache_[tileKey] = tile;
    cacheOrder_.push_back(tileKey);
    
    return true;
}

void TerrainDatabase::evictOldestTile() {
    if (cacheOrder_.empty()) return;
    
    std::string oldestKey = cacheOrder_.front();
    cacheOrder_.erase(cacheOrder_.begin());
    tileCache_.erase(oldestKey);
}

int16_t TerrainDatabase::getRawElevation(const ElevationTile& tile, 
                                        int row, int col) const {
    if (row < 0 || row >= SRTM_SIZE || col < 0 || col >= SRTM_SIZE) {
        return 0;
    }
    return tile.data[row * SRTM_SIZE + col];
}

double TerrainDatabase::interpolateElevation(const ElevationTile& tile,
                                            double fracRow, double fracCol) const {
    // Bilinear interpolation
    int row0 = static_cast<int>(fracRow);
    int col0 = static_cast<int>(fracCol);
    int row1 = std::min(row0 + 1, SRTM_SIZE - 1);
    int col1 = std::min(col0 + 1, SRTM_SIZE - 1);
    
    double u = fracRow - row0;
    double v = fracCol - col0;
    
    double v00 = getRawElevation(tile, row0, col0);
    double v01 = getRawElevation(tile, row0, col1);
    double v10 = getRawElevation(tile, row1, col0);
    double v11 = getRawElevation(tile, row1, col1);
    
    // Bilinear interpolation formula
    double v0 = v00 * (1 - v) + v01 * v;
    double v1 = v10 * (1 - v) + v11 * v;
    double result = v0 * (1 - u) + v1 * u;
    
    return result;
}

int16_t TerrainDatabase::bytesToInt16BE(const unsigned char* bytes) {
    return (static_cast<int16_t>(bytes[0]) << 8) | bytes[1];
}

} // namespace AICopilot

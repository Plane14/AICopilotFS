/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* SRTM Loader Implementation
* SRTM HGT file format handling and elevation data management
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "srtm_loader.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <iomanip>

namespace AICopilot {

// ============================================================================
// SRTMTile Implementation
// ============================================================================

SRTMTile::SRTMTile()
    : tile_lat_(0), tile_lon_(0), is_loaded_(false) {
}

SRTMTile::SRTMTile(int latitude, int longitude)
    : tile_lat_(latitude), tile_lon_(longitude), is_loaded_(false) {
}

std::string SRTMTile::GetFileName(int latitude, int longitude) {
    std::ostringstream oss;
    
    // Determine hemisphere
    char lat_char = (latitude >= 0) ? 'N' : 'S';
    char lon_char = (longitude >= 0) ? 'E' : 'W';
    
    // Format: N/S{LAT}E/W{LON}.hgt
    oss << lat_char << std::setfill('0') << std::setw(2) << std::abs(latitude)
        << lon_char << std::setw(3) << std::abs(longitude) << ".hgt";
    
    return oss.str();
}

bool SRTMTile::ParseFileName(const std::string& filename, int& out_lat, int& out_lon) {
    // Extract filename without path
    size_t pos = filename.find_last_of("/\\");
    std::string name = (pos != std::string::npos) ? filename.substr(pos + 1) : filename;
    
    // Remove extension
    if (name.length() >= 4 && name.substr(name.length() - 4) == ".hgt") {
        name = name.substr(0, name.length() - 4);
    }
    
    // Parse format: N/S{LAT}E/W{LON}
    if (name.length() < 7) {
        return false;
    }
    
    try {
        char lat_char = name[0];
        char lon_char = name[3];
        
        int lat_val = std::stoi(name.substr(1, 2));
        int lon_val = std::stoi(name.substr(4, 3));
        
        if (lat_char == 'S') lat_val = -lat_val;
        if (lon_char == 'W') lon_val = -lon_val;
        
        out_lat = lat_val;
        out_lon = lon_val;
        return true;
    } catch (...) {
        return false;
    }
}

bool SRTMTile::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Check file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (file_size != TILE_SIZE_BYTES) {
        return false;  // Invalid file size
    }
    
    // Read all data
    data_.resize(SRTM_SIZE * SRTM_SIZE);
    std::vector<uint8_t> buffer(file_size);
    
    if (!file.read(reinterpret_cast<char*>(buffer.data()), file_size)) {
        return false;
    }
    
    // Convert big-endian to native format
    for (int i = 0; i < SRTM_SIZE * SRTM_SIZE; ++i) {
        data_[i] = BytesToInt16BE(&buffer[i * 2]);
    }
    
    is_loaded_ = true;
    return true;
}

bool SRTMTile::LoadFromCompressed(const std::string& filepath) {
    // For now, just try to load as uncompressed
    // In full implementation, would handle gzip/zip decompression
    return LoadFromFile(filepath);
}

double SRTMTile::GetElevation(double row, double col) const {
    if (!is_loaded_) {
        return 0.0;
    }
    
    // Clamp to valid range
    row = std::max(0.0, std::min(static_cast<double>(SRTM_SIZE - 1), row));
    col = std::max(0.0, std::min(static_cast<double>(SRTM_SIZE - 1), col));
    
    // Get integer and fractional parts
    int row_int = static_cast<int>(row);
    int col_int = static_cast<int>(col);
    double row_frac = row - row_int;
    double col_frac = col - col_int;
    
    // Ensure we don't access out of bounds
    row_int = std::min(row_int, SRTM_SIZE - 2);
    col_int = std::min(col_int, SRTM_SIZE - 2);
    
    // Get 4 surrounding values
    int16_t v00 = GetRawElevation(row_int, col_int);
    int16_t v10 = GetRawElevation(row_int + 1, col_int);
    int16_t v01 = GetRawElevation(row_int, col_int + 1);
    int16_t v11 = GetRawElevation(row_int + 1, col_int + 1);
    
    // Handle void values
    if (v00 == VOID_VALUE) v00 = 0;
    if (v10 == VOID_VALUE) v10 = 0;
    if (v01 == VOID_VALUE) v01 = 0;
    if (v11 == VOID_VALUE) v11 = 0;
    
    // Bilinear interpolation
    return Interpolate(v00, v10, v01, v11, col_frac, row_frac);
}

int16_t SRTMTile::GetRawElevation(int row, int col) const {
    if (!is_loaded_ || row < 0 || row >= SRTM_SIZE || col < 0 || col >= SRTM_SIZE) {
        return VOID_VALUE;
    }
    
    return data_[row * SRTM_SIZE + col];
}

bool SRTMTile::IsVoid(int row, int col) const {
    return GetRawElevation(row, col) == VOID_VALUE;
}

double SRTMTile::GetFillPercentage() const {
    if (!is_loaded_ || data_.empty()) {
        return 0.0;
    }
    
    int void_count = 0;
    for (int16_t value : data_) {
        if (value == VOID_VALUE) {
            void_count++;
        }
    }
    
    double fill = static_cast<double>(data_.size() - void_count) / data_.size();
    return fill * 100.0;
}

int16_t SRTMTile::BytesToInt16BE(const uint8_t* bytes) {
    // Big-endian to native
    return static_cast<int16_t>((static_cast<uint16_t>(bytes[0]) << 8) | bytes[1]);
}

double SRTMTile::Interpolate(double v00, double v10, double v01, double v11,
                            double fx, double fy) {
    // Bilinear interpolation
    // v00 = value at (0, 0)
    // v10 = value at (1, 0)
    // v01 = value at (0, 1)
    // v11 = value at (1, 1)
    // fx = fractional x (0-1)
    // fy = fractional y (0-1)
    
    double v0 = v00 * (1.0 - fx) + v10 * fx;
    double v1 = v01 * (1.0 - fx) + v11 * fx;
    
    return v0 * (1.0 - fy) + v1 * fy;
}

// ============================================================================
// SRTMLoader Implementation
// ============================================================================

SRTMLoader::SRTMLoader(const std::string& srtm_data_path)
    : srtm_data_path_(srtm_data_path) {
}

SRTMLoader::~SRTMLoader() {
    ClearCache();
}

double SRTMLoader::GetElevation(double latitude, double longitude) {
    // Get tile
    auto tile = GetTile(latitude, longitude);
    if (!tile || !tile->IsLoaded()) {
        return 0.0;
    }
    
    // Convert lat/lon to row/col within tile
    int tile_lat = static_cast<int>(std::floor(latitude));
    int tile_lon = static_cast<int>(std::floor(longitude));
    
    double row = (tile_lat + 1.0 - latitude) * (SRTMTile::SRTM_SIZE - 1);
    double col = (longitude - tile_lon) * (SRTMTile::SRTM_SIZE - 1);
    
    // Get elevation (meters)
    double elevation_m = tile->GetElevation(row, col);
    
    // Convert meters to feet (1 meter = 3.28084 feet)
    return elevation_m * 3.28084;
}

std::vector<double> SRTMLoader::GetElevationProfile(
    double start_lat, double start_lon,
    double end_lat, double end_lon,
    int num_samples) {
    
    std::vector<double> profile;
    
    if (num_samples < 2) {
        num_samples = 10;
    }
    
    for (int i = 0; i <= num_samples; ++i) {
        double fraction = static_cast<double>(i) / num_samples;
        
        double lat = start_lat + (end_lat - start_lat) * fraction;
        double lon = start_lon + (end_lon - start_lon) * fraction;
        
        profile.push_back(GetElevation(lat, lon));
    }
    
    return profile;
}

bool SRTMLoader::IsDataAvailable(double latitude, double longitude) const {
    int tile_lat = static_cast<int>(std::floor(latitude));
    int tile_lon = static_cast<int>(std::floor(longitude));
    
    // Check if tile is in valid SRTM range (-60 to 60 latitude)
    if (tile_lat < -60 || tile_lat >= 60) {
        return false;
    }
    
    // In full implementation, would check if file exists
    return true;
}

void SRTMLoader::ClearCache() {
    tile_cache_.clear();
}

size_t SRTMLoader::GetCacheMemoryUsage() const {
    size_t total = 0;
    for (const auto& entry : tile_cache_) {
        if (entry.second) {
            total += entry.second->GetMemoryUsage();
        }
    }
    return total;
}

std::shared_ptr<SRTMTile> SRTMLoader::LoadTile(int latitude, int longitude) {
    // Create tile filename
    std::string filename = SRTMTile::GetFileName(latitude, longitude);
    
    // Check cache first
    auto cache_key = filename;
    auto cache_it = tile_cache_.find(cache_key);
    if (cache_it != tile_cache_.end()) {
        return cache_it->second;
    }
    
    // Try to load from disk
    std::string filepath = srtm_data_path_ + "/" + filename;
    auto tile = std::make_shared<SRTMTile>(latitude, longitude);
    
    if (!tile->LoadFromFile(filepath)) {
        // Try compressed version
        if (!tile->LoadFromCompressed(filepath + ".zip")) {
            if (!tile->LoadFromCompressed(filepath + ".gz")) {
                // Create empty tile
                return tile;
            }
        }
    }
    
    // Check cache size and evict if needed
    if (tile_cache_.size() >= MAX_CACHED_TILES) {
        EvictOldestTile();
    }
    
    // Add to cache
    tile_cache_[cache_key] = tile;
    return tile;
}

void SRTMLoader::EvictOldestTile() {
    if (!tile_cache_.empty()) {
        tile_cache_.erase(tile_cache_.begin());
    }
}

std::shared_ptr<SRTMTile> SRTMLoader::GetTile(double latitude, double longitude) {
    int tile_lat = static_cast<int>(std::floor(latitude));
    int tile_lon = static_cast<int>(std::floor(longitude));
    
    return LoadTile(tile_lat, tile_lon);
}

}  // namespace AICopilot

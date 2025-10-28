/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* SRTM Loader - SRTM 30m elevation data file format support
* Handles SRTM HGT file loading, format parsing, and data management
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef SRTM_LOADER_HPP
#define SRTM_LOADER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>

namespace AICopilot {

/**
 * SRTM Data Format Specifications
 * 
 * File format: Binary HGT files with .hgt extension
 * Naming: N/S{LAT}E/W{LON}.hgt (e.g., N39W104.hgt for Denver)
 * 
 * Data layout:
 * - 3601 x 3601 samples per 1° x 1° tile
 * - Each sample is int16_t (big-endian)
 * - Row-major order: top-left to bottom-right
 * - Top row: northernmost latitude
 * - Left column: westernmost longitude
 * - Bottom row: southernmost latitude (1° south of top)
 * - Right column: easternmost longitude (1° east of left)
 * 
 * File size: 3601 * 3601 * 2 bytes = 25,934,402 bytes (25.8 MB)
 */
class SRTMTile {
public:
    /// SRTM tile dimensions and constants
    static constexpr int SRTM_SIZE = 3601;                    ///< 3601x3601 samples
    static constexpr double SRTM_RESOLUTION = 1.0 / 3600.0;   ///< 1 arc-second resolution
    static constexpr int TILE_SIZE_BYTES = 25934402;          ///< Uncompressed file size
    static constexpr int TILE_SIZE_COMPRESSED = 15000000;     ///< Typical compressed size
    
    /// Missing data value in SRTM files
    static constexpr int16_t VOID_VALUE = -32768;
    
    /**
     * Constructor - empty tile
     */
    SRTMTile();
    
    /**
     * Constructor with tile coordinates
     * @param latitude Tile latitude (floor of actual coordinates)
     * @param longitude Tile longitude (floor of actual coordinates)
     */
    SRTMTile(int latitude, int longitude);
    
    /**
     * Get tile filename from coordinates
     * @param latitude Tile latitude
     * @param longitude Tile longitude
     * @return Filename in format N/S{LAT}E/W{LON}.hgt
     */
    static std::string GetFileName(int latitude, int longitude);
    
    /**
     * Get tile coordinates from filename
     * @param filename Filename (with or without path)
     * @param out_lat Output latitude
     * @param out_lon Output longitude
     * @return true if parsed successfully
     */
    static bool ParseFileName(const std::string& filename, int& out_lat, int& out_lon);
    
    /**
     * Load tile from file
     * @param filepath Full path to HGT file
     * @return true if loaded successfully
     */
    bool LoadFromFile(const std::string& filepath);
    
    /**
     * Load tile from compressed file (gzip)
     * @param filepath Full path to HGT.zip or HGT.gz file
     * @return true if loaded successfully
     */
    bool LoadFromCompressed(const std::string& filepath);
    
    /**
     * Get elevation at fractional row/column within tile
     * Uses bilinear interpolation for sub-sample accuracy
     * 
     * @param row Fractional row (0 to 3600)
     * @param col Fractional column (0 to 3600)
     * @return Elevation in meters (or 0 if void)
     */
    double GetElevation(double row, double col) const;
    
    /**
     * Get raw elevation value at integer row/column
     * @param row Row index (0 to 3600)
     * @param col Column index (0 to 3600)
     * @return Elevation in meters (VOID_VALUE if invalid)
     */
    int16_t GetRawElevation(int row, int col) const;
    
    /**
     * Check if tile data is loaded
     * @return true if tile contains valid data
     */
    bool IsLoaded() const { return is_loaded_; }
    
    /**
     * Get tile latitude
     * @return Tile latitude (northernmost point)
     */
    int GetLatitude() const { return tile_lat_; }
    
    /**
     * Get tile longitude
     * @return Tile longitude (westernmost point)
     */
    int GetLongitude() const { return tile_lon_; }
    
    /**
     * Get memory usage
     * @return Size in bytes
     */
    size_t GetMemoryUsage() const { return data_.size() * sizeof(int16_t); }
    
    /**
     * Check if data point is void (missing)
     * @param row Row index
     * @param col Column index
     * @return true if data is void/missing
     */
    bool IsVoid(int row, int col) const;
    
    /**
     * Get fill percentage (non-void data)
     * @return Percentage of valid data (0-100)
     */
    double GetFillPercentage() const;

private:
    int tile_lat_;                      ///< Tile latitude (floor value)
    int tile_lon_;                      ///< Tile longitude (floor value)
    std::vector<int16_t> data_;         ///< 3601x3601 elevation samples (big-endian)
    bool is_loaded_;                    ///< True if data is loaded
    
    /**
     * Convert big-endian int16 to native format
     * @param bytes Raw bytes (2 bytes)
     * @return Native int16_t value
     */
    static int16_t BytesToInt16BE(const uint8_t* bytes);
    
    /**
     * Bilinear interpolation
     * @param v00 Value at (0,0)
     * @param v10 Value at (1,0)
     * @param v01 Value at (0,1)
     * @param v11 Value at (1,1)
     * @param fx Fractional x (0-1)
     * @param fy Fractional y (0-1)
     * @return Interpolated value
     */
    static double Interpolate(double v00, double v10, double v01, double v11,
                             double fx, double fy);
};

/**
 * SRTM Loader - Manages loading and caching of SRTM tiles
 */
class SRTMLoader {
public:
    /// Maximum tiles to keep in memory
    static constexpr int MAX_CACHED_TILES = 16;
    
    /**
     * Constructor
     * @param srtm_data_path Path to directory containing HGT files
     */
    explicit SRTMLoader(const std::string& srtm_data_path);
    
    /**
     * Destructor
     */
    ~SRTMLoader();
    
    /**
     * Get elevation at latitude/longitude
     * Automatically loads tiles as needed
     * 
     * @param latitude Latitude in degrees (-60 to 60 for SRTM1)
     * @param longitude Longitude in degrees (-180 to 180)
     * @return Elevation in meters MSL, or 0 if data unavailable
     */
    double GetElevation(double latitude, double longitude);
    
    /**
     * Get elevation profile along a path
     * @param start_lat Starting latitude
     * @param start_lon Starting longitude
     * @param end_lat Ending latitude
     * @param end_lon Ending longitude
     * @param num_samples Number of samples along path
     * @return Vector of elevations (meters)
     */
    std::vector<double> GetElevationProfile(
        double start_lat, double start_lon,
        double end_lat, double end_lon,
        int num_samples = 10);
    
    /**
     * Check if SRTM data is available for location
     * @param latitude Latitude
     * @param longitude Longitude
     * @return true if SRTM file exists for this location
     */
    bool IsDataAvailable(double latitude, double longitude) const;
    
    /**
     * Clear tile cache
     */
    void ClearCache();
    
    /**
     * Get cache size
     * @return Number of cached tiles
     */
    int GetCacheSize() const { return tile_cache_.size(); }
    
    /**
     * Get cache memory usage
     * @return Total bytes used by cached tiles
     */
    size_t GetCacheMemoryUsage() const;
    
    /**
     * Get data path
     * @return Path to SRTM data directory
     */
    const std::string& GetDataPath() const { return srtm_data_path_; }

private:
    std::string srtm_data_path_;                    ///< Path to SRTM data directory
    std::map<std::string, std::shared_ptr<SRTMTile>> tile_cache_;  ///< LRU tile cache
    
    /**
     * Load tile from cache or disk
     * @param latitude Tile latitude
     * @param longitude Tile longitude
     * @return Pointer to loaded tile, or nullptr if load failed
     */
    std::shared_ptr<SRTMTile> LoadTile(int latitude, int longitude);
    
    /**
     * Evict oldest cached tile if needed
     */
    void EvictOldestTile();
    
    /**
     * Get tile for a coordinate
     * @param latitude Latitude
     * @param longitude Longitude
     * @return Pointer to loaded tile
     */
    std::shared_ptr<SRTMTile> GetTile(double latitude, double longitude);
};

/**
 * SRTM Data Exception
 */
class SRTMException : public std::runtime_error {
public:
    explicit SRTMException(const std::string& message)
        : std::runtime_error(message) {}
};

}  // namespace AICopilot

#endif  // SRTM_LOADER_HPP

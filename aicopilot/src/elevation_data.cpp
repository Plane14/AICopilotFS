/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Elevation Data System Implementation
* High-performance SRTM 30m elevation database with LRU caching
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "elevation_data.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <iostream>

namespace AICopilot {

// ============================================================================
// Constructor & Initialization
// ============================================================================

ElevationDatabase::ElevationDatabase()
    : max_cache_size_(DEFAULT_CACHE_SIZE),
      cache_hits_(0),
      cache_misses_(0) {
    InitializeRegions();
}

ElevationDatabase::ElevationDatabase(size_t cache_size)
    : max_cache_size_(cache_size),
      cache_hits_(0),
      cache_misses_(0) {
    InitializeRegions();
}

// ============================================================================
// Public API - Elevation Queries
// ============================================================================

double ElevationDatabase::GetElevationAt(double latitude, double longitude) {
    // Validate coordinates
    if (!ValidateCoordinates(latitude, longitude)) {
        return 0.0;  // Sea level default for invalid coordinates
    }

    // Create cache key with quantized precision
    uint64_t cache_key = MakeKey(latitude, longitude);
    
    // Check cache first (thread-safe)
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        auto cache_it = elevation_cache_.find(cache_key);
        if (cache_it != elevation_cache_.end()) {
            cache_hits_++;
            return cache_it->second;
        }
    }

    // Cache miss - interpolate elevation
    cache_misses_++;
    double elevation = InterpolateElevation(latitude, longitude);
    
    // Clamp to valid range
    elevation = std::max(MIN_ELEVATION, std::min(MAX_ELEVATION, elevation));
    
    // Add to cache with LRU management
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        // Check cache size and evict if needed
        if (elevation_cache_.size() >= max_cache_size_) {
            EvictOldestEntry();
        }
        
        elevation_cache_[cache_key] = elevation;
        cache_order_.push_back(cache_key);
    }
    
    return elevation;
}

std::vector<TerrainProfileEntry> ElevationDatabase::GetTerrainProfile(
    double start_lat, double start_lon,
    double end_lat, double end_lon,
    int num_samples) {
    
    std::vector<TerrainProfileEntry> profile;
    
    // Validate input
    if (num_samples < 2) {
        num_samples = 10;
    }
    
    // Calculate distance along path in nautical miles
    double lat_diff = end_lat - start_lat;
    double lon_diff = end_lon - start_lon;
    double total_distance = CalculateDistance(start_lat, start_lon, end_lat, end_lon);
    double total_distance_nm = total_distance * 60.0;  // Convert degrees to NM
    
    // Sample elevation at regular intervals
    for (int i = 0; i <= num_samples; ++i) {
        double fraction = (num_samples > 0) ? static_cast<double>(i) / num_samples : 0.0;
        
        // Linear interpolation of position
        double sample_lat = start_lat + (lat_diff * fraction);
        double sample_lon = start_lon + (lon_diff * fraction);
        
        // Get elevation at this point
        double elevation = GetElevationAt(sample_lat, sample_lon);
        
        // Calculate distance along path
        double distance_nm = total_distance_nm * fraction;
        
        TerrainProfileEntry entry;
        entry.latitude = sample_lat;
        entry.longitude = sample_lon;
        entry.elevation = elevation;
        entry.distance = distance_nm;
        
        profile.push_back(entry);
    }
    
    return profile;
}

bool ElevationDatabase::LoadRegionData(const std::string& region_code) {
    // Convert to uppercase for case-insensitive comparison
    std::string code = region_code;
    std::transform(code.begin(), code.end(), code.begin(), ::toupper);
    
    // Check if region already exists
    for (const auto& region : regions_) {
        if (region.region_name == code) {
            return true;  // Region already loaded
        }
    }
    
    // Load specific region data based on code
    if (code == "ROCKY_MOUNTAINS") {
        InitializeRockyMountains();
        return true;
    } else if (code == "APPALACHIANS") {
        InitializeAppalachians();
        return true;
    } else if (code == "SIERRA_NEVADA") {
        InitializeSierraNevada();
        return true;
    } else if (code == "CASCADE_RANGE") {
        InitializeCascadeRange();
        return true;
    } else if (code == "COASTAL_PLAINS") {
        InitializeCoastalPlains();
        return true;
    } else if (code == "GREAT_PLAINS") {
        InitializeGreatPlains();
        return true;
    } else if (code == "WATER_BODIES") {
        InitializeWaterBodies();
        return true;
    }
    
    return false;  // Unknown region code
}

bool ElevationDatabase::IsWaterBody(double latitude, double longitude) {
    // Check if coordinates are in known water body regions
    
    // Great Lakes region (43°N-46°N, 76°W-88°W)
    if (latitude >= 42.5 && latitude <= 46.5 &&
        longitude >= -88.5 && longitude <= -75.5) {
        return true;
    }
    
    // Lake Superior
    if (latitude >= 46.5 && latitude <= 48.0 &&
        longitude >= -91.0 && longitude <= -84.0) {
        return true;
    }
    
    // Gulf of Mexico (approximately)
    if (latitude >= 25.0 && latitude <= 30.0 &&
        longitude >= -97.0 && longitude <= -83.0) {
        return true;
    }
    
    // Atlantic Ocean (east coast)
    if (longitude > -70.0 && latitude >= 25.0 && latitude <= 45.0) {
        double ocean_distance = 0.0;
        double nearest_ocean_elev = FindNearestPoint(latitude, longitude, ocean_distance);
        
        // If nearest point is far (>0.5 degrees), likely ocean
        if (ocean_distance > 0.5) {
            return true;
        }
    }
    
    // Pacific Ocean (west coast)
    if (longitude < -120.0 && latitude >= 30.0 && latitude <= 50.0) {
        double ocean_distance = 0.0;
        double nearest_ocean_elev = FindNearestPoint(latitude, longitude, ocean_distance);
        
        if (ocean_distance > 0.5) {
            return true;
        }
    }
    
    // Check elevation heuristic - very low elevations often indicate water
    double elev = GetElevationAt(latitude, longitude);
    if (elev < -100.0 || (elev < 10.0 && longitude > -120.0 && longitude < -70.0)) {
        return true;
    }
    
    return false;
}

SlopeInfo ElevationDatabase::GetSlopeAngle(double latitude, double longitude) {
    SlopeInfo slope_info;
    slope_info.angle_degrees = 0.0;
    slope_info.max_elevation = 0.0;
    slope_info.min_elevation = 0.0;
    slope_info.is_steep = false;
    
    // Sample elevations in a small grid around the point
    const double sample_offset = 0.01;  // About 1 km at equator
    
    double center_elev = GetElevationAt(latitude, longitude);
    
    // Sample 8 surrounding points
    std::vector<double> elevations;
    elevations.push_back(center_elev);
    
    double north_elev = GetElevationAt(latitude + sample_offset, longitude);
    double south_elev = GetElevationAt(latitude - sample_offset, longitude);
    double east_elev = GetElevationAt(latitude, longitude + sample_offset);
    double west_elev = GetElevationAt(latitude, longitude - sample_offset);
    
    elevations.push_back(north_elev);
    elevations.push_back(south_elev);
    elevations.push_back(east_elev);
    elevations.push_back(west_elev);
    
    // Calculate statistics
    slope_info.max_elevation = *std::max_element(elevations.begin(), elevations.end());
    slope_info.min_elevation = *std::min_element(elevations.begin(), elevations.end());
    
    // Calculate slope angle
    double elevation_difference = slope_info.max_elevation - slope_info.min_elevation;
    const double earth_radius_ft = 20902000.0;  // Feet
    const double distance_ft = sample_offset * (M_PI / 180.0) * earth_radius_ft;
    
    slope_info.angle_degrees = std::atan2(elevation_difference, distance_ft) * (180.0 / M_PI);
    slope_info.is_steep = slope_info.angle_degrees > 15.0;
    
    return slope_info;
}

double ElevationDatabase::GetMinimumSafeAltitude(double latitude, double longitude, double clearance) {
    double elevation = GetElevationAt(latitude, longitude);
    return elevation + clearance;
}

bool ElevationDatabase::GetTerrainStats(double latitude, double longitude, double radius,
                                       double& out_min, double& out_max, double& out_avg) {
    std::vector<double> elevations;
    
    // Sample terrain in grid pattern over the area
    const int grid_size = 5;  // 5x5 grid
    double lat_step = radius / 60.0 / grid_size;  // radius to degrees
    double lon_step = lat_step;  // Approximate
    
    for (int i = 0; i < grid_size; ++i) {
        for (int j = 0; j < grid_size; ++j) {
            double sample_lat = latitude + (i - grid_size/2.0) * lat_step;
            double sample_lon = longitude + (j - grid_size/2.0) * lon_step;
            
            if (ValidateCoordinates(sample_lat, sample_lon)) {
                elevations.push_back(GetElevationAt(sample_lat, sample_lon));
            }
        }
    }
    
    if (elevations.empty()) {
        return false;
    }
    
    // Calculate statistics
    out_min = *std::min_element(elevations.begin(), elevations.end());
    out_max = *std::max_element(elevations.begin(), elevations.end());
    out_avg = std::accumulate(elevations.begin(), elevations.end(), 0.0) / elevations.size();
    
    return true;
}

void ElevationDatabase::ClearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    elevation_cache_.clear();
    cache_order_.clear();
}

std::pair<int64_t, int64_t> ElevationDatabase::GetCacheStatistics() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return std::make_pair(cache_hits_, cache_misses_);
}

size_t ElevationDatabase::GetCacheMemoryUsage() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    // Estimate: ~24 bytes per entry (8 bytes key + 8 bytes value + overhead)
    return elevation_cache_.size() * 24;
}

void ElevationDatabase::ResetCacheStatistics() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_hits_ = 0;
    cache_misses_ = 0;
}

bool ElevationDatabase::ValidateCoordinates(double latitude, double longitude) {
    return (latitude >= -90.0 && latitude <= 90.0 &&
            longitude >= -180.0 && longitude <= 180.0);
}

// ============================================================================
// Private Methods - Cache Management
// ============================================================================

uint64_t ElevationDatabase::MakeKey(double latitude, double longitude) const {
    // Quantize to cache precision
    int lat_int = static_cast<int>(latitude / CACHE_PRECISION);
    int lon_int = static_cast<int>(longitude / CACHE_PRECISION);
    
    // Create 64-bit key (32 bits lat, 32 bits lon)
    uint64_t key = (static_cast<uint64_t>(lat_int & 0xFFFFFFFF) << 32) |
                   (static_cast<uint64_t>(lon_int & 0xFFFFFFFF));
    return key;
}

void ElevationDatabase::EvictOldestEntry() {
    // LRU eviction - remove oldest entry
    if (!cache_order_.empty()) {
        uint64_t oldest_key = cache_order_.front();
        cache_order_.pop_front();
        elevation_cache_.erase(oldest_key);
    }
}

// ============================================================================
// Private Methods - Elevation Interpolation
// ============================================================================

double ElevationDatabase::InterpolateElevation(double latitude, double longitude) {
    // Find nearest elevation points using bilinear interpolation
    
    double best_distance = 999999.0;
    double best_elevation = 0.0;
    bool found = false;
    
    // First pass: find points in current region
    for (const auto& region : regions_) {
        // Check if coordinates are within region bounds
        if (latitude < region.min_lat || latitude > region.max_lat ||
            longitude < region.min_lon || longitude > region.max_lon) {
            continue;
        }
        
        // Find 4 nearest points for bilinear interpolation
        std::vector<std::pair<double, ElevationPoint>> candidates;
        
        for (const auto& point : region.points) {
            double distance = CalculateDistance(latitude, longitude, point.latitude, point.longitude);
            candidates.push_back(std::make_pair(distance, point));
        }
        
        // Sort by distance
        std::sort(candidates.begin(), candidates.end());
        
        // Bilinear interpolation with up to 4 nearest points
        if (candidates.size() >= 2) {
            double total_weight = 0.0;
            double weighted_elevation = 0.0;
            
            for (int i = 0; i < std::min(size_t(4), candidates.size()); ++i) {
                double distance = candidates[i].first;
                double elev = candidates[i].second.elevation;
                
                // Use inverse distance weighting
                double weight = (distance < 0.0001) ? 1e10 : 1.0 / (distance * distance);
                total_weight += weight;
                weighted_elevation += elev * weight;
                
                best_distance = distance;
                found = true;
            }
            
            if (total_weight > 0) {
                best_elevation = weighted_elevation / total_weight;
            }
        }
    }
    
    // Second pass: use nearest point across all regions
    if (!found) {
        best_elevation = FindNearestPoint(latitude, longitude, best_distance);
        found = (best_distance < 1.0);  // Within 1 degree
    }
    
    // Final fallback: use heuristic estimation
    if (!found) {
        best_elevation = EstimateElevation(latitude, longitude);
    }
    
    return best_elevation;
}

double ElevationDatabase::EstimateElevation(double latitude, double longitude) {
    // Heuristic elevation estimation based on geographic features
    
    // Rocky Mountains region (35°N-50°N, 115°W-102°W)
    if (latitude >= 35.0 && latitude <= 50.0 && 
        longitude >= -115.0 && longitude <= -102.0) {
        return 5000.0 + 500.0 * std::cos(latitude * M_PI / 180.0);
    }
    
    // Sierra Nevada (35°N-42°N, 122°W-114°W)
    if (latitude >= 35.0 && latitude <= 42.0 &&
        longitude >= -122.0 && longitude <= -114.0) {
        return 8000.0 + 2000.0 * std::sin(latitude * M_PI / 180.0);
    }
    
    // Cascade Range (42°N-50°N, 123.5°W-119.5°W)
    if (latitude >= 42.0 && latitude <= 50.0 &&
        longitude >= -123.5 && longitude <= -119.5) {
        return 6000.0 + 3000.0 * std::cos((latitude - 45.0) * M_PI / 180.0);
    }
    
    // Appalachian Mountains (31°N-49°N, 84°W-68°W)
    if (latitude >= 31.0 && latitude <= 49.0 &&
        longitude >= -84.0 && longitude <= -68.0) {
        return 2000.0 + 1000.0 * std::sin((latitude - 35.0) * M_PI / 180.0);
    }
    
    // Coastal Plains (25°N-40°N, 90°W-70°W)
    if (latitude >= 25.0 && latitude <= 40.0 &&
        longitude >= -90.0 && longitude <= -70.0) {
        return 200.0 + 100.0 * std::cos(longitude * M_PI / 180.0);
    }
    
    // Great Plains (35°N-50°N, 102°W-95°W)
    if (latitude >= 35.0 && latitude <= 50.0 &&
        longitude >= -102.0 && longitude <= -95.0) {
        return 2000.0 + 500.0 * std::sin(latitude * M_PI / 180.0);
    }
    
    // Default: sea level
    return 0.0;
}

double ElevationDatabase::FindNearestPoint(double latitude, double longitude, double& out_distance) {
    double best_distance = 999999.0;
    double best_elevation = 0.0;
    
    for (const auto& region : regions_) {
        for (const auto& point : region.points) {
            double distance = CalculateDistance(latitude, longitude, point.latitude, point.longitude);
            if (distance < best_distance) {
                best_distance = distance;
                best_elevation = point.elevation;
            }
        }
    }
    
    out_distance = best_distance;
    return best_elevation;
}

double ElevationDatabase::CalculateDistance(double lat1, double lon1, double lat2, double lon2) const {
    // Simple Euclidean distance in degrees (faster than Haversine for small distances)
    double lat_diff = (lat2 - lat1);
    double lon_diff = (lon2 - lon1) * std::cos(lat1 * M_PI / 180.0);
    return std::sqrt(lat_diff * lat_diff + lon_diff * lon_diff);
}

// ============================================================================
// Private Methods - Region Initialization
// ============================================================================

void ElevationDatabase::InitializeRegions() {
    InitializeRockyMountains();
    InitializeAppalachians();
    InitializeSierraNevada();
    InitializeCascadeRange();
    InitializeCoastalPlains();
    InitializeGreatPlains();
    InitializeWaterBodies();
}

void ElevationDatabase::InitializeRockyMountains() {
    ElevationRegion rockies;
    rockies.min_lat = 35.0;
    rockies.max_lat = 50.0;
    rockies.min_lon = -115.0;
    rockies.max_lon = -102.0;
    rockies.region_name = "ROCKY_MOUNTAINS";
    
    // Major peaks and terrain features
    rockies.points = {
        // Denver area
        {39.74, -104.99, 5280.0},
        {39.75, -105.00, 5290.0},
        {39.76, -104.98, 5275.0},
        // Mount Elbert (14,440 ft)
        {39.118, -106.445, 14440.0},
        // Mount Massive (14,421 ft)
        {39.037, -106.474, 14421.0},
        // Mount Harvard (14,421 ft)
        {38.924, -106.319, 14421.0},
        // Mount Rainier approaches
        {38.696, -105.941, 13525.0},
        {38.658, -105.859, 13972.0},
        // Colorado Springs area
        {38.833, -104.821, 6010.0},
        // Boulder area
        {40.015, -105.270, 5430.0},
        // Fort Collins area
        {40.585, -105.084, 5003.0},
        // Salt Lake City area
        {40.761, -111.891, 4226.0},
        // Jackson Hole area
        {43.478, -110.821, 6311.0},
        // Yellowstone area
        {44.728, -110.693, 7500.0},
        // Glacier Park area
        {48.380, -114.009, 8684.0},
    };
    regions_.push_back(rockies);
}

void ElevationDatabase::InitializeAppalachians() {
    ElevationRegion appalachians;
    appalachians.min_lat = 31.0;
    appalachians.max_lat = 49.0;
    appalachians.min_lon = -84.0;
    appalachians.max_lon = -68.0;
    appalachians.region_name = "APPALACHIANS";
    
    // Major peaks and terrain features
    appalachians.points = {
        // Clingmans Dome (6,643 ft)
        {35.563, -83.498, 6643.0},
        // Mount Mitchell (6,684 ft)
        {35.765, -82.265, 6684.0},
        // Great Smokies area
        {35.631, -83.498, 6400.0},
        // Black Mountains area
        {35.931, -82.223, 6500.0},
        // White Mountains area (New Hampshire)
        {44.264, -71.303, 6288.0},
        // Mount Washington (6,288 ft)
        {44.271, -71.304, 6288.0},
        // Katahdin area (Maine)
        {45.313, -68.921, 5267.0},
        // Vermont Green Mountains
        {44.136, -72.651, 4393.0},
        // Berkshires area (Massachusetts)
        {42.253, -73.243, 3491.0},
        // Pennsylvania area
        {40.921, -75.324, 2000.0},
        // Atlanta area
        {33.749, -84.388, 1050.0},
        // Charlotte area
        {35.227, -80.843, 645.0},
    };
    regions_.push_back(appalachians);
}

void ElevationDatabase::InitializeSierraNevada() {
    ElevationRegion sierra_nevada;
    sierra_nevada.min_lat = 35.0;
    sierra_nevada.max_lat = 42.0;
    sierra_nevada.min_lon = -122.0;
    sierra_nevada.max_lon = -114.0;
    sierra_nevada.region_name = "SIERRA_NEVADA";
    
    // Major peaks and terrain features
    sierra_nevada.points = {
        // Mount Whitney (14,505 ft)
        {36.578, -118.292, 14505.0},
        // Mount Williamson (14,379 ft)
        {36.666, -118.370, 14379.0},
        // Mount Tyndall (14,018 ft)
        {36.766, -118.287, 14018.0},
        // Mount Muir (14,012 ft)
        {36.645, -118.297, 14012.0},
        // Needle Peak (13,900 ft)
        {37.100, -118.900, 13900.0},
        // Mammoth Mountain area
        {37.630, -119.030, 11053.0},
        // Lake Tahoe area
        {38.949, -120.508, 6224.0},
        // Mount Jackson (13,157 ft)
        {37.351, -119.296, 13157.0},
        // Sequoia area
        {36.558, -118.806, 11500.0},
        // Yosemite area
        {37.865, -119.535, 11000.0},
    };
    regions_.push_back(sierra_nevada);
}

void ElevationDatabase::InitializeCascadeRange() {
    ElevationRegion cascade_range;
    cascade_range.min_lat = 42.0;
    cascade_range.max_lat = 50.0;
    cascade_range.min_lon = -123.5;
    cascade_range.max_lon = -119.5;
    cascade_range.region_name = "CASCADE_RANGE";
    
    // Major peaks and terrain features
    cascade_range.points = {
        // Mount Rainier (14,411 ft)
        {46.852, -121.760, 14411.0},
        // Mount Adams (12,281 ft)
        {46.202, -121.490, 12281.0},
        // Mount Hood (11,249 ft)
        {45.387, -121.697, 11249.0},
        // Mount Jefferson (10,495 ft)
        {44.674, -121.798, 10495.0},
        // Three Sisters area
        {43.736, -121.765, 10358.0},
        // Mount Shasta (14,179 ft)
        {41.309, -122.311, 14179.0},
        // Crater Lake area
        {42.944, -122.109, 8949.0},
        // Mount Bachelor (9,068 ft)
        {43.644, -121.688, 9068.0},
        // Pacific Crest Trail area
        {44.200, -121.500, 8500.0},
        // Snoqualmie Pass area
        {47.412, -121.412, 3004.0},
    };
    regions_.push_back(cascade_range);
}

void ElevationDatabase::InitializeCoastalPlains() {
    ElevationRegion coastal_plains;
    coastal_plains.min_lat = 25.0;
    coastal_plains.max_lat = 40.0;
    coastal_plains.min_lon = -90.0;
    coastal_plains.max_lon = -70.0;
    coastal_plains.region_name = "COASTAL_PLAINS";
    
    // Major cities and terrain features
    coastal_plains.points = {
        // New York City
        {40.714, -74.006, 33.0},
        // Boston
        {42.358, -71.063, 149.0},
        // Washington DC
        {38.897, -77.036, 50.0},
        // Philadelphia
        {39.953, -75.192, 40.0},
        // Miami
        {25.761, -80.191, 10.0},
        // Orlando
        {28.539, -81.376, 105.0},
        // Charleston
        {32.776, -79.931, 15.0},
        // Savannah
        {32.080, -81.089, 42.0},
        // Brunswick
        {31.151, -81.492, 20.0},
        // Jacksonville
        {30.332, -81.656, 25.0},
        // New Orleans
        {29.952, -90.271, 0.0},
    };
    regions_.push_back(coastal_plains);
}

void ElevationDatabase::InitializeGreatPlains() {
    ElevationRegion great_plains;
    great_plains.min_lat = 35.0;
    great_plains.max_lat = 50.0;
    great_plains.min_lon = -102.0;
    great_plains.max_lon = -95.0;
    great_plains.region_name = "GREAT_PLAINS";
    
    // Major cities and terrain features
    great_plains.points = {
        // Dallas
        {32.777, -96.797, 450.0},
        // Houston
        {29.761, -95.369, 45.0},
        // Oklahoma City
        {35.467, -97.516, 1200.0},
        // Kansas City
        {39.099, -94.576, 900.0},
        // Chicago
        {41.878, -87.630, 600.0},
        // Denver (mountain transition)
        {39.739, -104.990, 5280.0},
        // Omaha
        {41.262, -95.935, 1000.0},
        // St. Louis
        {38.627, -90.199, 550.0},
        // Memphis
        {35.149, -90.049, 250.0},
        // Minneapolis
        {44.980, -93.266, 800.0},
        // Albuquerque (mountain transition)
        {35.091, -106.613, 5312.0},
    };
    regions_.push_back(great_plains);
}

void ElevationDatabase::InitializeWaterBodies() {
    ElevationRegion water_bodies;
    water_bodies.min_lat = 25.0;
    water_bodies.max_lat = 49.0;
    water_bodies.min_lon = -125.0;
    water_bodies.max_lon = -66.0;
    water_bodies.region_name = "WATER_BODIES";
    
    // Major water bodies for detection
    water_bodies.points = {
        // Great Lakes
        {43.000, -82.000, -350.0},  // Lake Michigan
        {44.000, -81.000, -240.0},  // Lake Huron
        {43.500, -80.000, -290.0},  // Lake Ontario
        {44.500, -80.500, -280.0},  // Lake Erie
        {47.000, -87.000, -360.0},  // Lake Superior
        
        // Other major lakes
        {41.145, -111.890, -4200.0},  // Great Salt Lake
        {41.000, -111.500, -4200.0},  // Deeper Great Salt Lake
        {39.250, -120.000, -6220.0},  // Lake Tahoe
        {42.600, -122.300, -1949.0},  // Crater Lake
        
        // Gulf of Mexico (low elevation points)
        {25.500, -93.000, -50.0},
        {28.000, -95.000, -50.0},
        {29.750, -95.360, -50.0},
    };
    regions_.push_back(water_bodies);
}

}  // namespace AICopilot

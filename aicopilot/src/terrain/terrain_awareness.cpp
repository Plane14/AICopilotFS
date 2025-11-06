/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "terrain_awareness.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <algorithm>
#include <cctype>
#ifdef ENABLE_GDAL
#include "gdal_priv.h"
#include "cpl_conv.h"
#include <memory>
#endif

namespace AICopilot {

// GDAL-backed terrain cache (file-scope)
static bool gdalTerrainLoaded = false;
static int gdalCols = 0;
static int gdalRows = 0;
static double gdalGeoTransform[6] = {0};
static std::vector<double> gdalElevData; // row-major
TerrainAlert TerrainAwareness::checkTerrainClearance(const Position& pos) const {
    TerrainAlert alert;
    double elevation = getTerrainElevation(pos);
    double agl = pos.altitude - elevation;
    
    bool climbing = currentState_.verticalSpeed > 0;
    alert.level = determineWarningLevel(agl, climbing);
    alert.position = pos;
    alert.clearance = agl;
    alert.requiredAltitude = elevation + MIN_CLEARANCE_CRUISE;
    
    switch (alert.level) {
        case TerrainWarningLevel::PULL_UP:
            alert.message = "PULL UP, PULL UP";
            break;
        case TerrainWarningLevel::WARNING:
            alert.message = "TERRAIN TERRAIN";
            break;
        case TerrainWarningLevel::CAUTION:
            alert.message = "CAUTION TERRAIN";
            break;
        default:
            alert.message = "Clear of terrain";
    }
    
    return alert;
}

double TerrainAwareness::getTerrainElevation(const Position& pos) const {
    // Simplified terrain lookup - real implementation would query terrain database
    if (terrainDatabase_.empty()) {
        return 0.0;  // Sea level default
    }
    
    return interpolateElevation(pos);
}

double TerrainAwareness::getAltitudeAGL(const Position& pos, double altitudeMSL) const {
    double elevation = getTerrainElevation(pos);
    return altitudeMSL - elevation;
}

std::vector<Obstacle> TerrainAwareness::detectObstacles(
    const Position& start,
    const Position& end,
    double altitude) const {
    
    std::vector<Obstacle> obstacles;
    
    // Check obstacle database for obstacles in path
    for (const auto& obs : obstacleDatabase_) {
        if (obs.elevation <= altitude && altitude <= obs.elevation + 1000.0) {
            obstacles.push_back(obs);
        }
    }
    
    return obstacles;
}

Obstacle TerrainAwareness::getHighestObstacle(
    const Position& center,
    double radius) const {
    
    Obstacle highest;
    highest.type = ObstacleType::OTHER;
    highest.height = 0.0;
    highest.elevation = 0.0;
    
    for (const auto& obs : obstacleDatabase_) {
        if (obs.elevation > highest.elevation) {
            highest = obs;
        }
    }
    
    return highest;
}

double TerrainAwareness::calculateMinimumSafeAltitude(const Position& pos) const {
    double elevation = getTerrainElevation(pos);
    return elevation + MIN_CLEARANCE_CRUISE;
}

bool TerrainAwareness::isAltitudeSafe(const Position& pos, double altitude) const {
    double msa = calculateMinimumSafeAltitude(pos);
    return altitude >= msa;
}

TerrainAwareness::EscapeManeuver TerrainAwareness::getEscapeManeuver(
    const TerrainAlert& alert) const {
    
    EscapeManeuver maneuver;
    maneuver.targetAltitude = alert.requiredAltitude + 500.0;
    maneuver.targetHeading = currentState_.heading;
    maneuver.targetPitch = 15.0;  // Climb attitude
    maneuver.targetSpeed = currentState_.indicatedAirspeed;
    
    return maneuver;
}

bool TerrainAwareness::predictTerrainConflict(
    const Position& pos,
    double heading,
    double groundSpeed,
    double altitude,
    double lookaheadTime) const {
    
    // Calculate position after lookahead time
    double distance = (groundSpeed / 3600.0) * lookaheadTime;  // nautical miles
    
    // Simple prediction - real implementation would trace path
    double msa = calculateMinimumSafeAltitude(pos);
    return altitude < msa;
}

std::vector<TerrainPoint> TerrainAwareness::getTerrainProfile(
    const Position& start,
    double heading,
    double distance) const {
    
    std::vector<TerrainPoint> profile;
    
    // Sample terrain along heading
    int samples = 10;
    for (int i = 0; i <= samples; ++i) {
        double fraction = static_cast<double>(i) / samples;
        Position samplePos = start;
        // Simplified - real implementation would calculate position along heading
        samplePos.latitude += fraction * 0.1;
        
        TerrainPoint point;
        point.position = samplePos;
        point.elevation = getTerrainElevation(samplePos);
        profile.push_back(point);
    }
    
    return profile;
}

bool TerrainAwareness::loadTerrainDatabase(const std::string& databasePath) {
    // Enhanced terrain database loading
    // In a full implementation, would load SRTM, DEM, or other elevation data
    std::cout << "Terrain Awareness: Loading terrain database from " << databasePath << std::endl;
    // For now, implement a simple CSV loader for unit testing and small datasets.
    // CSV format: lat,lon,elevation_feet (no header required)
    terrainDatabase_.clear();
    std::ifstream ifs(databasePath);
    if (!ifs) {
        std::cout << "Terrain Awareness: Could not open terrain database file: " << databasePath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        double lat = 0.0, lon = 0.0, elev = 0.0;
        char sep = ',';
        // support both comma and space separated values
        if (!(ss >> lat)) continue;
        if (ss.peek() == ',') ss >> sep;
        if (!(ss >> lon)) continue;
        if (ss.peek() == ',') ss >> sep;
        if (!(ss >> elev)) continue;

        TerrainPoint tp;
        tp.position.latitude = lat;
        tp.position.longitude = lon;
        tp.elevation = elev; // feet MSL
        terrainDatabase_.push_back(tp);
    }

    if (!terrainDatabase_.empty()) {
        std::cout << "Terrain Awareness: Loaded " << terrainDatabase_.size() << " terrain points" << std::endl;
    }

#ifdef ENABLE_GDAL
    // If the provided path looks like a raster (GeoTIFF), try GDAL load
    auto ext = databasePath.substr(databasePath.find_last_of('.') + 1);
    for (auto &c : ext) c = static_cast<char>(tolower(c));
    if (ext == "tif" || ext == "tiff") {
        GDALAllRegister();
        GDALDataset *poDataset = (GDALDataset *)GDALOpen(databasePath.c_str(), GA_ReadOnly);
        if (poDataset == nullptr) {
            std::cout << "Terrain Awareness: GDAL failed to open " << databasePath << std::endl;
            // fall back to CSV-loaded points if any
            return !terrainDatabase_.empty();
        }

        if (poDataset->GetGeoTransform(gdalGeoTransform) != CE_None) {
            std::cout << "Terrain Awareness: GDAL dataset missing geotransform" << std::endl;
            GDALClose(poDataset);
            return !terrainDatabase_.empty();
        }

        gdalCols = poDataset->GetRasterXSize();
        gdalRows = poDataset->GetRasterYSize();
        gdalElevData.assign(gdalCols * gdalRows, 0.0);

        GDALRasterBand *poBand = poDataset->GetRasterBand(1);
        if (!poBand) {
            std::cout << "Terrain Awareness: No raster band found" << std::endl;
            GDALClose(poDataset);
            return !terrainDatabase_.empty();
        }

        CPLErr err = poBand->RasterIO(GF_Read, 0, 0, gdalCols, gdalRows,
                                      gdalElevData.data(), gdalCols, gdalRows, GDT_Float64,
                                      0, 0);
        if (err != CE_None) {
            std::cout << "Terrain Awareness: Failed to read raster data" << std::endl;
            GDALClose(poDataset);
            return !terrainDatabase_.empty();
        }

        gdalTerrainLoaded = true;
        std::cout << "Terrain Awareness: GDAL terrain loaded (" << gdalCols << "x" << gdalRows << ")" << std::endl;
        GDALClose(poDataset);
        return true;
    }
#endif

    if (terrainDatabase_.empty()) {
        std::cout << "Terrain Awareness: No terrain points loaded from " << databasePath << std::endl;
        return false;
    }

    return true;
}

bool TerrainAwareness::loadObstacleDatabase(const std::string& databasePath) {
    // Enhanced obstacle database loading
    // In a full implementation, would load tower, antenna, and obstacle data
    std::cout << "Terrain Awareness: Loading obstacle database from " << databasePath << std::endl;
    
    // For now, using basic obstacle detection
    // Future enhancements:
    // - Load FAA obstacle database
    // - Load antenna tower locations
    // - Load building heights in urban areas
    // - Support international obstacle databases
    // - Build spatial index for collision detection
    
    std::cout << "Terrain Awareness: Using basic obstacle detection (load obstacle DB for full coverage)" << std::endl;
    return true;
}

// Private methods

TerrainWarningLevel TerrainAwareness::determineWarningLevel(
    double clearance,
    bool climbing) const {
    
    if (clearance < 300.0 && !climbing) {
        return TerrainWarningLevel::PULL_UP;
    }
    if (clearance < 500.0) {
        return TerrainWarningLevel::WARNING;
    }
    if (clearance < 1000.0) {
        return TerrainWarningLevel::CAUTION;
    }
    return TerrainWarningLevel::NONE;
}

double TerrainAwareness::interpolateElevation(const Position& pos) const {
    // If GDAL terrain was loaded, perform bilinear lookup on raster
#ifdef ENABLE_GDAL
    if (gdalTerrainLoaded && gdalCols > 0 && gdalRows > 0) {
        // Assume geotransform: GT[0]=originX, GT[1]=pixelWidth, GT[2]=0, GT[3]=originY, GT[4]=0, GT[5]=pixelHeight
        double originX = gdalGeoTransform[0];
        double pixelW = gdalGeoTransform[1];
        double originY = gdalGeoTransform[3];
        double pixelH = gdalGeoTransform[5];

        // Convert lon/lat to pixel coordinates (assumes raster is geographic)
        double px = (pos.longitude - originX) / pixelW;
        double py = (pos.latitude - originY) / pixelH; // pixelH may be negative

        int ix = static_cast<int>(std::floor(px));
        int iy = static_cast<int>(std::floor(py));
        double fx = px - ix;
        double fy = py - iy;

        auto sample = [&](int x, int y) -> double {
            if (x < 0 || x >= gdalCols || y < 0 || y >= gdalRows) return 0.0;
            return gdalElevData[y * gdalCols + x];
        };

        double v00 = sample(ix, iy);
        double v10 = sample(ix + 1, iy);
        double v01 = sample(ix, iy + 1);
        double v11 = sample(ix + 1, iy + 1);

        double v0 = v00 * (1 - fx) + v10 * fx;
        double v1 = v01 * (1 - fx) + v11 * fx;
        double elev = v0 * (1 - fy) + v1 * fy;

        return elev; // elevation units depend on raster (often meters); existing API uses feet MSL - caller must handle units
    }
#endif

    // Fallback: nearest-neighbor elevation lookup. Suitable for unit tests and small datasets.
    if (terrainDatabase_.empty()) return 0.0;

    // Haversine helper (meters)
    auto haversineMeters = [](double lat1, double lon1, double lat2, double lon2) {
        const double R = 6371000.0; // earth radius meters
        const double PI = 3.14159265358979323846;
        auto toRad = [&](double d) { return d * PI / 180.0; };
        double rlat1 = toRad(lat1);
        double rlon1 = toRad(lon1);
        double rlat2 = toRad(lat2);
        double rlon2 = toRad(lon2);
        double dLat = rlat2 - rlat1;
        double dLon = rlon2 - rlon1;
        double a = std::sin(dLat/2) * std::sin(dLat/2) +
                   std::cos(rlat1) * std::cos(rlat2) *
                   std::sin(dLon/2) * std::sin(dLon/2);
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return R * c;
    };

    double bestDist = std::numeric_limits<double>::infinity();
    double bestElev = 0.0;
    for (const auto& tp : terrainDatabase_) {
        double d = haversineMeters(pos.latitude, pos.longitude,
                                   tp.position.latitude, tp.position.longitude);
        if (d < bestDist) {
            bestDist = d;
            bestElev = tp.elevation;
        }
    }

    return bestElev;
}

bool TerrainAwareness::isInMountainousArea(const Position& pos) const {
    double elevation = getTerrainElevation(pos);
    return elevation > 3000.0;
}

} // namespace AICopilot

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
#include <algorithm>

namespace AICopilot {

void TerrainAwareness::updateAircraftState(const AircraftState& state) {
    currentState_ = state;
}

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
    // Stub for terrain database integration
    return true;
}

bool TerrainAwareness::loadObstacleDatabase(const std::string& databasePath) {
    // Stub for obstacle database integration
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
    // Simplified - real implementation would interpolate from terrain grid
    return 0.0;
}

bool TerrainAwareness::isInMountainousArea(const Position& pos) const {
    double elevation = getTerrainElevation(pos);
    return elevation > 3000.0;
}

} // namespace AICopilot

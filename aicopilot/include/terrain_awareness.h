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

#ifndef TERRAIN_AWARENESS_H
#define TERRAIN_AWARENESS_H

#include "aicopilot_types.h"
#include <vector>
#include <memory>

namespace AICopilot {

// Terrain warning levels (TAWS/EGPWS)
enum class TerrainWarningLevel {
    NONE,
    CAUTION,          // "CAUTION TERRAIN"
    WARNING,          // "TERRAIN TERRAIN"
    PULL_UP           // "PULL UP, PULL UP"
};

// Obstacle types
enum class ObstacleType {
    TOWER,
    BUILDING,
    MOUNTAIN,
    ANTENNA,
    BRIDGE,
    POWER_LINE,
    OTHER
};

// Obstacle structure
struct Obstacle {
    ObstacleType type;
    Position position;
    double height;  // feet AGL
    double elevation;  // feet MSL
    std::string description;
};

// Terrain alert
struct TerrainAlert {
    TerrainWarningLevel level;
    Position position;
    double clearance;  // feet
    double requiredAltitude;  // feet MSL
    std::string message;
};

/**
 * Terrain Awareness and Warning System (TAWS/EGPWS)
 */
class TerrainAwareness {
public:
    TerrainAwareness() = default;
    
    // Update aircraft state
    void updateAircraftState(const AircraftState& state);
    
    // Check terrain clearance
    TerrainAlert checkTerrainClearance(const Position& pos) const;
    
    // Get terrain elevation at position
    double getTerrainElevation(const Position& pos) const;
    
    // Get altitude above ground level (AGL)
    double getAltitudeAGL(const Position& pos, double altitudeMSL) const;
    
    // Check for obstacles in path
    std::vector<Obstacle> detectObstacles(
        const Position& start,
        const Position& end,
        double altitude) const;
    
    // Get highest obstacle in area
    Obstacle getHighestObstacle(
        const Position& center,
        double radius) const;
    
    // Calculate minimum safe altitude (MSA)
    double calculateMinimumSafeAltitude(const Position& pos) const;
    
    // Check if altitude is safe for position
    bool isAltitudeSafe(const Position& pos, double altitude) const;
    
    // Get recommended escape maneuver
    struct EscapeManeuver {
        double targetAltitude;
        double targetHeading;
        double targetPitch;
        double targetSpeed;
    };
    EscapeManeuver getEscapeManeuver(const TerrainAlert& alert) const;
    
    // Predict terrain conflict ahead
    bool predictTerrainConflict(
        const Position& pos,
        double heading,
        double groundSpeed,
        double altitude,
        double lookaheadTime) const;  // seconds
    
    // Get terrain profile ahead
    std::vector<TerrainPoint> getTerrainProfile(
        const Position& start,
        double heading,
        double distance) const;  // nautical miles
    
    // Load terrain database (stub for integration)
    bool loadTerrainDatabase(const std::string& databasePath);
    
    // Load obstacle database
    bool loadObstacleDatabase(const std::string& databasePath);
    
private:
    AircraftState currentState_;
    std::vector<TerrainPoint> terrainDatabase_;
    std::vector<Obstacle> obstacleDatabase_;
    
    // Minimum clearances (feet)
    static constexpr double MIN_CLEARANCE_CRUISE = 1000.0;
    static constexpr double MIN_CLEARANCE_APPROACH = 500.0;
    static constexpr double MIN_CLEARANCE_EMERGENCY = 300.0;
    
    // Helper methods
    TerrainWarningLevel determineWarningLevel(double clearance, bool climbing) const;
    double interpolateElevation(const Position& pos) const;
    bool isInMountainousArea(const Position& pos) const;
};

} // namespace AICopilot

#endif // TERRAIN_AWARENESS_H

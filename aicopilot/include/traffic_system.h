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

#ifndef TRAFFIC_SYSTEM_H
#define TRAFFIC_SYSTEM_H

#include "aicopilot_types.h"
#include <vector>
#include <memory>
#include <string>

namespace AICopilot {

// Traffic advisory type (TCAS)
enum class TrafficAdvisoryType {
    NONE,
    TA,   // Traffic Advisory
    RA    // Resolution Advisory
};

// Traffic conflict type
enum class ConflictType {
    NONE,
    CONVERGING,
    HEAD_ON,
    OVERTAKING,
    CROSSING
};

// Resolution advisory direction
enum class RADirection {
    NONE,
    CLIMB,
    DESCEND,
    MAINTAIN,
    LEVEL_OFF
};

// Traffic target
struct TrafficTarget {
    std::string callsign;
    Position position;
    double altitude;
    double groundSpeed;
    double heading;
    double verticalSpeed;
    double relativeAltitude;  // feet (+ is above, - is below)
    double range;  // nautical miles
    double bearing;  // degrees
    double closureRate;  // knots
};

// Traffic advisory
struct TrafficAdvisory {
    TrafficAdvisoryType type;
    ConflictType conflictType;
    RADirection raDirection;
    TrafficTarget target;
    double timeToClosestApproach;  // seconds
    double minSeparation;  // nautical miles
    std::string message;
};

/**
 * Real-time Traffic Avoidance System (TCAS)
 */
class TrafficSystem {
public:
    TrafficSystem() = default;
    
    // Update own aircraft state
    void updateOwnAircraft(const AircraftState& state);
    
    // Update traffic targets
    void updateTrafficTargets(const std::vector<TrafficTarget>& targets);
    
    // Get all traffic targets
    std::vector<TrafficTarget> getTrafficTargets() const { return trafficTargets_; }
    
    // Get nearest traffic
    TrafficTarget getNearestTraffic() const;
    
    // Check for traffic conflicts
    std::vector<TrafficAdvisory> checkTrafficConflicts() const;
    
    // Get active advisories
    std::vector<TrafficAdvisory> getActiveAdvisories() const { return activeAdvisories_; }
    
    // Check if there's an active RA (Resolution Advisory)
    bool hasActiveRA() const;
    
    // Get current RA
    TrafficAdvisory getActiveRA() const;
    
    // Calculate avoidance maneuver
    struct AvoidanceManeuver {
        double targetAltitude;
        double targetHeading;
        double targetSpeed;
        double verticalSpeed;
        bool urgent;
    };
    AvoidanceManeuver calculateAvoidanceManeuver(
        const TrafficAdvisory& advisory) const;
    
    // Predict conflicts ahead
    bool predictConflict(
        const Position& pos,
        double heading,
        double altitude,
        double groundSpeed,
        double lookaheadTime) const;  // seconds
    
    // Get traffic in vicinity
    std::vector<TrafficTarget> getTrafficInVicinity(double range) const;  // nm
    
    // Calculate separation from target
    double calculateSeparation(const TrafficTarget& target) const;
    
    // Calculate time to closest approach
    double calculateTimeToClosestApproach(const TrafficTarget& target) const;
    
    // Check if target is a threat
    bool isThreat(const TrafficTarget& target) const;
    
private:
    AircraftState ownAircraft_;
    std::vector<TrafficTarget> trafficTargets_;
    std::vector<TrafficAdvisory> activeAdvisories_;
    
    // TCAS parameters (nautical miles)
    static constexpr double TA_RANGE = 6.0;
    static constexpr double RA_RANGE = 3.0;
    static constexpr double VERTICAL_SEPARATION = 1000.0;  // feet
    
    // Helper methods
    ConflictType determineConflictType(const TrafficTarget& target) const;
    RADirection determineRADirection(const TrafficTarget& target) const;
    double calculateRelativeBearing(const TrafficTarget& target) const;
    double calculateClosureRate(const TrafficTarget& target) const;
    bool isConverging(const TrafficTarget& target) const;
};

} // namespace AICopilot

#endif // TRAFFIC_SYSTEM_H

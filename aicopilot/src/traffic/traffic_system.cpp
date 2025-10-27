/*****************************************************************************
* Copyright 2025 AI Copilot FS Project - Traffic System Implementation
*****************************************************************************/

#include "traffic_system.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace AICopilot {

void TrafficSystem::updateOwnAircraft(const AircraftState& state) {
    ownAircraft_ = state;
}

void TrafficSystem::updateTrafficTargets(const std::vector<TrafficTarget>& targets) {
    trafficTargets_ = targets;
    
    // Update active advisories
    activeAdvisories_.clear();
    auto conflicts = checkTrafficConflicts();
    activeAdvisories_ = conflicts;
}

TrafficTarget TrafficSystem::getNearestTraffic() const {
    TrafficTarget nearest;
    double minRange = std::numeric_limits<double>::max();
    
    for (const auto& target : trafficTargets_) {
        if (target.range < minRange) {
            minRange = target.range;
            nearest = target;
        }
    }
    
    return nearest;
}

std::vector<TrafficAdvisory> TrafficSystem::checkTrafficConflicts() const {
    std::vector<TrafficAdvisory> advisories;
    
    for (const auto& target : trafficTargets_) {
        if (!isThreat(target)) continue;
        
        TrafficAdvisory advisory;
        advisory.target = target;
        advisory.conflictType = determineConflictType(target);
        
        // Determine advisory type based on range
        if (target.range < RA_RANGE && std::abs(target.relativeAltitude) < VERTICAL_SEPARATION) {
            advisory.type = TrafficAdvisoryType::RA;
            advisory.raDirection = determineRADirection(target);
            advisory.message = "TRAFFIC, CLIMB/DESCEND";
        } else if (target.range < TA_RANGE) {
            advisory.type = TrafficAdvisoryType::TA;
            advisory.message = "TRAFFIC ADVISORY";
        }
        
        if (advisory.type != TrafficAdvisoryType::NONE) {
            advisories.push_back(advisory);
        }
    }
    
    return advisories;
}

bool TrafficSystem::hasActiveRA() const {
    for (const auto& advisory : activeAdvisories_) {
        if (advisory.type == TrafficAdvisoryType::RA) {
            return true;
        }
    }
    return false;
}

TrafficAdvisory TrafficSystem::getActiveRA() const {
    for (const auto& advisory : activeAdvisories_) {
        if (advisory.type == TrafficAdvisoryType::RA) {
            return advisory;
        }
    }
    return TrafficAdvisory();
}

TrafficSystem::AvoidanceManeuver TrafficSystem::calculateAvoidanceManeuver(
    const TrafficAdvisory& advisory) const {
    
    AvoidanceManeuver maneuver;
    maneuver.targetHeading = ownAircraft_.heading;
    maneuver.targetSpeed = ownAircraft_.indicatedAirspeed;
    maneuver.urgent = (advisory.type == TrafficAdvisoryType::RA);
    
    if (advisory.raDirection == RADirection::CLIMB) {
        maneuver.targetAltitude = ownAircraft_.position.altitude + 1000.0;
        maneuver.verticalSpeed = 1500.0;
    } else if (advisory.raDirection == RADirection::DESCEND) {
        maneuver.targetAltitude = ownAircraft_.position.altitude - 1000.0;
        maneuver.verticalSpeed = -1500.0;
    } else {
        maneuver.targetAltitude = ownAircraft_.position.altitude;
        maneuver.verticalSpeed = 0.0;
    }
    
    return maneuver;
}

bool TrafficSystem::isThreat(const TrafficTarget& target) const {
    // Check if traffic is a threat based on range and closure
    if (target.range > TA_RANGE) return false;
    if (target.closureRate < 0) return false;  // Aircraft are moving apart (diverging)
    return true;
}

ConflictType TrafficSystem::determineConflictType(const TrafficTarget& target) const {
    double relativeBearing = calculateRelativeBearing(target);
    
    if (std::abs(relativeBearing) < 30.0 || std::abs(relativeBearing) > 330.0) {
        return ConflictType::HEAD_ON;
    } else if (relativeBearing > 30.0 && relativeBearing < 150.0) {
        return ConflictType::CROSSING;
    } else if (relativeBearing > 150.0 && relativeBearing < 210.0) {
        return ConflictType::OVERTAKING;
    }
    return ConflictType::CONVERGING;
}

RADirection TrafficSystem::determineRADirection(const TrafficTarget& target) const {
    if (target.relativeAltitude > 0) {
        return RADirection::DESCEND;
    } else {
        return RADirection::CLIMB;
    }
}

double TrafficSystem::calculateRelativeBearing(const TrafficTarget& target) const {
    double bearing = target.bearing - ownAircraft_.heading;
    while (bearing < 0) bearing += 360.0;
    while (bearing >= 360.0) bearing -= 360.0;
    return bearing;
}

} // namespace AICopilot

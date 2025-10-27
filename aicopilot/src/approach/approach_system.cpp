/*****************************************************************************
* Copyright 2025 AI Copilot FS Project - Approach System
*****************************************************************************/

#include "approach_system.h"
#include <cmath>
#include <algorithm>

namespace AICopilot {

bool ApproachSystem::loadApproach(const ApproachProcedure& approach) {
    currentApproach_ = approach;
    activeWaypointIndex_ = 0;
    return true;
}

void ApproachSystem::activateApproach() {
    approachActive_ = true;
    currentPhase_ = ApproachPhase::INITIAL;
}

void ApproachSystem::deactivateApproach() {
    approachActive_ = false;
}

ApproachStatus ApproachSystem::updateApproachStatus(const AircraftState& state) {
    if (!approachActive_) return status_;
    
    updatePhase(state.position);
    
    if (currentApproach_.type == ApproachType::ILS) {
        calculateILSDeviation(state.position, state.heading,
                            status_.localizerDeviation,
                            status_.glideslopeDeviation);
        status_.localizerCaptured = std::abs(status_.localizerDeviation) < 0.5;
        status_.glideslopeCaptured = std::abs(status_.glideslopeDeviation) < 0.5;
    }
    
    status_.distanceToThreshold = getDistanceToThreshold(state.position);
    status_.heightAboveThreshold = getHeightAboveThreshold(state.position, state.position.altitude);
    status_.onGlidepath = std::abs(status_.glideslopeDeviation) < MAX_GLIDESLOPE_DEVIATION;
    status_.stabilized = isStabilized(state);
    
    return status_;
}

bool ApproachSystem::isEstablished() const {
    if (currentApproach_.type == ApproachType::ILS) {
        return status_.localizerCaptured && status_.glideslopeCaptured;
    }
    return approachActive_ && currentPhase_ == ApproachPhase::FINAL;
}

bool ApproachSystem::isStabilized(const AircraftState& state) const {
    if (!isEstablished()) return false;
    
    // Check stabilized approach criteria
    if (std::abs(status_.localizerDeviation) > MAX_LOCALIZER_DEVIATION) return false;
    if (std::abs(status_.glideslopeDeviation) > MAX_GLIDESLOPE_DEVIATION) return false;
    if (std::abs(state.verticalSpeed) > MAX_SINK_RATE) return false;
    
    return true;
}

void ApproachSystem::calculateILSDeviation(
    const Position& pos,
    double heading,
    double& localizerDeviation,
    double& glideslopeDeviation) const {
    
    localizerDeviation = calculateLocalizerDeviation(pos, heading);
    glideslopeDeviation = calculateGlideslopeDeviation(pos, pos.altitude);
}

ApproachSystem::RNAVGuidance ApproachSystem::calculateRNAVGuidance(const Position& pos) const {
    RNAVGuidance guidance;
    
    if (activeWaypointIndex_ < currentApproach_.waypoints.size()) {
        auto waypoint = currentApproach_.waypoints[activeWaypointIndex_];
        guidance.targetAltitude = waypoint.altitude;
        guidance.targetSpeed = waypoint.speed;
        guidance.crossTrackError = 0.0;  // Simplified
        guidance.verticalDeviation = pos.altitude - waypoint.altitude;
    }
    
    return guidance;
}

RNAVWaypoint ApproachSystem::getNextWaypoint() const {
    if (activeWaypointIndex_ < currentApproach_.waypoints.size()) {
        return currentApproach_.waypoints[activeWaypointIndex_];
    }
    return RNAVWaypoint();
}

void ApproachSystem::advanceWaypoint() {
    if (activeWaypointIndex_ < currentApproach_.waypoints.size() - 1) {
        activeWaypointIndex_++;
    }
}

void ApproachSystem::executeMissedApproach() {
    currentPhase_ = ApproachPhase::MISSED;
}

bool ApproachSystem::shouldExecuteMissedApproach(const AircraftState& state) const {
    if (!isStabilized(state) && status_.heightAboveThreshold < 1000.0) {
        return true;
    }
    return false;
}

double ApproachSystem::getDistanceToThreshold(const Position& pos) const {
    // Simplified distance calculation
    return 5.0;  // nautical miles
}

double ApproachSystem::getHeightAboveThreshold(const Position& pos, double altitude) const {
    return altitude - currentApproach_.ilsData.thresholdPosition.altitude;
}

// Private methods

void ApproachSystem::updatePhase(const Position& pos) {
    double distance = getDistanceToThreshold(pos);
    
    if (distance > 10.0) {
        currentPhase_ = ApproachPhase::INITIAL;
    } else if (distance > 5.0) {
        currentPhase_ = ApproachPhase::INTERMEDIATE;
    } else {
        currentPhase_ = ApproachPhase::FINAL;
    }
}

double ApproachSystem::calculateLocalizerDeviation(const Position& pos, double heading) const {
    double courseDiff = heading - currentApproach_.ilsData.localizerCourse;
    return courseDiff / 2.5;  // Convert to dots
}

double ApproachSystem::calculateGlideslopeDeviation(const Position& pos, double altitude) const {
    double targetAltitude = currentApproach_.ilsData.thresholdPosition.altitude +
                           getDistanceToThreshold(pos) * 6076.0 * std::tan(currentApproach_.ilsData.glideslopeAngle * M_PI / 180.0);
    double deviation = (altitude - targetAltitude) / 100.0;
    return deviation;
}

} // namespace AICopilot

/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
* Comprehensive Stabilized Approach Implementation
*****************************************************************************/

#include "../include/stabilized_approach.h"
#include <cmath>
#include <sstream>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AICopilot {

StabilizedApproachSystem::StabilizedApproachSystem() = default;

bool StabilizedApproachSystem::initialize(const std::string& aircraftType) {
    aircraftType_ = aircraftType;
    configureThresholdsForAircraft();
    return true;
}

void StabilizedApproachSystem::startApproachMonitoring(
    double targetAltitude,
    double targetSpeed,
    double targetDescentRate) {
    
    targetAltitude_ = targetAltitude;
    targetSpeed_ = targetSpeed;
    targetDescentRate_ = targetDescentRate;
    monitoringActive_ = true;
    currentStage_ = ApproachStage::INITIAL_DESCENT;
    approachProgress_ = 0.0;
}

StabilizationCriteria StabilizedApproachSystem::updateApproachStatus(
    const AircraftState& currentState,
    const Position& thresholdPosition,
    double thresholdElevation) {
    
    StabilizationCriteria criteria;
    
    // Calculate height above threshold
    double heightAboveThreshold = currentState.position.altitude - thresholdElevation;
    
    // Update approach stage
    updateApproachStage(heightAboveThreshold);
    
    // Update progress (0.0 = start, 1.0 = touchdown)
    double startHeight = 3000.0;  // Typical approach start altitude AGL
    approachProgress_ = 1.0 - (heightAboveThreshold / startHeight);
    approachProgress_ = std::max(0.0, std::min(1.0, approachProgress_));
    
    // ALTITUDE CRITERIA
    criteria.altitudeStabilized = validateAltitudeStabilization(currentState, targetAltitude_);
    criteria.altitudeDeviation = currentState.position.altitude - targetAltitude_;
    
    // SPEED CRITERIA
    criteria.speedStabilized = validateSpeedStabilization(currentState, targetSpeed_);
    criteria.speedDeviation = currentState.indicatedAirspeed - targetSpeed_;
    
    // VERTICAL SPEED CRITERIA
    criteria.verticalSpeedActual = currentState.verticalSpeed;
    criteria.verticalSpeedStable = validateDescentRateStabilization(currentState, targetDescentRate_);
    criteria.descentRateDeviation = std::abs(currentState.verticalSpeed) - std::abs(targetDescentRate_);
    criteria.descentRateAcceptable = std::abs(currentState.verticalSpeed) <= StabilizationCriteria::MAX_DESCENT_RATE;
    
    // GLIDESLOPE CRITERIA
    // Simplified glideslope calculation (3 degree path is standard)
    double distanceToThreshold = std::sqrt(
        std::pow(thresholdPosition.latitude - currentState.position.latitude, 2) +
        std::pow(thresholdPosition.longitude - currentState.position.longitude, 2)) * 60.0;  // nm
    
    double expectedAltitude = thresholdElevation + (distanceToThreshold * 6076.0 * std::tan(3.0 * M_PI / 180.0));
    criteria.glideslopeDeviation = (currentState.position.altitude - expectedAltitude) / 100.0;  // Convert to dots
    criteria.glideslopeEstablished = std::abs(criteria.glideslopeDeviation) <= StabilizationCriteria::MAX_GLIDESLOPE_DEVIATION;
    
    // LOCALIZER CRITERIA (if ILS available)
    double courseDiff = currentState.heading - thresholdPosition.heading;
    if (courseDiff > 180.0) courseDiff -= 360.0;
    if (courseDiff < -180.0) courseDiff += 360.0;
    criteria.localizerDeviation = courseDiff / 2.5;  // Convert to dots
    criteria.localizerEstablished = std::abs(criteria.localizerDeviation) <= StabilizationCriteria::MAX_LOCALIZER_DEVIATION;
    
    // CONFIGURATION CRITERIA
    criteria.gearDown = currentState.gearDown;
    criteria.flapsCorrect = currentState.flapsPosition > 50;  // Assume approach flaps are > 50%
    criteria.lightsOn = true;  // Simplified - would check actual light status
    criteria.configurationCorrect = criteria.gearDown && criteria.flapsCorrect && criteria.lightsOn;
    
    // OVERALL STABILIZATION
    criteria.fullyStabilized = isFullyStabilized(criteria);
    
    return criteria;
}

bool StabilizedApproachSystem::isFullyStabilized(const StabilizationCriteria& criteria) const {
    return criteria.altitudeStabilized &&
           criteria.speedStabilized &&
           criteria.verticalSpeedStable &&
           criteria.glideslopeEstablished &&
           criteria.localizerEstablished &&
           criteria.configurationCorrect &&
           criteria.descentRateAcceptable;
}

bool StabilizedApproachSystem::isMinimumlyStabilized(const StabilizationCriteria& criteria) const {
    // Minimum criteria: altitude and speed within limits, descent rate acceptable
    return std::abs(criteria.altitudeDeviation) <= (StabilizationCriteria::MAX_ALTITUDE_DEVIATION * 2.0) &&
           std::abs(criteria.speedDeviation) <= (StabilizationCriteria::MAX_SPEED_DEVIATION * 2.0) &&
           criteria.descentRateAcceptable &&
           criteria.glideslopeEstablished &&
           criteria.configurationCorrect;
}

std::vector<GoAroundTrigger> StabilizedApproachSystem::evaluateGoAroundTriggers(
    const AircraftState& state,
    double thresholdElevation) {
    
    std::vector<GoAroundTrigger> triggers;
    double heightAboveThreshold = state.position.altitude - thresholdElevation;
    
    // Altitude trigger
    triggers.push_back(checkAltitudeGoAround(heightAboveThreshold, targetAltitude_, state.position.altitude));
    
    // Speed trigger
    triggers.push_back(checkSpeedGoAround(state, targetSpeed_));
    
    // Descent rate trigger
    triggers.push_back(checkDescentRateGoAround(state.verticalSpeed, targetDescentRate_));
    
    // Glideslope trigger
    // Calculate glideslope deviation
    double expectedAltitude = thresholdElevation + heightAboveThreshold * std::tan(3.0 * M_PI / 180.0);
    double glideslopeDeviation = (state.position.altitude - expectedAltitude) / 100.0;
    triggers.push_back(checkGlideslopeGoAround(glideslopeDeviation, heightAboveThreshold));
    
    // Localizer trigger
    double localizerDeviation = 0.0;  // Would calculate from actual heading vs required
    triggers.push_back(checkLocalizerGoAround(localizerDeviation, heightAboveThreshold));
    
    return triggers;
}

bool StabilizedApproachSystem::shouldExecuteGoAround(
    const std::vector<GoAroundTrigger>& triggers) const {
    
    for (const auto& trigger : triggers) {
        if (trigger.triggered) {
            return true;
        }
    }
    
    return false;
}

std::string StabilizedApproachSystem::getApproachReport(const StabilizationCriteria& criteria) const {
    std::ostringstream report;
    
    report << "APPROACH STABILIZATION REPORT\n";
    report << "=============================\n\n";
    
    report << "ALTITUDE\n";
    report << "--------\n";
    report << "Status: " << (criteria.altitudeStabilized ? "STABILIZED" : "UNSTABLE") << "\n";
    report << "Deviation: " << criteria.altitudeDeviation << " ft\n";
    report << "Max Allowed: " << StabilizationCriteria::MAX_ALTITUDE_DEVIATION << " ft\n\n";
    
    report << "SPEED\n";
    report << "-----\n";
    report << "Status: " << (criteria.speedStabilized ? "STABILIZED" : "UNSTABLE") << "\n";
    report << "Deviation: " << criteria.speedDeviation << " knots\n";
    report << "Max Allowed: " << StabilizationCriteria::MAX_SPEED_DEVIATION << " knots\n\n";
    
    report << "DESCENT RATE\n";
    report << "------------\n";
    report << "Status: " << (criteria.descentRateAcceptable ? "ACCEPTABLE" : "EXCESSIVE") << "\n";
    report << "Actual: " << criteria.verticalSpeedActual << " fpm\n";
    report << "Max Allowed: " << StabilizationCriteria::MAX_DESCENT_RATE << " fpm\n\n";
    
    report << "GLIDESLOPE\n";
    report << "----------\n";
    report << "Status: " << (criteria.glideslopeEstablished ? "ESTABLISHED" : "NOT ESTABLISHED") << "\n";
    report << "Deviation: " << criteria.glideslopeDeviation << " dots\n";
    report << "Max Allowed: " << StabilizationCriteria::MAX_GLIDESLOPE_DEVIATION << " dots\n\n";
    
    report << "LOCALIZER\n";
    report << "---------\n";
    report << "Status: " << (criteria.localizerEstablished ? "ESTABLISHED" : "NOT ESTABLISHED") << "\n";
    report << "Deviation: " << criteria.localizerDeviation << " dots\n";
    report << "Max Allowed: " << StabilizationCriteria::MAX_LOCALIZER_DEVIATION << " dots\n\n";
    
    report << "CONFIGURATION\n";
    report << "--------------\n";
    report << "Gear Down: " << (criteria.gearDown ? "YES" : "NO") << "\n";
    report << "Flaps Correct: " << (criteria.flapsCorrect ? "YES" : "NO") << "\n";
    report << "Lights On: " << (criteria.lightsOn ? "YES" : "NO") << "\n\n";
    
    report << "OVERALL STATUS\n";
    report << "---------------\n";
    report << "Fully Stabilized: " << (criteria.fullyStabilized ? "YES" : "NO") << "\n";
    
    return report.str();
}

std::string StabilizedApproachSystem::getGoAroundReason(
    const std::vector<GoAroundTrigger>& triggers) const {
    
    for (const auto& trigger : triggers) {
        if (trigger.triggered) {
            return trigger.reason;
        }
    }
    
    return "";
}

std::string StabilizedApproachSystem::getCorrectiveAction(
    const StabilizationCriteria& criteria) const {
    
    if (criteria.altitudeDeviation > StabilizationCriteria::MAX_ALTITUDE_DEVIATION) {
        return "Reduce descent rate or level off to stabilize altitude";
    }
    
    if (criteria.speedDeviation > StabilizationCriteria::MAX_SPEED_DEVIATION) {
        return "Reduce throttle and increase drag to reduce speed";
    }
    
    if (std::abs(criteria.verticalSpeedActual) > StabilizationCriteria::MAX_DESCENT_RATE) {
        return "Reduce descent rate - descent too steep";
    }
    
    if (std::abs(criteria.glideslopeDeviation) > StabilizationCriteria::MAX_GLIDESLOPE_DEVIATION) {
        if (criteria.glideslopeDeviation > 0) {
            return "Increase descent rate - above glideslope";
        } else {
            return "Reduce descent rate - below glideslope";
        }
    }
    
    if (std::abs(criteria.localizerDeviation) > StabilizationCriteria::MAX_LOCALIZER_DEVIATION) {
        if (criteria.localizerDeviation > 0) {
            return "Turn right to establish localizer";
        } else {
            return "Turn left to establish localizer";
        }
    }
    
    if (!criteria.configurationCorrect) {
        return "Verify landing configuration - gear down, flaps set, lights on";
    }
    
    return "Continue approach - stabilizing";
}

void StabilizedApproachSystem::reset() {
    monitoringActive_ = false;
    targetAltitude_ = 0.0;
    targetSpeed_ = 0.0;
    targetDescentRate_ = 0.0;
    currentStage_ = ApproachStage::INITIAL_DESCENT;
    approachProgress_ = 0.0;
}

ApproachStage StabilizedApproachSystem::getCurrentStage() const {
    return currentStage_;
}

double StabilizedApproachSystem::getApproachProgress() const {
    return approachProgress_;
}

// PRIVATE METHODS

void StabilizedApproachSystem::updateApproachStage(double heightAboveThreshold) {
    if (heightAboveThreshold > 2000.0) {
        currentStage_ = ApproachStage::INITIAL_DESCENT;
    } else if (heightAboveThreshold > 1000.0) {
        currentStage_ = ApproachStage::LEVEL_OFF_1000FT;
    } else if (heightAboveThreshold > 500.0) {
        currentStage_ = ApproachStage::FINAL_APPROACH;
    } else if (heightAboveThreshold > 200.0) {
        currentStage_ = ApproachStage::SHORT_FINAL;
    } else if (heightAboveThreshold > 50.0) {
        currentStage_ = ApproachStage::LANDING_IMMINENT;
    } else {
        currentStage_ = ApproachStage::TOUCHDOWN;
    }
}

bool StabilizedApproachSystem::validateAltitudeStabilization(const AircraftState& state,
                                                           double targetAltitude) {
    return std::abs(state.position.altitude - targetAltitude) <= allowedAltitudeDeviation_;
}

bool StabilizedApproachSystem::validateSpeedStabilization(const AircraftState& state,
                                                        double targetSpeed) {
    return std::abs(state.indicatedAirspeed - targetSpeed) <= allowedSpeedDeviation_;
}

bool StabilizedApproachSystem::validateDescentRateStabilization(const AircraftState& state,
                                                              double targetDescentRate) {
    return std::abs(std::abs(state.verticalSpeed) - std::abs(targetDescentRate)) <= allowedDescentRateDeviation_;
}

bool StabilizedApproachSystem::validateFlightPathStabilization(const AircraftState& state,
                                                             double glideslopeDeviation) {
    return std::abs(glideslopeDeviation) <= allowedGlideslopeDeviation_;
}

bool StabilizedApproachSystem::validateAircraftConfiguration(const AircraftState& state) {
    return state.gearDown && state.flapsPosition > 50;
}

void StabilizedApproachSystem::configureThresholdsForAircraft() {
    if (aircraftType_.find("Cessna") != std::string::npos ||
        aircraftType_.find("172") != std::string::npos ||
        aircraftType_.find("208") != std::string::npos) {
        configureSmallAircraftThresholds();
    } else if (aircraftType_.find("737") != std::string::npos ||
               aircraftType_.find("320") != std::string::npos ||
               aircraftType_.find("A380") != std::string::npos) {
        configureTransportCategoryThresholds();
    } else if (aircraftType_.find("helicopter") != std::string::npos ||
               aircraftType_.find("heli") != std::string::npos) {
        configureHelicopterThresholds();
    } else {
        // Default thresholds
        configureSmallAircraftThresholds();
    }
}

void StabilizedApproachSystem::configureSmallAircraftThresholds() {
    minStabilizationAltitude_ = 500.0;      // feet AGL
    allowedAltitudeDeviation_ = 50.0;       // feet
    allowedSpeedDeviation_ = 10.0;          // knots
    allowedDescentRateDeviation_ = 200.0;   // fpm
    allowedGlideslopeDeviation_ = 1.0;      // dots
    allowedLocalizerDeviation_ = 1.0;       // dots
}

void StabilizedApproachSystem::configureTransportCategoryThresholds() {
    minStabilizationAltitude_ = 1000.0;     // feet AGL
    allowedAltitudeDeviation_ = 100.0;      // feet
    allowedSpeedDeviation_ = 15.0;          // knots
    allowedDescentRateDeviation_ = 300.0;   // fpm
    allowedGlideslopeDeviation_ = 1.0;      // dots
    allowedLocalizerDeviation_ = 1.0;       // dots
}

void StabilizedApproachSystem::configureHelicopterThresholds() {
    minStabilizationAltitude_ = 100.0;      // feet AGL
    allowedAltitudeDeviation_ = 20.0;       // feet
    allowedSpeedDeviation_ = 5.0;           // knots
    allowedDescentRateDeviation_ = 100.0;   // fpm
    allowedGlideslopeDeviation_ = 0.5;      // dots
    allowedLocalizerDeviation_ = 0.5;       // dots
}

GoAroundTrigger StabilizedApproachSystem::checkAltitudeGoAround(
    double heightAboveThreshold,
    double targetAltitude,
    double actualAltitude) {
    
    GoAroundTrigger trigger;
    trigger.reason = "Altitude go-around";
    trigger.heightAboveThreshold = heightAboveThreshold;
    trigger.targetAltitude = targetAltitude;
    trigger.actualAltitude = actualAltitude;
    trigger.triggered = false;
    
    // Go-around if too high below 500 ft AGL
    if (heightAboveThreshold < 500.0 && actualAltitude > (targetAltitude + 100.0)) {
        trigger.reason = "Too high on approach";
        trigger.triggered = true;
    }
    
    return trigger;
}

GoAroundTrigger StabilizedApproachSystem::checkSpeedGoAround(
    const AircraftState& state,
    double targetSpeed) {
    
    GoAroundTrigger trigger;
    trigger.reason = "Speed go-around";
    trigger.speedVsTarget = state.indicatedAirspeed - targetSpeed;
    trigger.triggered = false;
    
    // Go-around if too fast (> 15 knots over target)
    if (state.indicatedAirspeed > (targetSpeed + 20.0)) {
        trigger.reason = "Too fast on approach";
        trigger.triggered = true;
    }
    
    return trigger;
}

GoAroundTrigger StabilizedApproachSystem::checkDescentRateGoAround(
    double actualDescentRate,
    double expectedDescentRate) {
    
    GoAroundTrigger trigger;
    trigger.reason = "Descent rate go-around";
    trigger.actualDescentRate = actualDescentRate;
    trigger.expectedDescentRate = expectedDescentRate;
    trigger.triggered = false;
    
    // Go-around if descent rate excessive
    if (std::abs(actualDescentRate) > MAX_DESCENT_RATE) {
        trigger.reason = "Descent rate excessive";
        trigger.triggered = true;
    }
    
    return trigger;
}

GoAroundTrigger StabilizedApproachSystem::checkGlideslopeGoAround(
    double glideslopeDeviation,
    double heightAboveThreshold) {
    
    GoAroundTrigger trigger;
    trigger.reason = "Glideslope go-around";
    trigger.triggered = false;
    
    // Go-around if above glide slope below 1000 ft
    if (heightAboveThreshold < 1000.0 && glideslopeDeviation > 1.0) {
        trigger.reason = "Above glide slope";
        trigger.triggered = true;
    }
    
    return trigger;
}

GoAroundTrigger StabilizedApproachSystem::checkLocalizerGoAround(
    double localizerDeviation,
    double heightAboveThreshold) {
    
    GoAroundTrigger trigger;
    trigger.reason = "Localizer go-around";
    trigger.triggered = false;
    
    // Go-around if off localizer below 500 ft
    if (heightAboveThreshold < 500.0 && std::abs(localizerDeviation) > 1.5) {
        trigger.reason = "Off localizer";
        trigger.triggered = true;
    }
    
    return trigger;
}

} // namespace AICopilot

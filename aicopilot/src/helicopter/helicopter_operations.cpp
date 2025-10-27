/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*****************************************************************************/

#include "helicopter_operations.h"
#include <cmath>
#include <algorithm>

namespace AICopilot {

void HelicopterOperations::initialize(const HelicopterData& profile) {
    profile_ = profile;
    currentMode_ = HelicopterMode::UNKNOWN;
}

bool HelicopterOperations::initiateHover(double targetAltitudeAGL) {
    currentMode_ = HelicopterMode::HOVER;
    hoverState_.altitude = targetAltitudeAGL;
    hoverState_.stable = false;
    return true;
}

bool HelicopterOperations::maintainHover(const AircraftState& state) {
    if (currentMode_ != HelicopterMode::HOVER) return false;
    
    hoverState_.position = state.position;
    hoverState_.driftRate = state.groundSpeed;
    hoverState_.verticalSpeed = state.verticalSpeed;
    hoverState_.stable = isHoverStable(hoverState_);
    
    return hoverState_.stable;
}

void HelicopterOperations::exitHover() {
    currentMode_ = HelicopterMode::UNKNOWN;
}

bool HelicopterOperations::initiateHoverTaxi(const Position& targetPosition, double speed) {
    currentMode_ = HelicopterMode::HOVER_TAXI;
    hoverTaxiTarget_ = targetPosition;
    return true;
}

bool HelicopterOperations::executeVerticalTakeoff(double targetAltitudeAGL) {
    currentMode_ = HelicopterMode::HOVER;
    return initiateHover(targetAltitudeAGL);
}

bool HelicopterOperations::transitionToForwardFlight(double targetSpeed) {
    currentMode_ = HelicopterMode::TRANSITION_TO_FORWARD;
    return true;
}

bool HelicopterOperations::transitionToHover() {
    currentMode_ = HelicopterMode::TRANSITION_TO_HOVER;
    return true;
}

bool HelicopterOperations::initiateAutorotation() {
    currentMode_ = HelicopterMode::AUTOROTATION;
    autorotationActive_ = true;
    return true;
}

void HelicopterOperations::maintainAutorotation(const AircraftState& state) {
    // Maintain optimal autorotation speed
}

double HelicopterOperations::getOptimalAutorotationSpeed() const {
    return profile_.autorotationSpeed;
}

bool HelicopterOperations::canHoverAtAltitude(double altitudeMSL, double temperature) const {
    return altitudeMSL < profile_.hoverCeiling;
}

double HelicopterOperations::getMaxHoverAltitude(double temperature) const {
    return profile_.hoverCeiling;
}

double HelicopterOperations::calculateCollectivePitch(double targetVerticalSpeed) const {
    // Simplified collective calculation
    return targetVerticalSpeed / profile_.maxVerticalSpeed;
}

HelicopterOperations::CyclicControl HelicopterOperations::calculateHoverCorrection(
    const HoverState& state) const {
    
    CyclicControl control;
    control.lateral = 0.0;
    control.longitudinal = 0.0;
    
    if (state.driftRate > MAX_HOVER_DRIFT) {
        control.longitudinal = -0.1;
    }
    
    return control;
}

bool HelicopterOperations::isWindSuitableForHover(const WeatherConditions& weather) const {
    return weather.windSpeed < 25.0;
}

bool HelicopterOperations::checkVortexRingState(const AircraftState& state) const {
    // VRS occurs in descent with low forward speed
    return state.verticalSpeed < -500.0 && state.groundSpeed < 20.0;
}

bool HelicopterOperations::isHoverStable(const HoverState& state) const {
    return state.driftRate < MAX_HOVER_DRIFT &&
           std::abs(state.verticalSpeed) < 100.0;
}

} // namespace AICopilot

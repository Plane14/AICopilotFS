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

void HelicopterOperations::updateHoverTaxi(const AircraftState& state) {
    if (currentMode_ != HelicopterMode::HOVER_TAXI) return;
    
    // Calculate distance to target
    double deltaLat = hoverTaxiTarget_.latitude - state.position.latitude;
    double deltaLon = hoverTaxiTarget_.longitude - state.position.longitude;
    double distance = std::sqrt(deltaLat * deltaLat + deltaLon * deltaLon) * 60.0 * 6076.12;  // Convert to feet
    
    // Update hover state
    hoverState_.position = state.position;
    hoverState_.driftRate = state.groundSpeed;
    hoverState_.verticalSpeed = state.verticalSpeed;
    
    // Check if arrived at target
    if (distance < HOVER_TOLERANCE_LATERAL) {
        // Arrived, transition to hover
        currentMode_ = HelicopterMode::HOVER;
        hoverState_.stable = true;
    }
}

bool HelicopterOperations::executeVerticalLanding(const Position& landingSpot) {
    // Validate landing conditions
    if (currentMode_ == HelicopterMode::UNKNOWN) {
        return false;
    }
    
    // Set mode to vertical landing (using hover mode for now)
    currentMode_ = HelicopterMode::HOVER;
    hoverState_.altitude = 0.0;  // Target ground level
    hoverState_.stable = false;
    
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
    if (!autorotationActive_) {
        return;
    }
    
    // Track autorotation start altitude if not set
    if (autorotationStartAltitude_ == 0.0) {
        autorotationStartAltitude_ = state.position.altitude;
    }
    
    // Phase 1: Entry - Immediately lower collective to maintain rotor RPM
    // In real autorotation, collective is lowered to reduce blade pitch,
    // allowing upward airflow to maintain rotor RPM
    const double currentAGL = state.position.altitude - getGroundElevation(state.position);
    const double optimalRPM = profile_.normalRotorRPM * 0.95;  // Target 95% of normal RPM
    
    // Phase 2: Steady Descent - Maintain optimal RPM and descent rate
    // Calculate optimal collective based on descent rate and rotor RPM
    double targetCollective = 0.0;
    
    // Check current rotor RPM (using engine RPM as proxy)
    double currentRPM = state.engineRPM;
    
    // RPM Management: Adjust collective to maintain optimal rotor RPM
    // If RPM is too low, raise collective slightly to reduce descent rate
    // If RPM is too high, lower collective to increase descent rate
    if (currentRPM < optimalRPM - 50.0) {
        // RPM too low - rotor is slowing, need to lower collective more
        targetCollective = std::max(0.0, targetCollective - 0.05);
    } else if (currentRPM > optimalRPM + 50.0) {
        // RPM too high - rotor is speeding up, can raise collective slightly
        targetCollective = std::min(0.3, targetCollective + 0.03);
    }
    
    // Descent Rate Control
    // Target descent rate: -1500 to -2000 fpm for most helicopters
    const double targetDescentRate = -1700.0;  // feet per minute
    const double descentRateError = state.verticalSpeed - targetDescentRate;
    
    // Adjust collective based on descent rate error
    if (std::abs(descentRateError) > 200.0) {
        // Significant deviation from target descent rate
        if (descentRateError > 0) {
            // Descending too slowly, lower collective
            targetCollective = std::max(0.0, targetCollective - 0.02);
        } else {
            // Descending too fast, raise collective (if RPM permits)
            if (currentRPM > optimalRPM) {
                targetCollective = std::min(0.3, targetCollective + 0.02);
            }
        }
    }
    
    // Phase 3: Wind Alignment
    // During autorotation, aircraft should be aligned with wind to minimize
    // sideways drift and maintain controllability
    // This would require wind data from WeatherConditions
    // For now, maintain current heading and minimize drift with cyclic
    
    // Phase 4: Flare Preparation and Execution
    // Flare should begin between 50-100 feet AGL depending on descent rate
    const double flareAltitude = calculateFlareAltitude(state.verticalSpeed);
    
    if (currentAGL <= flareAltitude && currentAGL > 10.0) {
        // Execute flare: Cyclic back to arrest forward speed and descent
        // Collective held low until the final moment
        // This is the critical phase where stored rotor energy is used
        
        // Calculate flare collective - increases as we get closer to ground
        double flareProgress = 1.0 - (currentAGL / flareAltitude);
        targetCollective = flareProgress * 0.8;  // Gradually increase to 80%
        
        // In flare, we want to:
        // 1. Reduce forward speed
        // 2. Reduce descent rate
        // 3. Use stored rotor RPM energy
        // 4. Touch down gently with minimal forward speed
    } else if (currentAGL <= 10.0) {
        // Final touchdown - full collective to cushion landing
        targetCollective = 1.0;
    }
    
    // Safety checks
    if (currentRPM < optimalRPM * 0.7) {
        // Critical low RPM - rotor is in danger of stalling
        // Emergency: lower collective immediately
        targetCollective = 0.0;
    }
    
    // Clamp collective to safe range during autorotation
    targetCollective = std::clamp(targetCollective, 0.0, 1.0);
    
    // Store autorotation state for monitoring
    autorotationState_.targetCollective = targetCollective;
    autorotationState_.currentRPM = currentRPM;
    autorotationState_.altitudeAGL = currentAGL;
    autorotationState_.descentRate = state.verticalSpeed;
    autorotationState_.inFlare = (currentAGL <= flareAltitude);
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

double HelicopterOperations::calculateFlareAltitude(double descentRate) const {
    // Calculate flare altitude based on descent rate
    // Faster descent = higher flare altitude needed
    // Typical range: 50-100 feet AGL
    
    double absDescentRate = std::abs(descentRate);
    
    // Base flare altitude
    double flareAlt = 60.0;  // feet
    
    // Adjust based on descent rate
    if (absDescentRate > 2000.0) {
        flareAlt = 100.0;  // High descent rate needs early flare
    } else if (absDescentRate > 1500.0) {
        flareAlt = 75.0;   // Moderate descent rate
    } else if (absDescentRate < 1000.0) {
        flareAlt = 50.0;   // Slow descent rate
    }
    
    return flareAlt;
}

double HelicopterOperations::getGroundElevation(const Position& position) const {
    // In a real implementation, this would query terrain elevation
    // For now, return 0 (sea level) - should be enhanced with terrain data
    return 0.0;
}

AutorotationState HelicopterOperations::getAutorotationState() const {
    return autorotationState_;
}

bool HelicopterOperations::isInAutorotation() const {
    return autorotationActive_;
}

void HelicopterOperations::abortAutorotation() {
    // Abort autorotation and attempt to restore power
    // This should only be done if engine power is available
    autorotationActive_ = false;
    currentMode_ = HelicopterMode::UNKNOWN;
    autorotationStartAltitude_ = 0.0;
}

bool HelicopterOperations::validateAutorotationEntry(const AircraftState& state) const {
    // Validate conditions for entering autorotation
    
    // Check altitude - need sufficient altitude for autorotation
    const double minAltitudeAGL = 200.0;  // feet
    double currentAGL = state.position.altitude;  // Simplified - should subtract ground elevation
    
    if (currentAGL < minAltitudeAGL) {
        return false;  // Too low for safe autorotation
    }
    
    // Check airspeed - need forward airspeed for rotor RPM
    if (state.indicatedAirspeed < 40.0) {
        // Very low airspeed - difficult autorotation
        // Still possible but not recommended
    }
    
    // Check if already on ground
    if (state.onGround) {
        return false;
    }
    
    return true;
}

double HelicopterOperations::calculateTailRotorPedal(double mainRotorTorque) const {
    // Calculate anti-torque pedal input needed to counteract main rotor torque
    // Simplified model: pedal position proportional to torque
    // Range: -1.0 (left) to +1.0 (right)
    
    // Normalize torque to pedal range
    // Typically, more power requires more right pedal (positive)
    double pedalInput = mainRotorTorque / 100.0;  // Assuming torque in percentage
    
    // Clamp to valid range
    return std::clamp(pedalInput, -1.0, 1.0);
}

bool HelicopterOperations::navigateToLandingZone(const Position& lz) {
    // Navigate to landing zone
    // This would typically involve:
    // 1. Calculate approach path
    // 2. Transition to forward flight if in hover
    // 3. Follow approach to landing zone
    // 4. Transition to hover at landing zone
    // 5. Execute vertical landing
    
    // For now, simplified implementation
    currentMode_ = HelicopterMode::TRANSITION_TO_HOVER;
    hoverTaxiTarget_ = lz;
    return true;
}

bool HelicopterOperations::performOrbitPattern(const Position& center, double radius) {
    // Perform circular orbit around a point
    // Used for observation, search and rescue, etc.
    
    // Validate parameters
    if (radius < 100.0) {
        return false;  // Too small radius
    }
    
    // Set mode to forward flight for orbit
    currentMode_ = HelicopterMode::FORWARD_FLIGHT;
    
    // Store orbit parameters (would need additional state variables)
    // For now, simplified implementation
    return true;
}

bool HelicopterOperations::performStationKeeping(const Position& station) {
    // Maintain position at a specific geographic point
    // Essentially high-precision hover
    
    hoverState_.position = station;
    currentMode_ = HelicopterMode::HOVER;
    hoverState_.stable = false;  // Will become stable when position is maintained
    
    return true;
}

bool HelicopterOperations::isVisibilitySuitable(const WeatherConditions& weather) const {
    // Check if visibility is suitable for helicopter operations
    // Helicopters typically require better visibility than fixed-wing
    
    // Minimum visibility for VFR helicopter operations (typically 1-3 statute miles)
    const double minVisibility = 1.0;  // statute miles
    
    // Check visibility
    if (weather.visibility < minVisibility) {
        return false;
    }
    
    // Check ceiling (cloud height)
    const double minCeiling = 500.0;  // feet AGL
    if (weather.cloudBase < minCeiling) {
        return false;
    }
    
    return true;
}

bool HelicopterOperations::checkLowRotorRPM() const {
    // Check if rotor RPM is dangerously low
    // This is a critical emergency condition
    
    if (autorotationActive_) {
        // During autorotation, check against autorotation minimum
        const double minAutorotationRPM = profile_.normalRotorRPM * 0.7;
        return autorotationState_.currentRPM < minAutorotationRPM;
    } else {
        // During normal operations
        const double minNormalRPM = profile_.normalRotorRPM * 0.9;
        // Would need access to current RPM from state
        // For now, use stored autorotation state RPM
        return autorotationState_.currentRPM < minNormalRPM;
    }
}

void HelicopterOperations::handleTailRotorFailure() {
    // Handle tail rotor failure emergency
    // This is a critical emergency requiring immediate action:
    // 1. Reduce power to minimum (reduces torque and yaw)
    // 2. Enter autorotation if altitude permits
    // 3. Land as soon as possible
    // 4. Use forward airspeed for directional control
    
    // If sufficient altitude, enter autorotation
    if (autorotationState_.altitudeAGL > 200.0) {
        initiateAutorotation();
    }
    
    // Note: In a real implementation, this would:
    // - Alert the pilot
    // - Reduce collective to minimum
    // - Maintain forward airspeed for weathervaning effect
    // - Plan for running landing (landing with forward speed)
}

void HelicopterOperations::updateMode(const AircraftState& state) {
    // Update flight mode based on current state
    
    // Check if on ground
    if (state.onGround) {
        currentMode_ = HelicopterMode::UNKNOWN;
        return;
    }
    
    // Check if in autorotation
    if (autorotationActive_) {
        currentMode_ = HelicopterMode::AUTOROTATION;
        return;
    }
    
    // Determine mode based on airspeed and altitude
    if (state.groundSpeed < 10.0 && state.position.altitude < 500.0) {
        // Low speed, low altitude - likely hovering
        if (currentMode_ != HelicopterMode::HOVER && currentMode_ != HelicopterMode::HOVER_TAXI) {
            currentMode_ = HelicopterMode::HOVER;
        }
    } else if (state.groundSpeed > 40.0) {
        // Higher speed - forward flight
        currentMode_ = HelicopterMode::FORWARD_FLIGHT;
    } else {
        // Transitional speed - maintain current mode or set transition
        if (currentMode_ == HelicopterMode::HOVER && state.groundSpeed > 20.0) {
            currentMode_ = HelicopterMode::TRANSITION_TO_FORWARD;
        } else if (currentMode_ == HelicopterMode::FORWARD_FLIGHT && state.groundSpeed < 30.0) {
            currentMode_ = HelicopterMode::TRANSITION_TO_HOVER;
        }
    }
}

double HelicopterOperations::calculateHoverPower(double altitude, double temperature) const {
    // Calculate power required for hover at given altitude and temperature
    // Returns percentage of maximum power required
    
    // Base power requirement (percentage of max)
    double basePower = 60.0;  // Typical hover power is 60-70% of max
    
    // Altitude compensation
    // Power increases with altitude due to reduced air density
    // Approximately 3% per 1000 feet
    double altitudeFactor = 1.0 + (altitude / 1000.0) * 0.03;
    
    // Temperature compensation
    // Higher temperature = less dense air = more power required
    // ISA standard temperature is 15°C at sea level, -2°C per 1000 feet
    double isaTemp = 15.0 - (altitude / 1000.0 * 2.0);
    double tempDeviation = temperature - isaTemp;
    double tempFactor = 1.0 + (tempDeviation / 10.0) * 0.01;  // 1% per 10°C
    
    // Calculate total power requirement
    double requiredPower = basePower * altitudeFactor * tempFactor;
    
    // Clamp to reasonable range
    return std::clamp(requiredPower, 0.0, 100.0);
}

} // namespace AICopilot

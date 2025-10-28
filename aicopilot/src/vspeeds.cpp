/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
* Comprehensive V-Speed Calculation Implementation
*****************************************************************************/

#include "../include/vspeeds.h"
#include <cmath>
#include <algorithm>

namespace AICopilot {

VSpeedCalculator::VSpeedCalculator() = default;

bool VSpeedCalculator::initialize(const AircraftConfig& config) {
    aircraftConfig_ = config;
    
    // Initialize with base values
    currentVSpeeds_.STALL_SPEED = config.stallSpeed;
    currentVSpeeds_.MAX_STRUCTURAL = config.maxSpeed;
    currentVSpeeds_.CRUISE_SPEED = config.cruiseSpeed;
    currentVSpeeds_.ECON_CRUISE = config.cruiseSpeed * 0.75;
    currentVSpeeds_.BEST_CLIMB = config.cruiseSpeed * 0.75;
    currentVSpeeds_.DESCENT_SPEED = config.cruiseSpeed * 0.6;
    
    return true;
}

VSpeedSet VSpeedCalculator::calculateVSpeeds(
    const WeightBalance& weightBalance,
    const EnvironmentalFactors& environment,
    const FlightConfiguration& config) {
    
    VSpeedSet vSpeeds;
    
    // Calculate base stall speed
    double baseStallSpeed = calculateBaseStallSpeed(weightBalance);
    vSpeeds.VS0 = baseStallSpeed;  // Stall speed in landing configuration (flaps down)
    vSpeeds.VS1 = baseStallSpeed * 0.95;  // Stall speed in clean configuration
    
    // Calculate approach speeds
    vSpeeds.VREF = baseStallSpeed * 1.3;  // Reference landing speed
    vSpeeds.VAPP = baseStallSpeed * 1.4;  // Approach speed
    vSpeeds.VF = baseStallSpeed * 1.3;    // Final approach speed
    
    // Apply environmental corrections
    double densityAltitude = calculateDensityAltitude(environment);
    
    // Correct stall speeds for density altitude
    vSpeeds.VS0 = applyDensityAltitudeCorrection(vSpeeds.VS0, densityAltitude);
    vSpeeds.VS1 = applyDensityAltitudeCorrection(vSpeeds.VS1, densityAltitude);
    vSpeeds.VREF = applyDensityAltitudeCorrection(vSpeeds.VREF, densityAltitude);
    vSpeeds.VAPP = applyDensityAltitudeCorrection(vSpeeds.VAPP, densityAltitude);
    
    // Apply headwind correction (increases speeds slightly)
    if (environment.windHeadwind > 0) {
        vSpeeds.V1 = applyWindCorrection(vSpeeds.VS0 * 1.1, environment.windHeadwind);
        vSpeeds.VR = applyWindCorrection(vSpeeds.VS0 * 1.05, environment.windHeadwind);
        vSpeeds.V2 = applyWindCorrection(vSpeeds.VS0 * 1.2, environment.windHeadwind);
    } else {
        vSpeeds.V1 = vSpeeds.VS0 * 1.1;
        vSpeeds.VR = vSpeeds.VS0 * 1.05;
        vSpeeds.V2 = vSpeeds.VS0 * 1.2;
    }
    
    // Apply surface correction for takeoff distance
    if (environment.runwaySurfaceCoeff < 1.0) {
        vSpeeds.V1 = applyWindCorrection(vSpeeds.V1, 0.0);  // Surface increases distance
        vSpeeds.VR = applyWindCorrection(vSpeeds.VR, 0.0);
        vSpeeds.V2 = applyWindCorrection(vSpeeds.V2, 0.0);
    }
    
    // Maximum speeds with configurations
    vSpeeds.VFE = config.flapsPosition > 0 ? aircraftConfig_.maxSpeed * 0.85 : aircraftConfig_.maxSpeed;
    vSpeeds.VFEF = config.flapsPosition > 0 ? aircraftConfig_.maxSpeed * 0.8 : aircraftConfig_.maxSpeed;
    vSpeeds.VMAX_GEAR_DOWN = aircraftConfig_.maxSpeed * 0.9;
    vSpeeds.VMAX_FLAPS = config.flapsPosition > 0 ? aircraftConfig_.maxSpeed * 0.75 : aircraftConfig_.maxSpeed;
    
    // Climb speeds
    vSpeeds.BEST_CLIMB = std::min(vSpeeds.VS1 * 1.5, aircraftConfig_.cruiseSpeed * 0.75);
    vSpeeds.BEST_ANGLE = std::min(vSpeeds.VS1 * 1.3, aircraftConfig_.cruiseSpeed * 0.65);
    
    // Cruise and descent
    vSpeeds.CRUISE_SPEED = aircraftConfig_.cruiseSpeed;
    vSpeeds.ECON_CRUISE = aircraftConfig_.cruiseSpeed * 0.75;
    vSpeeds.DESCENT_SPEED = aircraftConfig_.cruiseSpeed * 0.6;
    
    // Other speeds
    vSpeeds.STALL_SPEED = vSpeeds.VS0;
    vSpeeds.MAX_STRUCTURAL = aircraftConfig_.maxSpeed;
    
    currentVSpeeds_ = vSpeeds;
    return vSpeeds;
}

double VSpeedCalculator::calculateTakeoffDistanceRequired(
    const VSpeedSet& vSpeeds,
    const WeightBalance& weightBalance,
    const EnvironmentalFactors& environment,
    double safetyFactor) {
    
    // Base takeoff distance calculation (simplified model)
    // Real calculation would use:
    // - Runway surface friction coefficient
    // - Density altitude
    // - Aircraft weight
    // - Temperature
    // - Wind component
    
    double densityAltitude = calculateDensityAltitude(environment);
    
    // Simplified distance to 35 ft (per FAA standards)
    // Typical formula: TODR = (weight / thrust) * (1/[2g * (a - f)])
    // where a = acceleration due to engines, f = friction
    
    // Reference: Cessna 172 at 2700 lbs = ~1500 ft at sea level
    double baseDistance = 1500.0 * (weightBalance.totalWeight / aircraftConfig_.maxGrossWeight);
    
    // Density altitude correction (approximately 3.5% per 1000 ft DA)
    double daCorrection = 1.0 + (densityAltitude / 1000.0) * 0.035;
    baseDistance *= daCorrection;
    
    // Temperature correction (approximately 2% per 5°C above 15°C)
    double tempCorrection = 1.0 + std::max(0.0, (environment.temperature - 15.0) / 5.0) * 0.02;
    baseDistance *= tempCorrection;
    
    // Surface correction
    baseDistance /= environment.runwaySurfaceCoeff;
    
    // Wind correction (headwind reduces distance, tailwind increases)
    double windCorrection = 1.0 - (environment.windHeadwind / 20.0);  // 1 kt headwind = ~0.05% reduction
    baseDistance *= windCorrection;
    
    // Apply FAA safety factor (typically 15% for normal operations)
    baseDistance *= safetyFactor;
    
    return baseDistance;
}

double VSpeedCalculator::calculateLandingDistanceRequired(
    const VSpeedSet& vSpeeds,
    const WeightBalance& weightBalance,
    const EnvironmentalFactors& environment,
    double safetyFactor) {
    
    // Landing distance calculation
    // Reference: Cessna 172 = ~1300 ft landing distance at sea level
    
    double densityAltitude = calculateDensityAltitude(environment);
    
    double baseDistance = 1300.0 * (weightBalance.totalWeight / aircraftConfig_.maxGrossWeight);
    
    // Density altitude correction (approximately 3-4% per 1000 ft DA)
    double daCorrection = 1.0 + (densityAltitude / 1000.0) * 0.04;
    baseDistance *= daCorrection;
    
    // Runway surface correction
    baseDistance /= environment.runwaySurfaceCoeff;
    
    // Wind correction (headwind reduces landing distance significantly)
    double windCorrection = 1.0 - (environment.windHeadwind / 15.0);  // Stronger effect than takeoff
    baseDistance *= std::max(0.5, windCorrection);  // Never less than 50% of base
    
    // Apply FAA safety factor (67% for landing per Part 25)
    baseDistance *= safetyFactor;
    
    return baseDistance;
}

bool VSpeedCalculator::isRunwaySuitableForTakeoff(
    double runwayLength,
    double takeoffDistRequired,
    double safetyMargin) {
    
    return runwayLength > (takeoffDistRequired + safetyMargin);
}

bool VSpeedCalculator::isRunwaySuitableForLanding(
    double runwayLength,
    double landingDistRequired,
    double safetyMargin) {
    
    return runwayLength > (landingDistRequired + safetyMargin);
}

double VSpeedCalculator::calculateDensityAltitude(
    const EnvironmentalFactors& environment) {
    
    // Density altitude = pressure altitude + temp correction
    double pressureAltitude = calculatePressureAltitude(
        environment.altimeter, environment.runwayAltitude);
    
    // Temperature correction: ~120 ft per degree Celsius above standard
    double tempStandard = 15.0 - (environment.runwayAltitude / 1000.0 * 2.0);  // Standard lapse rate
    double tempCorrection = (environment.temperature - tempStandard) * 120.0;
    
    return pressureAltitude + tempCorrection;
}

double VSpeedCalculator::calculatePressureAltitude(
    double barometerSetting,
    double fieldElevation) {
    
    // Standard altimeter setting is 29.92 inHg
    double standardBaro = 29.92;
    
    // Each 0.1 inHg difference = approximately 1000 ft
    double pressureAltitude = fieldElevation + ((standardBaro - barometerSetting) * 1000.0);
    
    return pressureAltitude;
}

double VSpeedCalculator::calculateBaseStallSpeed(const WeightBalance& wb) {
    // Stall speed varies with square root of weight
    // VS = VSref * sqrt(W/Wref)
    
    // Reference values for Cessna 172: 38 knots (clean), 31 knots (flaps full)
    double vsRef = 38.0;  // Reference stall speed
    double wRef = 2450.0;  // Reference weight
    
    double stallSpeed = vsRef * std::sqrt(wb.totalWeight / wRef);
    
    // Landing configuration (full flaps) is typically 15-20% slower than clean
    return stallSpeed;
}

double VSpeedCalculator::applyDensityAltitudeCorrection(
    double speed, double densityAltitude) {
    
    // Density altitude correction increases required airspeed
    // Approximately 2% per 1000 ft density altitude
    double correction = 1.0 + (densityAltitude / 1000.0) * 0.02;
    return speed * correction;
}

double VSpeedCalculator::applyWeightCorrection(
    double speed, double actualWeight, double refWeight) {
    
    // Speed correction for weight: V_actual = V_ref * sqrt(W_actual/W_ref)
    return speed * std::sqrt(actualWeight / refWeight);
}

double VSpeedCalculator::applyWindCorrection(
    double speed, double headwind) {
    
    // Headwind reduces required ground speed
    // Ground speed = True airspeed - headwind
    // Required takeoff speed remains in TAS, so headwind helps
    return speed * (1.0 - headwind / 100.0);  // Weak approximation
}

double VSpeedCalculator::applySurfaceCorrection(
    double speed, double surfaceCoeff) {
    
    // Poor surface requires higher speed to generate same drag force
    // Correction factor: 1 / surface coefficient
    return speed / surfaceCoeff;
}

} // namespace AICopilot

/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Comprehensive V-Speed Calculation System
* Implements realistic takeoff and landing V-speed calculations
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef VSPEEDS_H
#define VSPEEDS_H

#include "aicopilot_types.h"
#include "aircraft_config.h"
#include <string>

namespace AICopilot {

/**
 * Critical V-Speeds for aircraft operations
 */
struct VSpeedSet {
    // Takeoff V-Speeds (all in knots)
    double VS0;      // Stall speed in landing configuration
    double VS1;      // Stall speed in clean configuration
    double VFE;      // Maximum speed with flaps extended
    double VFEF;     // Maximum speed for takeoff flaps configuration
    double VF;       // Final approach speed
    double VREF;     // Reference landing speed (1.3 * VS0)
    double VAPP;     // Approach speed
    
    // Takeoff speeds
    double V1;       // Decision speed - abort/go decision point
    double VR;       // Rotation speed - begin nose-up rotation
    double V2;       // Takeoff safety speed
    double VMAX_GEAR_DOWN;  // Maximum speed with gear down
    double VMAX_FLAPS;      // Maximum speed with flaps
    
    // Climb speeds
    double BEST_CLIMB;      // Best rate of climb speed
    double BEST_ANGLE;      // Best angle of climb speed
    
    // Descent and approach speeds
    double CRUISE_SPEED;    // Normal cruise speed
    double ECON_CRUISE;     // Economy cruise speed
    double DESCENT_SPEED;   // Normal descent speed
    
    // Other critical speeds
    double STALL_SPEED;     // Stall speed (clean)
    double MAX_STRUCTURAL;  // Never exceed speed (Vne)
};

/**
 * Environmental factors affecting V-speeds
 */
struct EnvironmentalFactors {
    double temperature;         // degrees Celsius
    double altimeter;          // inches of mercury
    double runwayAltitude;     // feet MSL
    double windHeadwind;       // knots (positive = headwind)
    double runwayLength;       // feet
    double runwaySurfaceCoeff; // friction coefficient (1.0 = concrete, 0.5 = grass)
};

/**
 * Weight and balance data
 */
struct WeightBalance {
    double aircraftEmptyWeight;    // lbs
    double fuelOnBoard;            // lbs
    double crewWeight;             // lbs
    double passengerWeight;        // lbs
    double cargoWeight;            // lbs
    double totalWeight;            // lbs
    double cgLocation;             // % of MAC
    double cgForwardLimit;         // % of MAC
    double cgAftLimit;             // % of MAC
};

/**
 * Configuration for V-speed calculation
 */
struct FlightConfiguration {
    int flapsPosition;             // degrees or %
    bool gearDown;
    bool landingLightOn;
    bool defrost;
    bool sidewindComponent;        // knots
};

/**
 * Comprehensive V-Speed Calculation System
 * Implements FAA and manufacturer procedures for realistic V-speeds
 */
class VSpeedCalculator {
public:
    VSpeedCalculator();
    ~VSpeedCalculator() = default;
    
    /**
     * Initialize with aircraft configuration
     */
    bool initialize(const AircraftConfig& config);
    
    /**
     * Calculate all V-speeds for given conditions
     */
    VSpeedSet calculateVSpeeds(
        const WeightBalance& weightBalance,
        const EnvironmentalFactors& environment,
        const FlightConfiguration& config);
    
    /**
     * Calculate takeoff distance required
     */
    double calculateTakeoffDistanceRequired(
        const VSpeedSet& vSpeeds,
        const WeightBalance& weightBalance,
        const EnvironmentalFactors& environment,
        double safetyFactor = 1.15);  // 15% safety margin per FAA
    
    /**
     * Calculate landing distance required
     */
    double calculateLandingDistanceRequired(
        const VSpeedSet& vSpeeds,
        const WeightBalance& weightBalance,
        const EnvironmentalFactors& environment,
        double safetyFactor = 1.67);  // 67% safety margin per FAA
    
    /**
     * Check if runway is suitable for takeoff
     */
    bool isRunwaySuitableForTakeoff(
        double runwayLength,
        double takeoffDistRequired,
        double safetyMargin = 1000.0);  // feet
    
    /**
     * Check if runway is suitable for landing
     */
    bool isRunwaySuitableForLanding(
        double runwayLength,
        double landingDistRequired,
        double safetyMargin = 1000.0);  // feet
    
    /**
     * Calculate density altitude
     */
    double calculateDensityAltitude(
        const EnvironmentalFactors& environment);
    
    /**
     * Calculate pressure altitude
     */
    double calculatePressureAltitude(
        double barometerSetting,  // inHg
        double fieldElevation);   // feet
    
    /**
     * Get current V-speed set
     */
    VSpeedSet getCurrentVSpeeds() const { return currentVSpeeds_; }
    
    /**
     * Get recommended takeoff speed
     */
    double getRecommendedTakeoffSpeed() const { 
        return currentVSpeeds_.V2 + 5.0;  // V2 + 5 knots for safety
    }
    
    /**
     * Get recommended landing speed
     */
    double getRecommendedLandingSpeed() const { 
        return currentVSpeeds_.VREF;
    }
    
    /**
     * Get recommended approach speed
     */
    double getRecommendedApproachSpeed() const { 
        return currentVSpeeds_.VAPP;
    }
    
private:
    AircraftConfig aircraftConfig_;
    VSpeedSet currentVSpeeds_;
    
    // Base V-speed calculation
    double calculateBaseStallSpeed(const WeightBalance& wb);
    double calculateV1(
        const VSpeedSet& baseSet,
        double densityAltitude,
        const EnvironmentalFactors& env);
    double calculateVR(const VSpeedSet& baseSet);
    double calculateV2(const VSpeedSet& baseSet);
    
    // Environmental corrections
    double applyDensityAltitudeCorrection(double speed, double densityAltitude);
    double applyWeightCorrection(double speed, double actualWeight, double refWeight);
    double applyWindCorrection(double speed, double headwind);
    double applySurfaceCorrection(double speed, double surfaceCoeff);
    
    // Aircraft type specific calculations
    double calculateSingleEngineVSpeeds(
        const WeightBalance& wb,
        const EnvironmentalFactors& env);
    double calculateMultiEngineVSpeeds(
        const WeightBalance& wb,
        const EnvironmentalFactors& env);
    double calculateTurbopropVSpeeds(
        const WeightBalance& wb,
        const EnvironmentalFactors& env);
    double calculateJetVSpeeds(
        const WeightBalance& wb,
        const EnvironmentalFactors& env);
};

} // namespace AICopilot

#endif // VSPEEDS_H

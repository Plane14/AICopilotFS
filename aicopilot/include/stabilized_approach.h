/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Comprehensive Stabilized Approach System
* Implements FAA and industry standard stabilized approach criteria
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef STABILIZED_APPROACH_H
#define STABILIZED_APPROACH_H

#include "aicopilot_types.h"
#include <string>
#include <vector>

namespace AICopilot {

/**
 * Stabilization criteria result
 */
struct StabilizationCriteria {
    // Altitude criteria
    bool altitudeStabilized;
    double altitudeDeviation;      // feet from target
    static constexpr double MAX_ALTITUDE_DEVIATION = 50.0;  // feet
    
    // Speed criteria
    bool speedStabilized;
    double speedDeviation;         // knots from target
    static constexpr double MAX_SPEED_DEVIATION = 10.0;    // knots
    
    // Vertical speed criteria
    bool verticalSpeedStable;
    double verticalSpeedActual;    // fpm
    static constexpr double MAX_DESCENT_RATE = 1000.0;     // fpm
    
    // Descent rate criteria
    bool descentRateAcceptable;
    double descentRateDeviation;   // fpm from expected
    static constexpr double MAX_DESCENT_RATE_DEVIATION = 200.0;  // fpm
    
    // Glide slope criteria
    bool glideslopeEstablished;
    double glideslopeDeviation;    // dots
    static constexpr double MAX_GLIDESLOPE_DEVIATION = 1.0; // dots
    
    // Localizer criteria (for ILS approaches)
    bool localizerEstablished;
    double localizerDeviation;     // dots
    static constexpr double MAX_LOCALIZER_DEVIATION = 1.0;  // dots
    
    // Configuration criteria
    bool configurationCorrect;
    bool flapsCorrect;
    bool gearDown;
    bool lightsOn;
    
    // Overall stabilization
    bool fullyStabilized;
};

/**
 * Go-around trigger condition
 */
struct GoAroundTrigger {
    std::string reason;
    bool triggered;
    double heightAboveThreshold;   // feet
    double targetAltitude;         // feet
    double actualAltitude;         // feet
    double speedVsTarget;          // speed deviation in knots
    double expectedDescentRate;    // fpm
    double actualDescentRate;      // fpm
};

/**
 * Approach phase and progress tracking
 */
enum class ApproachStage {
    INITIAL_DESCENT,
    LEVEL_OFF_1000FT,
    FINAL_APPROACH,
    SHORT_FINAL,
    LANDING_IMMINENT,
    TOUCHDOWN
};

/**
 * Comprehensive Stabilized Approach System
 * Implements realistic stabilization criteria per FAA and international standards
 */
class StabilizedApproachSystem {
public:
    StabilizedApproachSystem();
    ~StabilizedApproachSystem() = default;
    
    /**
     * Initialize for specific aircraft
     */
    bool initialize(const std::string& aircraftType);
    
    /**
     * Start approach monitoring
     */
    void startApproachMonitoring(
        double targetAltitude,        // feet MSL
        double targetSpeed,           // knots
        double targetDescentRate);    // fpm
    
    /**
     * Update approach status with current aircraft state
     */
    StabilizationCriteria updateApproachStatus(
        const AircraftState& currentState,
        const Position& thresholdPosition,
        double thresholdElevation);
    
    /**
     * Check if approach is fully stabilized
     */
    bool isFullyStabilized(const StabilizationCriteria& criteria) const;
    
    /**
     * Check if approach is adequately stabilized (minimum criteria)
     */
    bool isMinimumlyStabilized(const StabilizationCriteria& criteria) const;
    
    /**
     * Evaluate go-around triggers
     */
    std::vector<GoAroundTrigger> evaluateGoAroundTriggers(
        const AircraftState& state,
        double thresholdElevation);
    
    /**
     * Check if go-around is required
     */
    bool shouldExecuteGoAround(
        const std::vector<GoAroundTrigger>& triggers) const;
    
    /**
     * Get current approach stage
     */
    ApproachStage getCurrentStage() const { return currentStage_; }
    
    /**
     * Get approach progress (0.0 to 1.0)
     */
    double getApproachProgress() const { return approachProgress_; }
    
    /**
     * Get detailed approach report
     */
    std::string getApproachReport(const StabilizationCriteria& criteria) const;
    
    /**
     * Get go-around reason
     */
    std::string getGoAroundReason(
        const std::vector<GoAroundTrigger>& triggers) const;
    
    /**
     * Get recommended corrective action
     */
    std::string getCorrectiveAction(
        const StabilizationCriteria& criteria) const;
    
    /**
     * Reset approach monitoring
     */
    void reset();
    
private:
    std::string aircraftType_;
    double targetAltitude_ = 0.0;
    double targetSpeed_ = 0.0;
    double targetDescentRate_ = 0.0;
    ApproachStage currentStage_ = ApproachStage::INITIAL_DESCENT;
    double approachProgress_ = 0.0;
    bool monitoringActive_ = false;
    
    // Go-around threshold distances
    static constexpr double STABILIZED_BY_ALTITUDE = 1000.0;  // feet AGL
    static constexpr double STABILIZED_BY_DISTANCE = 2.0;     // nm from threshold
    
    // Aircraft type specific thresholds
    double minStabilizationAltitude_;      // feet AGL
    double allowedAltitudeDeviation_;      // feet
    double allowedSpeedDeviation_;         // knots
    double allowedDescentRateDeviation_;   // fpm
    double allowedGlideslopeDeviation_;    // dots
    double allowedLocalizerDeviation_;     // dots
    
    // Helper methods
    void updateApproachStage(double heightAboveThreshold);
    bool validateAltitudeStabilization(const AircraftState& state, 
                                      double targetAltitude);
    bool validateSpeedStabilization(const AircraftState& state, 
                                   double targetSpeed);
    bool validateDescentRateStabilization(const AircraftState& state, 
                                         double targetDescentRate);
    bool validateFlightPathStabilization(const AircraftState& state,
                                        double glideslopeDeviation);
    bool validateAircraftConfiguration(const AircraftState& state);
    
    // Threshold configuration
    void configureThresholdsForAircraft();
    void configureSmallAircraftThresholds();
    void configureTransportCategoryThresholds();
    void configureHelicopterThresholds();
    
    // Go-around decision logic
    GoAroundTrigger checkAltitudeGoAround(
        double heightAboveThreshold,
        double targetAltitude,
        double actualAltitude);
    
    GoAroundTrigger checkSpeedGoAround(
        const AircraftState& state,
        double targetSpeed);
    
    GoAroundTrigger checkDescentRateGoAround(
        double actualDescentRate,
        double expectedDescentRate);
    
    GoAroundTrigger checkGlideslopeGoAround(
        double glideslopeDeviation,
        double heightAboveThreshold);
    
    GoAroundTrigger checkLocalizerGoAround(
        double localizerDeviation,
        double heightAboveThreshold);
};

} // namespace AICopilot

#endif // STABILIZED_APPROACH_H

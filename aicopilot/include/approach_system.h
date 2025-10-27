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

#ifndef APPROACH_SYSTEM_H
#define APPROACH_SYSTEM_H

#include "aicopilot_types.h"
#include <vector>
#include <memory>
#include <string>

namespace AICopilot {

// Approach types
enum class ApproachType {
    ILS,          // Instrument Landing System
    RNAV,         // Area Navigation
    RNP,          // Required Navigation Performance
    VOR,          // VOR approach
    NDB,          // NDB approach
    VISUAL,       // Visual approach
    CIRCLING,     // Circling approach
    UNKNOWN
};

// Approach phase
enum class ApproachPhase {
    INITIAL,
    INTERMEDIATE,
    FINAL,
    MISSED
};

// ILS components
struct ILSData {
    double localizerFrequency;
    double glideslopeFrequency;
    double localizerCourse;    // degrees
    double glideslopeAngle;    // degrees (typically 3.0)
    Position thresholdPosition;
    double decisionHeight;     // feet AGL
};

// RNAV waypoint
struct RNAVWaypoint {
    std::string name;
    Position position;
    double altitude;           // feet MSL
    double speed;              // knots (if constrained)
    bool altitudeConstraint;
    bool speedConstraint;
    std::string type;          // IAF, IF, FAF, MAP
};

// Approach procedure
struct ApproachProcedure {
    ApproachType type;
    std::string name;
    std::string runwayId;
    std::vector<RNAVWaypoint> waypoints;
    ILSData ilsData;
    double minimumAltitude;    // feet MSL
    double missedApproachAltitude;  // feet MSL
    Position missedApproachPoint;
};

// Approach status
struct ApproachStatus {
    bool localizerCaptured;
    bool glideslopeCaptured;
    double localizerDeviation;   // dots (-2.5 to +2.5)
    double glideslopeDeviation;  // dots (-2.5 to +2.5)
    double distanceToThreshold;  // nautical miles
    double heightAboveThreshold; // feet
    bool onGlidepath;
    bool stabilized;
};

/**
 * Advanced Approach Procedures System
 */
class ApproachSystem {
public:
    ApproachSystem() = default;
    
    // Load approach procedure
    bool loadApproach(const ApproachProcedure& approach);
    
    // Get current approach
    ApproachProcedure getCurrentApproach() const { return currentApproach_; }
    
    // Activate approach
    void activateApproach();
    
    // Deactivate approach
    void deactivateApproach();
    
    // Check if approach is active
    bool isApproachActive() const { return approachActive_; }
    
    // Update approach status
    ApproachStatus updateApproachStatus(const AircraftState& state);
    
    // Get current approach phase
    ApproachPhase getCurrentPhase() const { return currentPhase_; }
    
    // Check if aircraft is established on approach
    bool isEstablished() const;
    
    // Check if approach is stabilized
    bool isStabilized(const AircraftState& state) const;
    
    // Calculate ILS deviation
    void calculateILSDeviation(
        const Position& pos,
        double heading,
        double& localizerDeviation,
        double& glideslopeDeviation) const;
    
    // Calculate RNAV guidance
    struct RNAVGuidance {
        double targetHeading;
        double targetAltitude;
        double targetSpeed;
        double crossTrackError;
        double verticalDeviation;
    };
    RNAVGuidance calculateRNAVGuidance(const Position& pos) const;
    
    // Get next waypoint
    RNAVWaypoint getNextWaypoint() const;
    
    // Advance to next waypoint
    void advanceWaypoint();
    
    // Execute missed approach
    void executeMissedApproach();
    
    // Check for missed approach conditions
    bool shouldExecuteMissedApproach(const AircraftState& state) const;
    
    // Calculate vertical path for RNP
    double calculateVerticalPath(const Position& pos) const;
    
    // Get distance to threshold
    double getDistanceToThreshold(const Position& pos) const;
    
    // Get height above threshold
    double getHeightAboveThreshold(const Position& pos, double altitude) const;
    
    // Check circling criteria
    bool canCircle(const AircraftState& state) const;
    
private:
    ApproachProcedure currentApproach_;
    bool approachActive_ = false;
    ApproachPhase currentPhase_ = ApproachPhase::INITIAL;
    ApproachStatus status_;
    size_t activeWaypointIndex_ = 0;
    
    // Stabilized approach criteria
    static constexpr double MAX_LOCALIZER_DEVIATION = 1.0;  // dots
    static constexpr double MAX_GLIDESLOPE_DEVIATION = 1.0;  // dots
    static constexpr double MAX_AIRSPEED_DEVIATION = 10.0;   // knots
    static constexpr double MAX_SINK_RATE = 1000.0;          // fpm
    
    // Helper methods
    void updatePhase(const Position& pos);
    double calculateLocalizerDeviation(const Position& pos, double heading) const;
    double calculateGlideslopeDeviation(const Position& pos, double altitude) const;
    bool isWithinLimits(const AircraftState& state) const;
};

} // namespace AICopilot

#endif // APPROACH_SYSTEM_H

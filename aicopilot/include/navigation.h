/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "aicopilot_types.h"
#include <vector>

namespace AICopilot {

/**
 * Navigation and flight planning
 */
class Navigation {
public:
    Navigation() = default;
    
    // Load flight plan from file
    bool loadFlightPlan(const std::string& filePath);
    
    // Create simple direct flight plan
    FlightPlan createDirectPlan(const std::string& departure, 
                               const std::string& arrival,
                               double cruiseAltitude);
    
    // Get current flight plan
    FlightPlan getFlightPlan() const { return flightPlan_; }
    
    // Get active waypoint
    Waypoint getActiveWaypoint() const;
    
    // Get next waypoint
    Waypoint getNextWaypoint() const;
    
    // Advance to next waypoint
    void advanceWaypoint();
    
    // Calculate distance to waypoint (nautical miles)
    double distanceToWaypoint(const Position& current, const Waypoint& waypoint) const;
    
    // Calculate bearing to waypoint (degrees)
    double bearingToWaypoint(const Position& current, const Waypoint& waypoint) const;
    
    // Calculate cross-track error (nautical miles)
    double crossTrackError(const Position& current) const;
    
    // Check if waypoint is reached
    bool isWaypointReached(const Position& current, double tolerance = 0.5) const;
    
    // Get progress percentage
    double getProgress() const;
    
    // Calculate time to destination (minutes)
    double timeToDestination(double groundSpeed) const;
    
private:
    FlightPlan flightPlan_;
    size_t activeWaypointIndex_ = 0;
    
    // Great circle calculations
    double greatCircleDistance(const Position& p1, const Position& p2) const;
    double greatCircleBearing(const Position& p1, const Position& p2) const;
};

} // namespace AICopilot

#endif // NAVIGATION_H

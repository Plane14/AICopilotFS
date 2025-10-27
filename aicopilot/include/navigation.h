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
    
    // Update waypoint coordinates from external source
    // This should be called after creating a flight plan to populate actual coordinates
    // Returns true if waypoint was found and updated
    bool updateWaypointPosition(const std::string& waypointId, const Position& position);
    
    // Validate flight plan waypoints have valid coordinates
    bool validateFlightPlan() const;
    
    // Add waypoint to current flight plan
    void addWaypoint(const Waypoint& waypoint);
    
    // Insert waypoint at specific index
    void insertWaypoint(size_t index, const Waypoint& waypoint);
    
    // Remove waypoint by index
    void removeWaypoint(size_t index);
    
    // Calculate total route distance (nautical miles)
    double getTotalDistance() const;
    
private:
    FlightPlan flightPlan_;
    size_t activeWaypointIndex_ = 0;
    
    // Great circle calculations
    double greatCircleDistance(const Position& p1, const Position& p2) const;
    double greatCircleBearing(const Position& p1, const Position& p2) const;
    
    // Flight plan parsers
    bool parsePLN(std::ifstream& file);
    bool parseFMS(std::ifstream& file);
};

} // namespace AICopilot

#endif // NAVIGATION_H

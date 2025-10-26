/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "../include/navigation.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace AICopilot {

constexpr double PI = 3.14159265358979323846;
constexpr double EARTH_RADIUS_NM = 3440.065;  // Nautical miles

bool Navigation::loadFlightPlan(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    // Detect file format by extension
    std::string ext = filePath.substr(filePath.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "pln") {
        return parsePLN(file);
    } else if (ext == "fms") {
        return parseFMS(file);
    }
    
    activeWaypointIndex_ = 0;
    return false;
}

bool Navigation::parsePLN(std::ifstream& file) {
    // Parse MSFS/P3D PLN format (XML-based)
    std::string line;
    FlightPlan plan;
    Waypoint currentWaypoint;
    bool inATCWaypoint = false;
    
    while (std::getline(file, line)) {
        // Simple XML parsing for key elements
        if (line.find("<DepartureLLA>") != std::string::npos) {
            // Parse departure coordinates
            size_t start = line.find(">") + 1;
            size_t end = line.find("<", start);
            std::string coords = line.substr(start, end - start);
            // Format: "lat,lon,+altitude"
            std::istringstream iss(coords);
            std::string token;
            std::getline(iss, token, ',');
            double lat = std::stod(token);
            std::getline(iss, token, ',');
            double lon = std::stod(token);
            std::getline(iss, token);
            double alt = std::stod(token.substr(1)); // Skip +/-
            
            Waypoint wp;
            wp.id = plan.departure;
            wp.position.latitude = lat;
            wp.position.longitude = lon;
            wp.position.altitude = alt;
            wp.altitude = alt;
            wp.type = "AIRPORT";
            plan.waypoints.push_back(wp);
        }
        else if (line.find("<DestinationLLA>") != std::string::npos) {
            size_t start = line.find(">") + 1;
            size_t end = line.find("<", start);
            std::string coords = line.substr(start, end - start);
            std::istringstream iss(coords);
            std::string token;
            std::getline(iss, token, ',');
            double lat = std::stod(token);
            std::getline(iss, token, ',');
            double lon = std::stod(token);
            std::getline(iss, token);
            double alt = std::stod(token.substr(1));
            
            Waypoint wp;
            wp.id = plan.arrival;
            wp.position.latitude = lat;
            wp.position.longitude = lon;
            wp.position.altitude = alt;
            wp.altitude = alt;
            wp.type = "AIRPORT";
            plan.waypoints.push_back(wp);
        }
        else if (line.find("<CruiseAltitude>") != std::string::npos) {
            size_t start = line.find(">") + 1;
            size_t end = line.find("<", start);
            plan.cruiseAltitude = std::stod(line.substr(start, end - start));
        }
        else if (line.find("<Title>") != std::string::npos) {
            size_t start = line.find(">") + 1;
            size_t end = line.find("<", start);
            std::string title = line.substr(start, end - start);
            // Parse title for departure/arrival (format: "ICAO to ICAO")
            size_t toPos = title.find(" to ");
            if (toPos != std::string::npos) {
                plan.departure = title.substr(0, toPos);
                plan.arrival = title.substr(toPos + 4);
            }
        }
    }
    
    flightPlan_ = plan;
    activeWaypointIndex_ = 0;
    return !plan.waypoints.empty();
}

bool Navigation::parseFMS(std::ifstream& file) {
    // Parse FMS format (text-based)
    std::string line;
    FlightPlan plan;
    int lineNum = 0;
    
    while (std::getline(file, line)) {
        lineNum++;
        
        // Skip header lines
        if (lineNum <= 2) continue;
        
        // Parse waypoint line
        std::istringstream iss(line);
        std::string waypointType, id;
        double lat, lon, alt;
        
        if (iss >> waypointType >> id >> lat >> lon >> alt) {
            Waypoint wp;
            wp.id = id;
            wp.position.latitude = lat;
            wp.position.longitude = lon;
            wp.position.altitude = alt;
            wp.altitude = alt;
            wp.type = waypointType;
            plan.waypoints.push_back(wp);
            
            if (lineNum == 3 && !plan.waypoints.empty()) {
                plan.departure = plan.waypoints[0].id;
            }
        }
    }
    
    if (!plan.waypoints.empty()) {
        plan.arrival = plan.waypoints.back().id;
        // Estimate cruise altitude from waypoints
        plan.cruiseAltitude = 0;
        for (const auto& wp : plan.waypoints) {
            if (wp.altitude > plan.cruiseAltitude) {
                plan.cruiseAltitude = wp.altitude;
            }
        }
    }
    
    flightPlan_ = plan;
    activeWaypointIndex_ = 0;
    return !plan.waypoints.empty();
}

FlightPlan Navigation::createDirectPlan(const std::string& departure, 
                                       const std::string& arrival,
                                       double cruiseAltitude) {
    FlightPlan plan;
    plan.departure = departure;
    plan.arrival = arrival;
    plan.cruiseAltitude = cruiseAltitude;
    
    // TODO: Create waypoints based on departure/arrival airports
    // This would require navdata database integration
    
    flightPlan_ = plan;
    activeWaypointIndex_ = 0;
    
    return plan;
}

Waypoint Navigation::getActiveWaypoint() const {
    if (activeWaypointIndex_ < flightPlan_.waypoints.size()) {
        return flightPlan_.waypoints[activeWaypointIndex_];
    }
    return Waypoint();
}

Waypoint Navigation::getNextWaypoint() const {
    if (activeWaypointIndex_ + 1 < flightPlan_.waypoints.size()) {
        return flightPlan_.waypoints[activeWaypointIndex_ + 1];
    }
    return Waypoint();
}

void Navigation::advanceWaypoint() {
    if (activeWaypointIndex_ < flightPlan_.waypoints.size()) {
        activeWaypointIndex_++;
    }
}

double Navigation::distanceToWaypoint(const Position& current, const Waypoint& waypoint) const {
    return greatCircleDistance(current, waypoint.position);
}

double Navigation::bearingToWaypoint(const Position& current, const Waypoint& waypoint) const {
    return greatCircleBearing(current, waypoint.position);
}

double Navigation::crossTrackError(const Position& current) const {
    if (flightPlan_.waypoints.size() < 2 || activeWaypointIndex_ == 0) {
        return 0.0;
    }
    
    // Calculate cross-track error from previous to current waypoint
    Waypoint prev = flightPlan_.waypoints[activeWaypointIndex_ - 1];
    Waypoint next = getActiveWaypoint();
    
    double d13 = greatCircleDistance(prev.position, current);
    double brg13 = greatCircleBearing(prev.position, current);
    double brg12 = greatCircleBearing(prev.position, next.position);
    
    return asin(sin(d13 / EARTH_RADIUS_NM) * sin(brg13 - brg12)) * EARTH_RADIUS_NM;
}

bool Navigation::isWaypointReached(const Position& current, double tolerance) const {
    Waypoint active = getActiveWaypoint();
    double distance = distanceToWaypoint(current, active);
    return distance <= tolerance;
}

double Navigation::getProgress() const {
    if (flightPlan_.waypoints.empty()) {
        return 0.0;
    }
    return (double)activeWaypointIndex_ / flightPlan_.waypoints.size() * 100.0;
}

double Navigation::timeToDestination(double groundSpeed) const {
    if (groundSpeed <= 0.0) {
        return 0.0;
    }
    
    double totalDistance = 0.0;
    for (size_t i = activeWaypointIndex_; i < flightPlan_.waypoints.size() - 1; i++) {
        totalDistance += greatCircleDistance(
            flightPlan_.waypoints[i].position,
            flightPlan_.waypoints[i + 1].position
        );
    }
    
    return (totalDistance / groundSpeed) * 60.0; // Convert to minutes
}

double Navigation::greatCircleDistance(const Position& p1, const Position& p2) const {
    double lat1 = p1.latitude * PI / 180.0;
    double lat2 = p2.latitude * PI / 180.0;
    double dLat = (p2.latitude - p1.latitude) * PI / 180.0;
    double dLon = (p2.longitude - p1.longitude) * PI / 180.0;
    
    double a = sin(dLat / 2.0) * sin(dLat / 2.0) +
               cos(lat1) * cos(lat2) *
               sin(dLon / 2.0) * sin(dLon / 2.0);
    
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    
    return EARTH_RADIUS_NM * c;
}

double Navigation::greatCircleBearing(const Position& p1, const Position& p2) const {
    double lat1 = p1.latitude * PI / 180.0;
    double lat2 = p2.latitude * PI / 180.0;
    double dLon = (p2.longitude - p1.longitude) * PI / 180.0;
    
    double y = sin(dLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
    
    double bearing = atan2(y, x) * 180.0 / PI;
    
    // Normalize to 0-360
    return fmod(bearing + 360.0, 360.0);
}

} // namespace AICopilot

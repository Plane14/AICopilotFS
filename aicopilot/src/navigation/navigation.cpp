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
    
    try {
        while (std::getline(file, line)) {
            // Simple XML parsing for key elements
            if (line.find("<DepartureLLA>") != std::string::npos) {
                // Parse departure coordinates
                size_t start = line.find(">") + 1;
                size_t end = line.find("<", start);
                if (start == std::string::npos || end == std::string::npos) {
                    continue; // Skip malformed line
                }
                
                std::string coords = line.substr(start, end - start);
                // Format: "lat,lon,+altitude"
                std::istringstream iss(coords);
                std::string token;
                
                if (!std::getline(iss, token, ',')) continue;
                double lat = std::stod(token);
                
                if (!std::getline(iss, token, ',')) continue;
                double lon = std::stod(token);
                
                if (!std::getline(iss, token)) continue;
                // Skip +/- sign if present
                double alt = std::stod(token.substr(token.find_first_of("+-0123456789")));
                
                // Validate coordinates
                if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
                    continue; // Skip invalid coordinates
                }
                
                Waypoint wp;
                wp.id = plan.departure.empty() ? "DEPARTURE" : plan.departure;
                wp.position.latitude = lat;
                wp.position.longitude = lon;
                wp.position.altitude = alt;
                wp.altitude = alt;
                wp.position.heading = 0.0;
                wp.type = "AIRPORT";
                plan.waypoints.push_back(wp);
            }
            else if (line.find("<DestinationLLA>") != std::string::npos) {
                size_t start = line.find(">") + 1;
                size_t end = line.find("<", start);
                if (start == std::string::npos || end == std::string::npos) {
                    continue;
                }
                
                std::string coords = line.substr(start, end - start);
                std::istringstream iss(coords);
                std::string token;
                
                if (!std::getline(iss, token, ',')) continue;
                double lat = std::stod(token);
                
                if (!std::getline(iss, token, ',')) continue;
                double lon = std::stod(token);
                
                if (!std::getline(iss, token)) continue;
                double alt = std::stod(token.substr(token.find_first_of("+-0123456789")));
                
                // Validate coordinates
                if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
                    continue;
                }
                
                Waypoint wp;
                wp.id = plan.arrival.empty() ? "ARRIVAL" : plan.arrival;
                wp.position.latitude = lat;
                wp.position.longitude = lon;
                wp.position.altitude = alt;
                wp.altitude = alt;
                wp.position.heading = 0.0;
                wp.type = "AIRPORT";
                plan.waypoints.push_back(wp);
            }
            else if (line.find("<CruiseAltitude>") != std::string::npos) {
                size_t start = line.find(">") + 1;
                size_t end = line.find("<", start);
                if (start != std::string::npos && end != std::string::npos) {
                    try {
                        double altitude = std::stod(line.substr(start, end - start));
                        if (altitude >= 0.0 && altitude <= 60000.0) {
                            plan.cruiseAltitude = altitude;
                        }
                    } catch (...) {
                        // Ignore invalid altitude
                    }
                }
            }
            else if (line.find("<Title>") != std::string::npos) {
                size_t start = line.find(">") + 1;
                size_t end = line.find("<", start);
                if (start != std::string::npos && end != std::string::npos) {
                    std::string title = line.substr(start, end - start);
                    // Parse title for departure/arrival (format: "ICAO to ICAO")
                    size_t toPos = title.find(" to ");
                    if (toPos != std::string::npos) {
                        plan.departure = title.substr(0, toPos);
                        plan.arrival = title.substr(toPos + 4);
                    }
                }
            }
        }
    } catch (const std::exception&) {
        // Return false if parsing fails
        return false;
    }
    
    // Only accept the plan if it has at least one waypoint
    if (plan.waypoints.empty()) {
        return false;
    }
    
    flightPlan_ = plan;
    activeWaypointIndex_ = 0;
    return true;
}

bool Navigation::parseFMS(std::ifstream& file) {
    // Parse FMS format (text-based)
    std::string line;
    FlightPlan plan;
    int lineNum = 0;
    
    try {
        while (std::getline(file, line)) {
            lineNum++;
            
            // Skip header lines (typically first 2-3 lines)
            if (lineNum <= 2) continue;
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }
            
            // Parse waypoint line
            std::istringstream iss(line);
            std::string waypointType, id;
            double lat, lon, alt;
            
            if (iss >> waypointType >> id >> lat >> lon >> alt) {
                // Validate coordinates
                if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
                    continue; // Skip invalid coordinates
                }
                
                // Validate altitude
                if (alt < -1000.0 || alt > 60000.0) {
                    continue; // Skip invalid altitude
                }
                
                Waypoint wp;
                wp.id = id;
                wp.position.latitude = lat;
                wp.position.longitude = lon;
                wp.position.altitude = alt;
                wp.altitude = alt;
                wp.position.heading = 0.0;
                wp.type = waypointType;
                plan.waypoints.push_back(wp);
                
                // Set departure from first waypoint
                if (lineNum == 3 && !plan.waypoints.empty()) {
                    plan.departure = plan.waypoints[0].id;
                }
            }
        }
    } catch (const std::exception&) {
        // Return false if parsing fails
        return false;
    }
    
    // Validate we have at least 2 waypoints
    if (plan.waypoints.size() < 2) {
        return false;
    }
    
    // Set arrival from last waypoint
    plan.arrival = plan.waypoints.back().id;
    
    // Estimate cruise altitude from waypoints (use maximum altitude)
    plan.cruiseAltitude = 0;
    for (const auto& wp : plan.waypoints) {
        if (wp.altitude > plan.cruiseAltitude) {
            plan.cruiseAltitude = wp.altitude;
        }
    }
    
    // Calculate headings for each waypoint based on next waypoint
    for (size_t i = 0; i < plan.waypoints.size() - 1; i++) {
        plan.waypoints[i].position.heading = greatCircleBearing(
            plan.waypoints[i].position,
            plan.waypoints[i + 1].position
        );
    }
    
    // Last waypoint heading is same as second-to-last
    if (plan.waypoints.size() > 1) {
        plan.waypoints.back().position.heading = 
            plan.waypoints[plan.waypoints.size() - 2].position.heading;
    }
    
    flightPlan_ = plan;
    activeWaypointIndex_ = 0;
    return true;
}

FlightPlan Navigation::createDirectPlan(const std::string& departure, 
                                       const std::string& arrival,
                                       double cruiseAltitude) {
    FlightPlan plan;
    plan.departure = departure;
    plan.arrival = arrival;
    plan.cruiseAltitude = cruiseAltitude;
    
    // Validate input parameters
    if (departure.empty() || arrival.empty()) {
        // Return empty plan if inputs are invalid
        return plan;
    }
    
    if (cruiseAltitude <= 0.0) {
        // Set default cruise altitude based on route length
        // Short routes: 5000ft, medium: 10000ft, long: FL350
        cruiseAltitude = 10000.0; // Default to 10,000 feet
    }
    
    // Create departure waypoint
    // In a full implementation, this would query the navdata database or SimConnect
    // for the actual airport coordinates. For now, we create placeholder waypoints
    // that should be populated with actual coordinates from an external source.
    Waypoint departureWP;
    departureWP.id = departure;
    departureWP.type = "AIRPORT";
    departureWP.altitude = 0.0; // Field elevation, to be populated
    departureWP.position.altitude = 0.0;
    departureWP.position.latitude = 0.0;  // To be populated from database
    departureWP.position.longitude = 0.0; // To be populated from database
    departureWP.position.heading = 0.0;
    
    // Create arrival waypoint
    Waypoint arrivalWP;
    arrivalWP.id = arrival;
    arrivalWP.type = "AIRPORT";
    arrivalWP.altitude = 0.0; // Field elevation, to be populated
    arrivalWP.position.altitude = 0.0;
    arrivalWP.position.latitude = 0.0;  // To be populated from database
    arrivalWP.position.longitude = 0.0; // To be populated from database
    arrivalWP.position.heading = 0.0;
    
    // In production, coordinates should be obtained via:
    // 1. SimConnect facility requests (RequestFacilitiesList)
    // 2. Navdata database lookup (using navdatareader)
    // 3. External API or embedded airport database
    
    // For a direct route, we only need departure and arrival
    // A more sophisticated implementation would add:
    // - SID waypoints after departure
    // - Enroute waypoints based on airway routing
    // - STAR waypoints before arrival
    // - Top of climb (TOC) and top of descent (TOD) points
    
    // Only add waypoints if we have valid coordinates
    // This prevents corrupted flight plans with zero coordinates
    bool hasValidDeparture = (departureWP.position.latitude != 0.0 || 
                              departureWP.position.longitude != 0.0);
    bool hasValidArrival = (arrivalWP.position.latitude != 0.0 || 
                            arrivalWP.position.longitude != 0.0);
    
    if (hasValidDeparture) {
        plan.waypoints.push_back(departureWP);
    }
    
    // Create intermediate cruise waypoint if both airports have valid coordinates
    if (hasValidDeparture && hasValidArrival) {
        // Calculate midpoint for cruise waypoint
        double midLat = (departureWP.position.latitude + arrivalWP.position.latitude) / 2.0;
        double midLon = (departureWP.position.longitude + arrivalWP.position.longitude) / 2.0;
        
        Waypoint cruiseWP;
        cruiseWP.id = "CRUISE";
        cruiseWP.type = "FIX";
        cruiseWP.altitude = cruiseAltitude;
        cruiseWP.position.latitude = midLat;
        cruiseWP.position.longitude = midLon;
        cruiseWP.position.altitude = cruiseAltitude;
        cruiseWP.position.heading = greatCircleBearing(departureWP.position, arrivalWP.position);
        
        plan.waypoints.push_back(cruiseWP);
    }
    
    if (hasValidArrival) {
        plan.waypoints.push_back(arrivalWP);
    }
    
    // Set cruise speed based on altitude (rough estimation)
    // This should ideally come from aircraft performance data
    if (cruiseAltitude < 10000.0) {
        plan.cruiseSpeed = 150.0; // knots - typical for GA aircraft
    } else if (cruiseAltitude < 25000.0) {
        plan.cruiseSpeed = 250.0; // knots - typical for turboprops/light jets
    } else {
        plan.cruiseSpeed = 450.0; // knots - typical for jets at cruise
    }
    
    flightPlan_ = plan;
    activeWaypointIndex_ = 0;
    
    return plan;
}

Waypoint Navigation::getActiveWaypoint() const {
    if (activeWaypointIndex_ < flightPlan_.waypoints.size()) {
        return flightPlan_.waypoints[activeWaypointIndex_];
    }
    
    // Return empty waypoint if index out of bounds
    Waypoint empty;
    empty.id = "";
    empty.type = "";
    empty.altitude = 0.0;
    empty.position.latitude = 0.0;
    empty.position.longitude = 0.0;
    empty.position.altitude = 0.0;
    empty.position.heading = 0.0;
    return empty;
}

Waypoint Navigation::getNextWaypoint() const {
    if (activeWaypointIndex_ + 1 < flightPlan_.waypoints.size()) {
        return flightPlan_.waypoints[activeWaypointIndex_ + 1];
    }
    
    // Return empty waypoint if no next waypoint
    Waypoint empty;
    empty.id = "";
    empty.type = "";
    empty.altitude = 0.0;
    empty.position.latitude = 0.0;
    empty.position.longitude = 0.0;
    empty.position.altitude = 0.0;
    empty.position.heading = 0.0;
    return empty;
}

void Navigation::advanceWaypoint() {
    if (activeWaypointIndex_ < flightPlan_.waypoints.size()) {
        activeWaypointIndex_++;
    }
    
    // Ensure we don't go past the last waypoint
    if (activeWaypointIndex_ > flightPlan_.waypoints.size()) {
        activeWaypointIndex_ = flightPlan_.waypoints.size();
    }
}

double Navigation::distanceToWaypoint(const Position& current, const Waypoint& waypoint) const {
    return greatCircleDistance(current, waypoint.position);
}

double Navigation::bearingToWaypoint(const Position& current, const Waypoint& waypoint) const {
    return greatCircleBearing(current, waypoint.position);
}

double Navigation::crossTrackError(const Position& current) const {
    // Need at least 2 waypoints and not be at the first waypoint
    if (flightPlan_.waypoints.size() < 2 || activeWaypointIndex_ == 0) {
        return 0.0;
    }
    
    // Make sure we're not past the end
    if (activeWaypointIndex_ >= flightPlan_.waypoints.size()) {
        return 0.0;
    }
    
    // Calculate cross-track error from previous to current waypoint
    Waypoint prev = flightPlan_.waypoints[activeWaypointIndex_ - 1];
    Waypoint next = getActiveWaypoint();
    
    // Calculate distances and bearings
    double d13 = greatCircleDistance(prev.position, current);
    
    // If we're very close to the previous waypoint, XTE is essentially zero
    if (d13 < 0.01) {
        return 0.0;
    }
    
    double brg13 = greatCircleBearing(prev.position, current);
    double brg12 = greatCircleBearing(prev.position, next.position);
    
    // Calculate angular difference
    double angularDiff = brg13 - brg12;
    
    // Calculate cross-track distance
    double xte = asin(sin(d13 / EARTH_RADIUS_NM) * sin(angularDiff * PI / 180.0)) * EARTH_RADIUS_NM;
    
    return xte;
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
    // Validate ground speed
    if (groundSpeed <= 0.0) {
        return 0.0;
    }
    
    // Need at least one more waypoint
    if (activeWaypointIndex_ >= flightPlan_.waypoints.size()) {
        return 0.0;
    }
    
    // If only one waypoint left, it's the active one
    if (activeWaypointIndex_ == flightPlan_.waypoints.size() - 1) {
        return 0.0;
    }
    
    double totalDistance = 0.0;
    
    // Sum distances from active waypoint to destination
    for (size_t i = activeWaypointIndex_; i < flightPlan_.waypoints.size() - 1; i++) {
        totalDistance += greatCircleDistance(
            flightPlan_.waypoints[i].position,
            flightPlan_.waypoints[i + 1].position
        );
    }
    
    // Convert to minutes (distance in NM, speed in knots)
    return (totalDistance / groundSpeed) * 60.0;
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

bool Navigation::updateWaypointPosition(const std::string& waypointId, const Position& position) {
    bool updated = false;
    
    // Search through all waypoints and update matching ones
    for (auto& waypoint : flightPlan_.waypoints) {
        if (waypoint.id == waypointId) {
            waypoint.position = position;
            waypoint.altitude = position.altitude;
            updated = true;
        }
    }
    
    return updated;
}

bool Navigation::validateFlightPlan() const {
    if (flightPlan_.waypoints.empty()) {
        return false;
    }
    
    // Check that all waypoints have valid (non-zero) coordinates
    for (const auto& waypoint : flightPlan_.waypoints) {
        // At least one coordinate must be non-zero for a valid position
        if (waypoint.position.latitude == 0.0 && waypoint.position.longitude == 0.0) {
            return false;
        }
        
        // Validate coordinate ranges
        if (waypoint.position.latitude < -90.0 || waypoint.position.latitude > 90.0) {
            return false;
        }
        
        if (waypoint.position.longitude < -180.0 || waypoint.position.longitude > 180.0) {
            return false;
        }
        
        // Validate altitude is reasonable (not negative, not in space)
        if (waypoint.altitude < -1000.0 || waypoint.altitude > 60000.0) {
            return false;
        }
    }
    
    return true;
}

void Navigation::addWaypoint(const Waypoint& waypoint) {
    flightPlan_.waypoints.push_back(waypoint);
}

void Navigation::insertWaypoint(size_t index, const Waypoint& waypoint) {
    if (index <= flightPlan_.waypoints.size()) {
        flightPlan_.waypoints.insert(flightPlan_.waypoints.begin() + index, waypoint);
        
        // Adjust active waypoint index if necessary
        if (index <= activeWaypointIndex_) {
            activeWaypointIndex_++;
        }
    }
}

void Navigation::removeWaypoint(size_t index) {
    if (index < flightPlan_.waypoints.size()) {
        flightPlan_.waypoints.erase(flightPlan_.waypoints.begin() + index);
        
        // Adjust active waypoint index if necessary
        if (index < activeWaypointIndex_ && activeWaypointIndex_ > 0) {
            activeWaypointIndex_--;
        } else if (index == activeWaypointIndex_ && activeWaypointIndex_ >= flightPlan_.waypoints.size()) {
            // If we removed the active waypoint and it was the last one, go back one
            if (activeWaypointIndex_ > 0) {
                activeWaypointIndex_--;
            }
        }
    }
}

double Navigation::getTotalDistance() const {
    if (flightPlan_.waypoints.size() < 2) {
        return 0.0;
    }
    
    double totalDistance = 0.0;
    
    for (size_t i = 0; i < flightPlan_.waypoints.size() - 1; i++) {
        totalDistance += greatCircleDistance(
            flightPlan_.waypoints[i].position,
            flightPlan_.waypoints[i + 1].position
        );
    }
    
    return totalDistance;
}

} // namespace AICopilot

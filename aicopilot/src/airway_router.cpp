/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Airway routing implementation using modified Dijkstra's algorithm
*****************************************************************************/

#include "../include/airway_router.hpp"
#include "../include/navdata_database.hpp"
#include <queue>
#include <map>
#include <set>
#include <limits>
#include <cmath>
#include <sstream>

namespace AICopilot {

// ============================================================================
// CONSTRUCTOR AND INITIALIZATION
// ============================================================================

AirwayRouter::AirwayRouter(const NavigationDatabase& database)
    : database_(database), maxSearchDistance_(10000.0), preferAirways_(true),
      minAltitudeConstraint_(1200), maxAltitudeConstraint_(60000) {
}

AirwayRouter::~AirwayRouter() {
}

// ============================================================================
// ROUTE FINDING
// ============================================================================

std::vector<RouteSegment> AirwayRouter::FindOptimalRoute(const std::string& origin,
                                                        const std::string& destination,
                                                        int cruiseAltitude) const {
    std::vector<RouteSegment> result;
    
    auto originWp = database_.GetWaypoint(origin);
    auto destWp = database_.GetWaypoint(destination);
    
    if (!originWp || !destWp) {
        return result;  // Empty route
    }
    
    // Use Dijkstra's algorithm to find optimal path
    std::map<std::string, double> distances;
    std::map<std::string, std::string> previous;
    std::map<std::string, std::string> viaAirway;
    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> pq;
    
    // Initialize
    distances[origin] = 0.0;
    pq.push({origin, 0.0, CalculateHeuristic(origin, destination), 0.0, {origin}});
    
    std::set<std::string> visited;
    
    while (!pq.empty()) {
        PathNode current = pq.top();
        pq.pop();
        
        if (visited.count(current.waypoint)) continue;
        visited.insert(current.waypoint);
        
        if (current.waypoint == destination) {
            // Reconstruct route
            std::vector<std::string> path = current.path;
            for (size_t i = 0; i < path.size() - 1; ++i) {
                RouteSegment segment;
                segment.fromWaypoint = path[i];
                segment.toWaypoint = path[i + 1];
                segment.distance = database_.CalculateDistance(path[i], path[i+1]);
                segment.heading = database_.CalculateHeading(path[i], path[i+1]);
                segment.airwayName = viaAirway[path[i+1]];
                
                auto fromWp = database_.GetWaypoint(path[i]);
                auto toWp = database_.GetWaypoint(path[i+1]);
                if (fromWp && toWp) {
                    segment.minimumAltitude = static_cast<int>(
                        std::max(fromWp->elevation, toWp->elevation) + 1000);
                    segment.maximumAltitude = 60000;
                }
                
                result.push_back(segment);
            }
            return result;
        }
        
        // Get adjacent waypoints
        auto adjacents = GetAdjacentWaypoints(current.waypoint, cruiseAltitude);
        
        for (const auto& next : adjacents) {
            if (visited.count(next)) continue;
            
            double cost = CalculateSegmentCost(current.waypoint, next, "");
            double newDist = distances[current.waypoint] + cost;
            
            if (distances.find(next) == distances.end() || newDist < distances[next]) {
                distances[next] = newDist;
                previous[next] = current.waypoint;
                
                double h = CalculateHeuristic(next, destination);
                PathNode pn;
                pn.waypoint = next;
                pn.gCost = newDist;
                pn.hCost = h;
                pn.fCost = newDist + h;
                pn.path = current.path;
                pn.path.push_back(next);
                
                pq.push(pn);
            }
        }
    }
    
    // If no path found via airways, return direct route as single segment
    RouteSegment direct = FindDirectRoute(origin, destination);
    if (!direct.fromWaypoint.empty()) {
        return std::vector<RouteSegment>(1, direct);
    }
    return std::vector<RouteSegment>();
}

RouteSegment AirwayRouter::FindDirectRoute(const std::string& origin,
                                          const std::string& destination) const {
    RouteSegment segment;
    
    auto originWp = database_.GetWaypoint(origin);
    auto destWp = database_.GetWaypoint(destination);
    
    if (!originWp || !destWp) {
        return segment;
    }
    
    segment.fromWaypoint = origin;
    segment.toWaypoint = destination;
    segment.airwayName = "DIRECT";
    segment.distance = database_.CalculateDistance(origin, destination);
    segment.heading = database_.CalculateHeading(origin, destination);
    segment.minimumAltitude = static_cast<int>(
        std::max(originWp->elevation, destWp->elevation) + 1000);
    segment.maximumAltitude = 60000;
    
    return segment;
}

std::vector<std::vector<RouteSegment>> AirwayRouter::FindAlternateRoutes(
    const std::string& origin, const std::string& destination,
    int maxResults) const {
    
    std::vector<std::vector<RouteSegment>> results;
    
    // Primary route
    auto primaryRoute = FindOptimalRoute(origin, destination, 35000);
    if (!primaryRoute.empty()) {
        results.push_back(primaryRoute);
    }
    
    // Direct route as alternate
    auto directSegment = FindDirectRoute(origin, destination);
    if (!directSegment.fromWaypoint.empty()) {
        results.push_back({directSegment});
    }
    
    // Could add more alternate routing logic here
    
    return results;
}

// ============================================================================
// PATHFINDING OPTIONS
// ============================================================================

void AirwayRouter::SetMaxSearchDistance(double distanceNM) {
    maxSearchDistance_ = distanceNM;
}

void AirwayRouter::SetPreferAirways(bool preferAirways) {
    preferAirways_ = preferAirways;
}

void AirwayRouter::SetAltitudeConstraints(int minAltitude, int maxAltitude) {
    minAltitudeConstraint_ = minAltitude;
    maxAltitudeConstraint_ = maxAltitude;
}

// ============================================================================
// ROUTE ANALYSIS
// ============================================================================

double AirwayRouter::CalculateRouteTotalDistance(const std::vector<RouteSegment>& route) {
    double total = 0.0;
    for (const auto& segment : route) {
        total += segment.distance;
    }
    return total;
}

double AirwayRouter::CalculateEstimatedTime(const std::vector<RouteSegment>& route,
                                           double groundSpeed) {
    if (groundSpeed <= 0.0) return 0.0;
    
    double totalDistance = CalculateRouteTotalDistance(route);
    return (totalDistance / groundSpeed) * 60.0;  // Convert to minutes
}

std::string AirwayRouter::FormatRoute(const std::vector<RouteSegment>& route) {
    std::ostringstream oss;
    
    for (size_t i = 0; i < route.size(); ++i) {
        const auto& seg = route[i];
        
        if (i == 0) {
            oss << seg.fromWaypoint;
        }
        
        if (!seg.airwayName.empty() && seg.airwayName != "DIRECT") {
            oss << " via " << seg.airwayName;
        } else {
            oss << " direct";
        }
        
        oss << " to " << seg.toWaypoint;
        
        if (i < route.size() - 1) {
            oss << ", ";
        }
    }
    
    return oss.str();
}

// ============================================================================
// PRIVATE HELPER METHODS
// ============================================================================

std::vector<std::string> AirwayRouter::GetAdjacentWaypoints(const std::string& waypoint,
                                                           int cruiseAltitude) const {
    std::vector<std::string> adjacents;
    
    // Get airways from this waypoint
    std::vector<Airway> airways;
    
    // Search for airways containing this waypoint
    // (This is a simplified approach; in production, use airway index)
    
    // For now, return nearby waypoints
    auto nearbyWaypoints = database_.GetWaypointsNearby(
        database_.GetWaypoint(waypoint) ? database_.GetWaypoint(waypoint)->latitude : 0.0,
        database_.GetWaypoint(waypoint) ? database_.GetWaypoint(waypoint)->longitude : 0.0,
        200.0);  // 200 NM radius
    
    for (const auto& wp : nearbyWaypoints) {
        if (wp.name != waypoint) {
            adjacents.push_back(wp.name);
        }
    }
    
    return adjacents;
}

std::vector<std::string> AirwayRouter::GetAirwaySuccessors(const std::string& waypoint,
                                                          const std::string& currentAirway) const {
    std::vector<std::string> successors;
    
    // Get next waypoints on the same airway or connecting airways
    auto airway = database_.GetAirway(currentAirway);
    if (airway) {
        auto it = std::find(airway->waypointSequence.begin(),
                           airway->waypointSequence.end(), waypoint);
        if (it != airway->waypointSequence.end() && it + 1 != airway->waypointSequence.end()) {
            successors.push_back(*(it + 1));
        }
    }
    
    return successors;
}

double AirwayRouter::CalculateSegmentCost(const std::string& from,
                                         const std::string& to,
                                         const std::string& airwayName) const {
    double distance = database_.CalculateDistance(from, to);
    
    // Cost = distance (+ airway preference factor)
    double cost = distance;
    
    if (preferAirways_ && !airwayName.empty()) {
        cost *= 0.9;  // 10% preference for airways
    }
    
    return cost;
}

double AirwayRouter::CalculateHeuristic(const std::string& current,
                                       const std::string& goal) const {
    // Use great circle distance as heuristic
    double distance = database_.CalculateDistance(current, goal);
    return distance < 0.0 ? std::numeric_limits<double>::max() : distance;
}

std::vector<RouteSegment> AirwayRouter::ReconstructRoute(
    const std::map<std::string, std::pair<std::string, std::string>>& cameFrom,
    const std::string& current) const {
    
    std::vector<RouteSegment> route;
    
    std::string current_node = current;
    while (cameFrom.find(current_node) != cameFrom.end()) {
        auto prev_and_airway = cameFrom.at(current_node);
        std::string prev = prev_and_airway.first;
        std::string airway = prev_and_airway.second;
        
        RouteSegment segment;
        segment.fromWaypoint = prev;
        segment.toWaypoint = current_node;
        segment.airwayName = airway;
        segment.distance = database_.CalculateDistance(prev, current_node);
        segment.heading = database_.CalculateHeading(prev, current_node);
        
        route.insert(route.begin(), segment);
        current_node = prev;
    }
    
    return route;
}

} // namespace AICopilot

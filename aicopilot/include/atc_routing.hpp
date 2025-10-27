#pragma once

#include "airport_data.hpp"
#include <queue>
#include <set>
#include <deque>
#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <limits>
#include <algorithm>

// ============================================================================
// PART 2: ATC GROUND ROUTING SYSTEMS
// ============================================================================

namespace AICopilot {
namespace ATC {

using namespace Airport;

// ============================================================================
// PART 2.1: Dijkstra's Shortest Path Algorithm
// ============================================================================

struct PathNode {
    int node_id;
    double cost_distance;               // Distance in feet
    double time_cost;                   // Time in seconds
    std::vector<int> path_edge_ids;    // Sequence of edge IDs
    
    PathNode(int id = 0, double dist = std::numeric_limits<double>::infinity())
        : node_id(id), cost_distance(dist), time_cost(0.0) {}
    
    // For priority queue (min-heap)
    bool operator>(const PathNode& other) const {
        return cost_distance > other.cost_distance;
    }
};

class GroundRouter {
private:
    const TaxiwayNetwork* taxiway_network;
    
public:
    struct RouteResult {
        bool success;
        std::vector<int> path_node_ids;         // Sequence of nodes
        std::vector<int> path_edge_ids;         // Sequence of edges
        double total_distance_feet;
        double estimated_time_seconds;
        std::string error_message;
        
        RouteResult() : success(false), total_distance_feet(0.0),
                        estimated_time_seconds(0.0) {}
    };
    
    GroundRouter() : taxiway_network(nullptr) {}
    
    void set_taxiway_network(const TaxiwayNetwork* network) {
        taxiway_network = network;
    }
    
    // Dijkstra's algorithm for shortest path
    RouteResult find_shortest_path(
        int start_node_id,
        int end_node_id,
        double max_aircraft_speed_knots = 15.0) const
    {
        RouteResult result;
        
        if (!taxiway_network) {
            result.error_message = "Taxiway network not set";
            return result;
        }
        
        // Validate start and end nodes
        if (!taxiway_network->get_node_const(start_node_id) ||
            !taxiway_network->get_node_const(end_node_id)) {
            result.error_message = "Invalid start or end node";
            return result;
        }
        
        // Initialize distances and visited set
        std::map<int, double> distances;
        std::map<int, int> previous_node;
        std::map<int, int> previous_edge;
        std::set<int> visited;
        
        // Get all nodes
        auto all_node_ids = taxiway_network->get_all_node_ids();
        for (int node_id : all_node_ids) {
            distances[node_id] = std::numeric_limits<double>::infinity();
        }
        distances[start_node_id] = 0.0;
        
        // Priority queue: (distance, node_id)
        std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> pq;
        pq.push(PathNode(start_node_id, 0.0));
        
        // Dijkstra main loop
        while (!pq.empty()) {
            PathNode current = pq.top();
            pq.pop();
            
            if (visited.count(current.node_id)) continue;
            visited.insert(current.node_id);
            
            if (current.node_id == end_node_id) {
                // Reconstruct path
                result.success = true;
                int node = end_node_id;
                double total_time = 0.0;
                
                while (node != start_node_id) {
                    result.path_node_ids.push_back(node);
                    int edge_id = previous_edge[node];
                    result.path_edge_ids.push_back(edge_id);
                    
                    const TaxiwayEdge* edge = taxiway_network->get_edge(edge_id);
                    if (edge) {
                        result.total_distance_feet += edge->length_feet;
                        total_time += edge->calculate_traversal_time_seconds(max_aircraft_speed_knots);
                    }
                    
                    node = previous_node[node];
                }
                result.path_node_ids.push_back(start_node_id);
                result.estimated_time_seconds = total_time;
                
                // Reverse to get correct order
                std::reverse(result.path_node_ids.begin(), result.path_node_ids.end());
                std::reverse(result.path_edge_ids.begin(), result.path_edge_ids.end());
                
                return result;
            }
            
            // Explore neighbors
            const auto& adjacent_edges = taxiway_network->get_adjacent_edges(current.node_id);
            
            for (int edge_id : adjacent_edges) {
                const TaxiwayEdge* edge = taxiway_network->get_edge(edge_id);
                if (!edge) continue;
                
                int neighbor_id = edge->to_node_id;
                if (visited.count(neighbor_id)) continue;
                
                double new_distance = distances[current.node_id] + edge->length_feet;
                
                if (new_distance < distances[neighbor_id]) {
                    distances[neighbor_id] = new_distance;
                    previous_node[neighbor_id] = current.node_id;
                    previous_edge[neighbor_id] = edge_id;
                    
                    pq.push(PathNode(neighbor_id, new_distance));
                }
            }
        }
        
        result.error_message = "No path found between nodes";
        return result;
    }
    
    // A* search (optimized for when destination is known)
    RouteResult find_fastest_path(
        int start_node_id,
        int end_node_id,
        const LatLonAlt& reference_point,
        double max_aircraft_speed_knots = 15.0) const
    {
        // Similar to Dijkstra but with heuristic function
        // h(n) = straight-line distance to goal / max_speed
        
        const TaxiwayNode* end_node = taxiway_network->get_node_const(end_node_id);
        if (!end_node) {
            RouteResult result;
            result.error_message = "End node not found";
            return result;
        }
        
        auto heuristic = [&](int node_id) -> double {
            const TaxiwayNode* node = taxiway_network->get_node_const(node_id);
            if (!node) return 0.0;
            double dist = node->position.distance_to(end_node->position);
            double max_speed_ft_per_s = max_aircraft_speed_knots * 1.6878;
            return dist / max_speed_ft_per_s;
        };
        
        // A* implementation similar to Dijkstra with f(n) = g(n) + h(n)
        std::map<int, double> g_scores;  // Cost from start
        std::map<int, double> f_scores;  // g + h
        
        auto all_node_ids = taxiway_network->get_all_node_ids();
        for (int node_id : all_node_ids) {
            g_scores[node_id] = std::numeric_limits<double>::infinity();
            f_scores[node_id] = std::numeric_limits<double>::infinity();
        }
        
        g_scores[start_node_id] = 0.0;
        f_scores[start_node_id] = heuristic(start_node_id);
        
        std::map<int, int> came_from;
        std::map<int, int> came_from_edge;
        std::set<int> open_set, closed_set;
        open_set.insert(start_node_id);
        
        while (!open_set.empty()) {
            // Find node with lowest f_score
            int current = *open_set.begin();
            double min_f = f_scores[current];
            
            for (int node : open_set) {
                if (f_scores[node] < min_f) {
                    min_f = f_scores[node];
                    current = node;
                }
            }
            
            if (current == end_node_id) {
                // Reconstruct path
                RouteResult result;
                result.success = true;
                
                int node = end_node_id;
                while (came_from.count(node)) {
                    result.path_node_ids.push_back(node);
                    int edge_id = came_from_edge[node];
                    result.path_edge_ids.push_back(edge_id);
                    
                    const TaxiwayEdge* edge = taxiway_network->get_edge(edge_id);
                    if (edge) {
                        result.total_distance_feet += edge->length_feet;
                        result.estimated_time_seconds += edge->calculate_traversal_time_seconds(max_aircraft_speed_knots);
                    }
                    
                    node = came_from[node];
                }
                result.path_node_ids.push_back(start_node_id);
                
                std::reverse(result.path_node_ids.begin(), result.path_node_ids.end());
                std::reverse(result.path_edge_ids.begin(), result.path_edge_ids.end());
                
                return result;
            }
            
            open_set.erase(current);
            closed_set.insert(current);
            
            const auto& neighbors = taxiway_network->get_adjacent_edges(current);
            for (int edge_id : neighbors) {
                const TaxiwayEdge* edge = taxiway_network->get_edge(edge_id);
                if (!edge) continue;
                
                int neighbor = edge->to_node_id;
                if (closed_set.count(neighbor)) continue;
                
                double tentative_g = g_scores[current] + edge->length_feet;
                
                if (tentative_g < g_scores[neighbor]) {
                    came_from[neighbor] = current;
                    came_from_edge[neighbor] = edge_id;
                    g_scores[neighbor] = tentative_g;
                    f_scores[neighbor] = g_scores[neighbor] + heuristic(neighbor);
                    
                    if (!open_set.count(neighbor)) {
                        open_set.insert(neighbor);
                    }
                }
            }
        }
        
        RouteResult result;
        result.error_message = "No path found (A*)";
        return result;
    }
};

// ============================================================================
// PART 2.2: Clearance State Machine
// ============================================================================

class ClearanceStateMachine {
public:
    enum class ClearanceState {
        Idle,                           // Aircraft not active
        PushbackRequested,              // Requesting push-back
        PushbackInProgress,             // Actively pushing back
        TaxiToRunway,                   // Cleared to taxi to assigned runway
        TaxiingToRunway,                // In progress
        HoldingAtRunway,                // Holding for takeoff clearance
        TakeoffCleared,                 // Cleared for takeoff
        ExecutingTakeoff,               // Rotating/taking off
        Airborne,                       // Successfully departed
        TaxiToParking,                  // Returned/landed, taxiing to parking
        TaxiingToParking,               // In progress
        ParkingArrived,                 // Parked
        GateHoldDueTraffic              // Ground stop
    };
    
    enum class ClearanceRequest {
        Pushback,
        TaxiClearance,
        TakeoffClearance,
        GoAround,
        HoldingPattern,
        LandingClearance
    };
    
private:
    int aircraft_id;
    ClearanceState current_state;
    ClearanceState previous_state;
    int assigned_runway_id;
    int assigned_parking_id;
    std::chrono::system_clock::time_point last_state_change;
    std::deque<ClearanceRequest> pending_requests;
    
    // Timing constraints
    double min_separation_seconds;      // Minimum time between departures
    double holding_time_limit_seconds;
    
public:
    ClearanceStateMachine(int aircraft_id = 0)
        : aircraft_id(aircraft_id),
          current_state(ClearanceState::Idle),
          previous_state(ClearanceState::Idle),
          assigned_runway_id(-1),
          assigned_parking_id(-1),
          min_separation_seconds(60.0),
          holding_time_limit_seconds(600.0)
    {
        last_state_change = std::chrono::system_clock::now();
    }
    
    // State transition functions
    bool request_pushback(int parking_id) {
        if (current_state != ClearanceState::Idle) return false;
        assigned_parking_id = parking_id;
        transition_state(ClearanceState::PushbackRequested);
        return true;
    }
    
    bool approve_pushback() {
        if (current_state != ClearanceState::PushbackRequested) return false;
        transition_state(ClearanceState::PushbackInProgress);
        return true;
    }
    
    bool complete_pushback() {
        if (current_state != ClearanceState::PushbackInProgress) return false;
        transition_state(ClearanceState::TaxiToRunway);
        return true;
    }
    
    bool request_taxi_clearance(int runway_id) {
        if (current_state != ClearanceState::TaxiToRunway) return false;
        assigned_runway_id = runway_id;
        transition_state(ClearanceState::TaxiingToRunway);
        return true;
    }
    
    bool arrive_at_runway_hold() {
        if (current_state != ClearanceState::TaxiingToRunway) return false;
        transition_state(ClearanceState::HoldingAtRunway);
        return true;
    }
    
    bool request_takeoff_clearance() {
        if (current_state != ClearanceState::HoldingAtRunway) return false;
        transition_state(ClearanceState::TakeoffCleared);
        return true;
    }
    
    bool begin_takeoff() {
        if (current_state != ClearanceState::TakeoffCleared) return false;
        transition_state(ClearanceState::ExecutingTakeoff);
        return true;
    }
    
    bool complete_takeoff() {
        if (current_state != ClearanceState::ExecutingTakeoff) return false;
        transition_state(ClearanceState::Airborne);
        return true;
    }
    
    bool initiate_landing() {
        if (current_state != ClearanceState::Airborne) return false;
        transition_state(ClearanceState::TaxiToParking);
        return true;
    }
    
    bool begin_landing_taxi(int landing_runway_id) {
        if (current_state != ClearanceState::TaxiToParking) return false;
        assigned_runway_id = landing_runway_id;
        transition_state(ClearanceState::TaxiingToParking);
        return true;
    }
    
    bool request_parking(int parking_id) {
        if (current_state != ClearanceState::TaxiingToParking) return false;
        assigned_parking_id = parking_id;
        return true;
    }
    
    bool complete_parking() {
        if (current_state != ClearanceState::TaxiingToParking) return false;
        transition_state(ClearanceState::ParkingArrived);
        return true;
    }
    
    // Getters
    ClearanceState get_state() const { return current_state; }
    ClearanceState get_previous_state() const { return previous_state; }
    int get_assigned_runway() const { return assigned_runway_id; }
    int get_assigned_parking() const { return assigned_parking_id; }
    int get_aircraft_id() const { return aircraft_id; }
    
    bool is_at_runway_hold() const { return current_state == ClearanceState::HoldingAtRunway; }
    bool is_taxiing() const {
        return current_state == ClearanceState::TaxiingToRunway ||
               current_state == ClearanceState::TaxiingToParking;
    }
    bool is_airborne() const { return current_state == ClearanceState::Airborne; }
    bool is_parked() const { return current_state == ClearanceState::ParkingArrived; }
    
    double get_time_in_current_state_seconds() const {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_state_change);
        return static_cast<double>(duration.count());
    }
    
private:
    void transition_state(ClearanceState new_state) {
        previous_state = current_state;
        current_state = new_state;
        last_state_change = std::chrono::system_clock::now();
    }
};

// ============================================================================
// PART 2.3: Holding Pattern Generation
// ============================================================================

struct HoldingPattern {
    LatLonAlt entry_point;
    LatLonAlt holding_fix;              // The fix around which we hold
    double holding_altitude_feet;
    double inbound_course;              // Heading into holding fix
    double leg_length_nm;               // Nautical miles (typically 1 NM for standard, 1.5 for high speed)
    double leg_time_minutes;            // Time to fly each leg
    bool turn_right;                    // Right turns vs left turns
    
    std::vector<LatLonAlt> waypoints;   // Calculated hold waypoints
    
    HoldingPattern()
        : holding_altitude_feet(0.0), inbound_course(0.0),
          leg_length_nm(1.0), leg_time_minutes(1.0), turn_right(true) {}
};

class HoldingPatternGenerator {
private:
    const AirportMaster* airport;
    
public:
    HoldingPatternGenerator() : airport(nullptr) {}
    
    void set_airport(const AirportMaster* apt) { airport = apt; }
    
    HoldingPattern generate_holding_pattern(
        const LatLonAlt& fix_position,
        double inbound_heading,
        double altitude_feet,
        bool turn_right = true,
        double leg_length_nm = 1.0) const
    {
        HoldingPattern pattern;
        pattern.holding_fix = fix_position;
        pattern.holding_altitude_feet = altitude_feet;
        pattern.inbound_course = inbound_heading;
        pattern.leg_length_nm = leg_length_nm;
        pattern.turn_right = turn_right;
        
        // Calculate leg time: distance/speed
        // Standard holding speed: 200 knots below 14,000 ft
        double holding_speed_knots = (altitude_feet < 14000.0) ? 200.0 : 230.0;
        if (altitude_feet > 34000.0) holding_speed_knots = 265.0;
        
        pattern.leg_time_minutes = (leg_length_nm / holding_speed_knots) * 60.0;
        
        // Generate holding pattern waypoints
        generate_hold_waypoints(pattern);
        
        return pattern;
    }
    
private:
    void generate_hold_waypoints(HoldingPattern& pattern) const {
        // Outbound leg: opposite of inbound heading
        double outbound_heading = pattern.inbound_course + 180.0;
        if (outbound_heading >= 360.0) outbound_heading -= 360.0;
        
        double turn_direction = pattern.turn_right ? 1.0 : -1.0;
        
        // Point 1: Entry point (on inbound course)
        LatLonAlt p1 = pattern.holding_fix;
        pattern.waypoints.push_back(p1);
        
        // Point 2: End of outbound leg
        double leg_distance_feet = pattern.leg_length_nm * 6076.12;  // NM to feet
        LatLonAlt p2 = project_position(p1, outbound_heading, leg_distance_feet);
        pattern.waypoints.push_back(p2);
        
        // Point 3: End of right/left turn (should be inbound heading)
        LatLonAlt p3 = project_position(p2, pattern.inbound_course, leg_distance_feet);
        pattern.waypoints.push_back(p3);
        
        // Point 4: Back to holding fix (start of next outbound)
        // (implicit, connects back to p1)
    }
    
    LatLonAlt project_position(
        const LatLonAlt& start,
        double heading_degrees,
        double distance_feet) const
    {
        // Simple projection using haversine
        double heading_rad = heading_degrees * DEG_TO_RAD;
        double distance_rad = distance_feet / EARTH_RADIUS_M;
        
        double lat1 = start.latitude * DEG_TO_RAD;
        double lon1 = start.longitude * DEG_TO_RAD;
        
        double lat2 = std::asin(
            std::sin(lat1) * std::cos(distance_rad) +
            std::cos(lat1) * std::sin(distance_rad) * std::cos(heading_rad)
        );
        
        double lon2 = lon1 + std::atan2(
            std::sin(heading_rad) * std::sin(distance_rad) * std::cos(lat1),
            std::cos(distance_rad) - std::sin(lat1) * std::sin(lat2)
        );
        
        return LatLonAlt(lat2 * RAD_TO_DEG, lon2 * RAD_TO_DEG, start.altitude);
    }
};

// ============================================================================
// PART 2.4: Runway Assignment Logic
// ============================================================================

class RunwayAssignment {
public:
    struct RunwayCandidate {
        Runway* runway;
        double wind_headwind_component;  // Negative = headwind, positive = tailwind
        double crosswind_component;      // Magnitude
        double suitability_score;        // 0-100, higher is better
        int queue_length;                // Number of aircraft waiting
        double wait_time_minutes;        // Estimated wait
        
        RunwayCandidate()
            : runway(nullptr), wind_headwind_component(0.0),
              crosswind_component(0.0), suitability_score(0.0),
              queue_length(0), wait_time_minutes(0.0) {}
    };
    
private:
    const AirportMaster* airport;
    
    // Constraints (can be configured)
    double max_headwind_knots;          // Typically 15 knots for small GA
    double max_crosswind_knots;         // Depends on aircraft type
    double max_tailwind_knots;          // Usually not allowed
    
public:
    RunwayAssignment()
        : airport(nullptr),
          max_headwind_knots(999.0),    // Effectively unlimited
          max_crosswind_knots(15.0),    // Can be overridden per aircraft
          max_tailwind_knots(0.0) {}
    
    void set_airport(const AirportMaster* apt) { airport = apt; }
    
    void set_wind_constraints(
        double max_head_knots,
        double max_cross_knots,
        double max_tail_knots)
    {
        max_headwind_knots = max_head_knots;
        max_crosswind_knots = max_cross_knots;
        max_tailwind_knots = max_tail_knots;
    }
    
    // Assign best runway based on weather and traffic
    RunwayCandidate assign_runway_for_departure(
        double wind_direction_degrees,
        double wind_speed_knots,
        const std::map<int, int>& runway_queue_lengths) const
    {
        if (!airport) {
            return RunwayCandidate();
        }
        
        const auto& runways = airport->get_runways();
        RunwayCandidate best_candidate;
        best_candidate.suitability_score = -1.0;
        
        for (const auto& runway : runways) {
            if (!runway.is_active) continue;
            
            RunwayCandidate candidate;
            candidate.runway = const_cast<Runway*>(&runway);
            
            // Calculate wind components
            candidate.wind_headwind_component = runway.calculate_wind_component(wind_direction_degrees, wind_speed_knots);
            candidate.crosswind_component = std::abs(runway.calculate_crosswind_component(wind_direction_degrees, wind_speed_knots));
            
            // Check constraints
            if (candidate.wind_headwind_component > max_tailwind_knots) {
                continue;  // Tailwind too strong
            }
            
            if (candidate.crosswind_component > max_crosswind_knots) {
                continue;  // Crosswind too strong
            }
            
            // Calculate suitability (0-100)
            candidate.suitability_score = 100.0;
            
            // Prefer headwind (negative = headwind, more negative is better)
            double headwind_preference = -candidate.wind_headwind_component;  // More is better
            candidate.suitability_score -= headwind_preference * 0.1;  // Reduce by headwind bonus weight
            
            // Penalize crosswind
            candidate.suitability_score -= candidate.crosswind_component * 2.0;
            
            // Prefer shorter queue
            auto queue_it = runway_queue_lengths.find(runway.runway_number);
            candidate.queue_length = (queue_it != runway_queue_lengths.end()) ? queue_it->second : 0;
            candidate.suitability_score -= candidate.queue_length * 10.0;
            
            if (candidate.suitability_score > best_candidate.suitability_score) {
                best_candidate = candidate;
            }
        }
        
        return best_candidate;
    }
    
    RunwayCandidate assign_runway_for_arrival(
        double wind_direction_degrees,
        double wind_speed_knots,
        const std::map<int, int>& runway_queue_lengths,
        double aircraft_ceiling_feet) const
    {
        // For arrivals, prefer into wind (headwind is good)
        return assign_runway_for_departure(wind_direction_degrees, wind_speed_knots, runway_queue_lengths);
    }
};

// ============================================================================
// PART 2.5: ATC Sequencing for Multiple Aircraft
// ============================================================================

struct SequencedAircraft {
    int aircraft_id;
    int sequence_number;                // Takeoff/landing sequence order
    double timestamp_queued;             // When added to sequence
    double estimated_ready_time;         // When ready for clearance
    double estimated_execution_time;    // When will execute
    ClearanceStateMachine::ClearanceState state;
    
    SequencedAircraft()
        : aircraft_id(-1), sequence_number(-1), timestamp_queued(0.0),
          estimated_ready_time(0.0), estimated_execution_time(0.0),
          state(ClearanceStateMachine::ClearanceState::Idle) {}
};

class ATCSequencer {
private:
    struct RunwayQueue {
        int runway_id;
        std::deque<int> departure_queue;    // Aircraft IDs
        std::deque<int> arrival_queue;
        double last_departure_time;
        double last_arrival_time;
        double minimum_separation_seconds;
        
        RunwayQueue(int id = 0)
            : runway_id(id), last_departure_time(0.0), last_arrival_time(0.0),
              minimum_separation_seconds(60.0) {}
    };
    
    std::map<int, RunwayQueue> runway_queues;
    std::map<int, ClearanceStateMachine> aircraft_clearances;
    std::map<int, SequencedAircraft> sequence_table;
    int next_sequence_number;
    
public:
    ATCSequencer() : next_sequence_number(1) {}
    
    void register_runway(int runway_id) {
        runway_queues[runway_id] = RunwayQueue(runway_id);
    }
    
    void set_minimum_separation(int runway_id, double separation_seconds) {
        auto it = runway_queues.find(runway_id);
        if (it != runway_queues.end()) {
            it->second.minimum_separation_seconds = separation_seconds;
        }
    }
    
    // Request departure slot
    bool request_departure_slot(int aircraft_id, int runway_id, double current_time) {
        auto queue_it = runway_queues.find(runway_id);
        if (queue_it == runway_queues.end()) return false;
        
        queue_it->second.departure_queue.push_back(aircraft_id);
        
        SequencedAircraft seq;
        seq.aircraft_id = aircraft_id;
        seq.sequence_number = next_sequence_number++;
        seq.timestamp_queued = current_time;
        seq.state = ClearanceStateMachine::ClearanceState::PushbackRequested;
        
        sequence_table[aircraft_id] = seq;
        aircraft_clearances[aircraft_id] = ClearanceStateMachine(aircraft_id);
        
        return true;
    }
    
    // Get next aircraft to clear for departure
    int get_next_departure_clearance(int runway_id, double current_time) {
        auto queue_it = runway_queues.find(runway_id);
        if (queue_it == runway_queues.end() || queue_it->second.departure_queue.empty()) {
            return -1;
        }
        
        // Check if enough time has passed since last departure
        double time_since_last = current_time - queue_it->second.last_departure_time;
        if (time_since_last < queue_it->second.minimum_separation_seconds) {
            return -1;  // Not enough separation
        }
        
        // Get next aircraft (could implement priority here)
        int aircraft_id = queue_it->second.departure_queue.front();
        queue_it->second.departure_queue.pop_front();
        queue_it->second.last_departure_time = current_time;
        
        return aircraft_id;
    }
    
    // Get next aircraft to clear for landing
    int get_next_arrival_clearance(int runway_id, double current_time) {
        auto queue_it = runway_queues.find(runway_id);
        if (queue_it == runway_queues.end() || queue_it->second.arrival_queue.empty()) {
            return -1;
        }
        
        double time_since_last = current_time - queue_it->second.last_arrival_time;
        if (time_since_last < queue_it->second.minimum_separation_seconds) {
            return -1;
        }
        
        int aircraft_id = queue_it->second.arrival_queue.front();
        queue_it->second.arrival_queue.pop_front();
        queue_it->second.last_arrival_time = current_time;
        
        return aircraft_id;
    }
    
    ClearanceStateMachine* get_aircraft_clearance(int aircraft_id) {
        auto it = aircraft_clearances.find(aircraft_id);
        return (it != aircraft_clearances.end()) ? &it->second : nullptr;
    }
    
    const SequencedAircraft* get_sequence_info(int aircraft_id) const {
        auto it = sequence_table.find(aircraft_id);
        return (it != sequence_table.end()) ? &it->second : nullptr;
    }
    
    size_t get_queue_length(int runway_id) const {
        auto it = runway_queues.find(runway_id);
        return (it != runway_queues.end()) ? it->second.departure_queue.size() : 0;
    }
};

} // namespace ATC
} // namespace AICopilot

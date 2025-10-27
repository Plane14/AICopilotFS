#pragma once

#include "airport_integration.hpp"
#include <iostream>
#include <iomanip>

// ============================================================================
// QUICK REFERENCE: PRACTICAL EXAMPLES & USAGE GUIDE
// ============================================================================

namespace AICopilot {
namespace Examples {

using namespace Airport;
using namespace ATC;
using namespace Collision;
using namespace Integration;

// ============================================================================
// EXAMPLE 1: Initialize Airport with Sample Data (KJFK)
// ============================================================================

void example_initialize_airport() {
    std::cout << "=== Example 1: Initialize Airport ===" << std::endl;
    
    AirportOperationSystem system;
    
    // Set airport reference point (JFK terminal area)
    LatLonAlt kjfk_reference(40.6413, -73.7781, 13.0);
    system.initialize_airport("KJFK", kjfk_reference);
    
    // Create runways
    std::vector<Runway> runways;
    
    // Runway 04L/22R
    Runway rwy_04l;
    rwy_04l.runway_number = 4;
    rwy_04l.runway_ident = "04L";
    rwy_04l.heading_true = 40.0;
    rwy_04l.length_feet = 14572.0;
    rwy_04l.width_feet = 200.0;
    rwy_04l.surface_type = Runway::Surface::Concrete;
    rwy_04l.lighting_type = Runway::Lighting::FullHighIntensity;
    rwy_04l.has_ils = true;
    rwy_04l.is_active = true;
    runways.push_back(rwy_04l);
    
    // Runway 13L/31R
    Runway rwy_13l;
    rwy_13l.runway_number = 13;
    rwy_13l.runway_ident = "13L";
    rwy_13l.heading_true = 130.0;
    rwy_13l.length_feet = 14511.0;
    rwy_13l.width_feet = 200.0;
    rwy_13l.surface_type = Runway::Surface::Asphalt;
    rwy_13l.lighting_type = Runway::Lighting::FullHighIntensity;
    rwy_13l.has_ils = true;
    rwy_13l.is_active = true;
    runways.push_back(rwy_13l);
    
    // Create taxiway network
    TaxiwayNetwork taxiway_net;
    
    // Taxiway nodes
    TaxiwayNode node_a1(1, LatLonAlt(40.6413, -73.7781, 13.0));
    node_a1.name = "A1";
    node_a1.type = TaxiwayNode::NodeType::Intersection;
    
    TaxiwayNode node_b(2, LatLonAlt(40.6420, -73.7775, 13.0));
    node_b.name = "B";
    node_b.type = TaxiwayNode::NodeType::Intersection;
    
    TaxiwayNode node_runway_hold(3, LatLonAlt(40.6430, -73.7770, 13.0));
    node_runway_hold.name = "RWY04L_HOLD";
    node_runway_hold.type = TaxiwayNode::NodeType::RunwayHold;
    
    taxiway_net.add_node(node_a1);
    taxiway_net.add_node(node_b);
    taxiway_net.add_node(node_runway_hold);
    
    // Taxiway edges (A1 <-> B <-> Runway Hold)
    TaxiwayEdge edge_a1_b(1, 1, 2, 1500.0);  // 1500 feet
    edge_a1_b.max_speed_knots = 15.0;
    edge_a1_b.is_bidirectional = true;
    
    TaxiwayEdge edge_b_rwy(2, 2, 3, 2000.0);
    edge_b_rwy.max_speed_knots = 10.0;  // Tighter section
    edge_b_rwy.is_bidirectional = true;
    
    taxiway_net.add_edge(edge_a1_b);
    taxiway_net.add_edge(edge_b_rwy);
    
    // Create parking positions
    std::vector<ParkingPosition> parkings;
    
    ParkingPosition parking_a1(101, LatLonAlt(40.6405, -73.7785, 13.0));
    parking_a1.gate_name = "A1";
    parking_a1.type = ParkingPosition::Type::Gate;
    parking_a1.has_jetway = true;
    parking_a1.max_wingspan_feet = 210.0;
    parking_a1.has_electrical = true;
    parking_a1.has_catering = true;
    parking_a1.has_fueling = true;
    
    ParkingPosition parking_a2(102, LatLonAlt(40.6402, -73.7784, 13.0));
    parking_a2.gate_name = "A2";
    parking_a2.type = ParkingPosition::Type::Gate;
    parking_a2.has_jetway = true;
    parking_a2.max_wingspan_feet = 210.0;
    
    parkings.push_back(parking_a1);
    parkings.push_back(parking_a2);
    
    // Load all data into system
    system.load_airport_data(runways, taxiway_net, parkings);
    
    std::cout << "Airport initialized: " << system.get_airport_const()->get_icao() << std::endl;
    std::cout << "Runways: " << runways.size() << std::endl;
    std::cout << "Taxiway nodes: " << taxiway_net.get_node_count() << std::endl;
    std::cout << "Parking positions: " << parkings.size() << std::endl;
}

// ============================================================================
// EXAMPLE 2: Request Taxi Route (Pathfinding)
// ============================================================================

void example_request_taxi_route() {
    std::cout << "\n=== Example 2: Request Taxi Route ===" << std::endl;
    
    AirportOperationSystem system;
    LatLonAlt kjfk_ref(40.6413, -73.7781, 13.0);
    system.initialize_airport("KJFK", kjfk_ref);
    
    // ... setup airport data (from Example 1) ...
    
    // Aircraft requests taxi from parking A1 (node 1) to runway hold (node 3)
    int start_node = 1;    // A1 intersection
    int end_node = 3;      // Runway hold
    double max_speed = 15.0;  // knots
    
    auto route = system.request_taxi_route(start_node, end_node, max_speed);
    
    if (route.success) {
        std::cout << "Route found!" << std::endl;
        std::cout << "Path: ";
        for (int node_id : route.path_node_ids) {
            std::cout << node_id << " -> ";
        }
        std::cout << std::endl;
        std::cout << "Distance: " << route.total_distance_feet << " feet" << std::endl;
        std::cout << "Est. Time: " << route.estimated_time_seconds << " seconds" << std::endl;
    } else {
        std::cout << "Error: " << route.error_message << std::endl;
    }
}

// ============================================================================
// EXAMPLE 3: Simulate Aircraft State & Update System
// ============================================================================

void example_aircraft_state_update() {
    std::cout << "\n=== Example 3: Aircraft State Update ===" << std::endl;
    
    AirportOperationSystem system;
    LatLonAlt kjfk_ref(40.6413, -73.7781, 13.0);
    system.initialize_airport("KJFK", kjfk_ref);
    
    // Simulate aircraft state
    SimConnectBridge::SimConnectData ac_state1;
    ac_state1.aircraft_id = 1;
    ac_state1.position = LatLonAlt(40.6415, -73.7780, 50.0);
    ac_state1.velocity = Vector2D(30.0, 10.0);  // ft/s
    ac_state1.heading_true = 45.0;
    ac_state1.altitude_feet = 50.0;
    ac_state1.groundspeed_knots = 25.0;
    ac_state1.wingspan_feet = 117.0;  // 737 sized
    ac_state1.fuselage_length_feet = 110.0;
    
    system.update_aircraft_state(ac_state1);
    
    SimConnectBridge::SimConnectData ac_state2;
    ac_state2.aircraft_id = 2;
    ac_state2.position = LatLonAlt(40.6420, -73.7775, 100.0);
    ac_state2.velocity = Vector2D(-20.0, -15.0);  // ft/s, converging
    ac_state2.heading_true = 225.0;
    ac_state2.altitude_feet = 100.0;
    ac_state2.groundspeed_knots = 22.0;
    ac_state2.wingspan_feet = 100.0;
    ac_state2.fuselage_length_feet = 100.0;
    
    system.update_aircraft_state(ac_state2);
    
    // Run update cycle
    system.update(0.1);  // 100ms
    
    std::cout << "Aircraft 1 position: " << ac_state1.position.latitude
              << ", " << ac_state1.position.longitude << std::endl;
    std::cout << "Aircraft 1 altitude: " << ac_state1.altitude_feet << " feet" << std::endl;
    std::cout << "Aircraft 1 groundspeed: " << ac_state1.groundspeed_knots << " knots" << std::endl;
}

// ============================================================================
// EXAMPLE 4: Runway Assignment Based on Weather
// ============================================================================

void example_runway_assignment() {
    std::cout << "\n=== Example 4: Runway Assignment ===" << std::endl;
    
    AirportOperationSystem system;
    LatLonAlt kjfk_ref(40.6413, -73.7781, 13.0);
    system.initialize_airport("KJFK", kjfk_ref);
    
    // ... setup runways ...
    
    // Current weather
    double wind_direction = 180.0;  // From south (opposite of runway 04)
    double wind_speed = 12.0;       // knots
    bool is_departure = true;
    
    auto assigned_runway = system.assign_runway(wind_direction, wind_speed, is_departure);
    
    if (assigned_runway.runway) {
        std::cout << "Assigned runway: " << assigned_runway.runway->runway_ident << std::endl;
        std::cout << "Runway heading: " << assigned_runway.runway->heading_true << std::endl;
        std::cout << "Headwind component: " << assigned_runway.wind_headwind_component << " knots" << std::endl;
        std::cout << "Crosswind component: " << assigned_runway.crosswind_component << " knots" << std::endl;
        std::cout << "Suitability score: " << assigned_runway.suitability_score << std::endl;
    }
}

// ============================================================================
// EXAMPLE 5: Clearance State Machine
// ============================================================================

void example_clearance_state_machine() {
    std::cout << "\n=== Example 5: Clearance State Machine ===" << std::endl;
    
    ClearanceStateMachine clearance(1);  // Aircraft ID 1
    
    std::cout << "Initial state: " << static_cast<int>(clearance.get_state()) << std::endl;
    
    // Request pushback
    if (clearance.request_pushback(101)) {
        std::cout << "Pushback requested" << std::endl;
        std::cout << "Current state: " << static_cast<int>(clearance.get_state()) << std::endl;
    }
    
    // Approve pushback
    if (clearance.approve_pushback()) {
        std::cout << "Pushback approved" << std::endl;
        std::cout << "Current state: " << static_cast<int>(clearance.get_state()) << std::endl;
    }
    
    // Complete pushback
    if (clearance.complete_pushback()) {
        std::cout << "Pushback completed" << std::endl;
        std::cout << "Current state: " << static_cast<int>(clearance.get_state()) << std::endl;
    }
    
    // Request taxi clearance
    if (clearance.request_taxi_clearance(1)) {
        std::cout << "Taxi clearance granted to runway " << clearance.get_assigned_runway() << std::endl;
        std::cout << "Current state: " << static_cast<int>(clearance.get_state()) << std::endl;
    }
    
    // Simulate time at runway hold
    std::cout << "Time in state: " << clearance.get_time_in_current_state_seconds() << " seconds" << std::endl;
}

// ============================================================================
// EXAMPLE 6: Collision Detection
// ============================================================================

void example_collision_detection() {
    std::cout << "\n=== Example 6: Collision Detection ===" << std::endl;
    
    // Circle collision
    Circle aircraft1(Vector2D(0, 0), 50);    // 50 ft radius
    Circle aircraft2(Vector2D(80, 0), 50);   // 80 ft away, 50 ft radius
    
    bool collision = CollisionDetector::check_circle_circle_collision(aircraft1, aircraft2);
    std::cout << "Aircraft 1 (0,0) r=50 vs Aircraft 2 (80,0) r=50: "
              << (collision ? "COLLISION" : "SAFE") << std::endl;
    
    // Closer aircraft
    Circle aircraft3(Vector2D(90, 0), 50);
    collision = CollisionDetector::check_circle_circle_collision(aircraft1, aircraft3);
    std::cout << "Aircraft 1 (0,0) r=50 vs Aircraft 3 (90,0) r=50: "
              << (collision ? "COLLISION" : "SAFE") << std::endl;
}

// ============================================================================
// EXAMPLE 7: Conflict Prediction (30-second lookahead)
// ============================================================================

void example_conflict_prediction() {
    std::cout << "\n=== Example 7: Conflict Prediction ===" << std::endl;
    
    ConflictPredictor predictor(30.0);  // 30-second prediction
    
    // Aircraft 1: stationary
    SeparationStandards::AircraftState ac1;
    ac1.aircraft_id = 1;
    ac1.position_local = Vector2D(0, 0);
    ac1.velocity = Vector2D(0, 0);
    ac1.altitude_feet = 1000.0;
    ac1.heading_true = 0.0;
    ac1.groundspeed_knots = 0.0;
    
    // Aircraft 2: approaching head-on at 200 ft/s
    SeparationStandards::AircraftState ac2;
    ac2.aircraft_id = 2;
    ac2.position_local = Vector2D(5000, 0);
    ac2.velocity = Vector2D(-200, 0);  // 200 ft/s converging
    ac2.altitude_feet = 1000.0;
    ac2.heading_true = 180.0;
    ac2.groundspeed_knots = 136.0;  // ~200 ft/s
    
    predictor.update_aircraft_state(ac1);
    predictor.update_aircraft_state(ac2);
    
    auto conflicts = predictor.predict_conflicts();
    
    std::cout << "Number of conflicts: " << conflicts.size() << std::endl;
    if (!conflicts.empty()) {
        for (const auto& conflict : conflicts) {
            std::cout << "Conflict between AC" << conflict.aircraft1_id
                      << " and AC" << conflict.aircraft2_id << std::endl;
            std::cout << "Time to conflict: " << conflict.time_to_conflict_seconds << " seconds" << std::endl;
            std::cout << "Conflict type: " << static_cast<int>(conflict.conflict_type) << std::endl;
        }
    }
}

// ============================================================================
// EXAMPLE 8: Avoidance Maneuver Selection
// ============================================================================

void example_avoidance_maneuver() {
    std::cout << "\n=== Example 8: Avoidance Maneuver Selection ===" << std::endl;
    
    ManeuverSelector selector;
    
    // Ownship aircraft
    SeparationStandards::AircraftState ownship;
    ownship.aircraft_id = 1;
    ownship.position_local = Vector2D(0, 0);
    ownship.velocity = Vector2D(200, 0);  // 200 ft/s heading east
    ownship.altitude_feet = 5000.0;
    ownship.heading_true = 90.0;
    ownship.groundspeed_knots = 136.0;
    ownship.wingspan_feet = 117.0;
    ownship.length_feet = 110.0;
    
    // Intruder aircraft
    SeparationStandards::AircraftState intruder;
    intruder.aircraft_id = 2;
    intruder.position_local = Vector2D(1000, 500);
    intruder.velocity = Vector2D(-100, -100);  // Converging southwest
    intruder.altitude_feet = 5000.0;
    intruder.heading_true = 225.0;
    intruder.groundspeed_knots = 95.0;
    
    auto maneuver = selector.select_avoidance_maneuver(ownship, intruder, false, false);
    
    std::cout << "Selected maneuver type: " << static_cast<int>(maneuver.maneuver_type) << std::endl;
    std::cout << "Description: " << maneuver.description << std::endl;
    std::cout << "Pilot workload: " << maneuver.pilot_workload << std::endl;
    std::cout << "Duration: " << maneuver.duration_seconds << " seconds" << std::endl;
}

// ============================================================================
// EXAMPLE 9: Holding Pattern Generation
// ============================================================================

void example_holding_pattern() {
    std::cout << "\n=== Example 9: Holding Pattern Generation ===" << std::endl;
    
    HoldingPatternGenerator holding_gen;
    
    // Fix location (e.g., outer marker)
    LatLonAlt holding_fix(40.6450, -73.7750, 0.0);
    
    // Generate holding pattern
    auto hold = holding_gen.generate_holding_pattern(
        holding_fix,
        180.0,              // Inbound heading
        3000.0,             // Altitude
        true,               // Turn right
        1.0                 // 1 nautical mile legs
    );
    
    std::cout << "Holding pattern at " << holding_fix.latitude
              << ", " << holding_fix.longitude << std::endl;
    std::cout << "Inbound course: " << hold.inbound_course << " degrees" << std::endl;
    std::cout << "Leg length: " << hold.leg_length_nm << " nautical miles" << std::endl;
    std::cout << "Leg time: " << hold.leg_time_minutes << " minutes" << std::endl;
    std::cout << "Waypoints in pattern: " << hold.waypoints.size() << std::endl;
}

// ============================================================================
// EXAMPLE 10: Multi-Aircraft Sequencing
// ============================================================================

void example_atc_sequencing() {
    std::cout << "\n=== Example 10: ATC Sequencing ===" << std::endl;
    
    ATCSequencer sequencer;
    sequencer.register_runway(1);  // Runway 1
    sequencer.set_minimum_separation(1, 60.0);  // 60-second spacing
    
    double current_time = 0.0;
    
    // Aircraft 1 requests departure
    sequencer.request_departure_slot(101, 1, current_time);
    std::cout << "Aircraft 101 queued for runway 1" << std::endl;
    std::cout << "Queue length: " << sequencer.get_queue_length(1) << std::endl;
    
    // Aircraft 2 requests departure
    sequencer.request_departure_slot(102, 1, current_time + 10.0);
    std::cout << "Aircraft 102 queued for runway 1" << std::endl;
    std::cout << "Queue length: " << sequencer.get_queue_length(1) << std::endl;
    
    // Get next departure
    int next_depart = sequencer.get_next_departure_clearance(1, current_time);
    std::cout << "Next departure: Aircraft " << next_depart << std::endl;
    
    // 70 seconds later, get next departure
    current_time += 70.0;
    next_depart = sequencer.get_next_departure_clearance(1, current_time);
    std::cout << "Next departure after 70 seconds: Aircraft " << next_depart << std::endl;
}

// ============================================================================
// HELPER: Print Vector
// ============================================================================

void print_vector2d(const Vector2D& v) {
    std::cout << "(" << std::fixed << std::setprecision(2)
              << v.x << ", " << v.y << ")";
}

} // namespace Examples
} // namespace AICopilot

// ============================================================================
// MAIN: Run Examples
// ============================================================================

/*
int main() {
    using namespace AICopilot::Examples;
    
    try {
        example_initialize_airport();
        example_request_taxi_route();
        example_aircraft_state_update();
        example_runway_assignment();
        example_clearance_state_machine();
        example_collision_detection();
        example_conflict_prediction();
        example_avoidance_maneuver();
        example_holding_pattern();
        example_atc_sequencing();
        
        std::cout << "\n=== All Examples Complete ===" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
*/

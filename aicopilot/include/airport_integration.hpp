#pragma once

#include "airport_data.hpp"
#include "atc_routing.hpp"
#include "collision_avoidance.hpp"
#include <memory>
#include <chrono>

// ============================================================================
// PART 4: IMPLEMENTATION APPROACH & INTEGRATION
// ============================================================================

namespace AICopilot {
namespace Integration {

using namespace Airport;
using namespace ATC;
using namespace Collision;

// ============================================================================
// 4.1: SIMCONNECT INTEGRATION LAYER
// ============================================================================

/**
 * Enum: SimConnect Simulation Variables needed for aircraft state
 * 
 * From SimConnect SDK (MSFS 2024):
 * - POSITION LATITUDE     : Aircraft latitude (degrees)
 * - POSITION LONGITUDE    : Aircraft longitude (degrees)
 * - POSITION ALTITUDE     : Aircraft altitude (feet MSL)
 * - HEADING TRUE          : True heading (degrees)
 * - GROUND VELOCITY       : Groundspeed (feet/second)
 * - VELOCITY WORLD X,Y,Z  : World velocity vector (feet/second)
 * - WING SPAN             : Aircraft wing span (feet)
 * - FUSELAGE LENGTH        : Aircraft length (feet)
 */

class SimConnectBridge {
public:
    struct SimConnectRequest {
        int request_id;
        int aircraft_id;
        // Additional request metadata
    };
    
    struct SimConnectData {
        int aircraft_id;
        // State data
        LatLonAlt position;
        Vector2D velocity;              // Local coordinate system
        double heading_true;
        double altitude_feet;
        double groundspeed_knots;
        double wingspan_feet;
        double fuselage_length_feet;
        
        // Timestamps
        std::chrono::system_clock::time_point timestamp;
        
        SimConnectData()
            : aircraft_id(-1), heading_true(0.0), altitude_feet(0.0),
              groundspeed_knots(0.0), wingspan_feet(100.0),
              fuselage_length_feet(100.0) {}
    };
    
    /**
     * Integration Points with SimConnect:
     * 
     * 1. INITIALIZATION:
     *    - Call SIMCONNECT_OPEN() to establish connection to MSFS
     *    - Register data definitions:
     *      SIMCONNECT_ADD_TO_DATA_DEFINITION(def_id, "POSITION LATITUDE", "degrees")
     *      SIMCONNECT_ADD_TO_DATA_DEFINITION(def_id, "POSITION LONGITUDE", "degrees")
     *      SIMCONNECT_ADD_TO_DATA_DEFINITION(def_id, "POSITION ALTITUDE", "feet")
     *      SIMCONNECT_ADD_TO_DATA_DEFINITION(def_id, "HEADING TRUE", "degrees")
     *      SIMCONNECT_ADD_TO_DATA_DEFINITION(def_id, "VELOCITY WORLD X", "feet/second")
     *      SIMCONNECT_ADD_TO_DATA_DEFINITION(def_id, "VELOCITY WORLD Y", "feet/second")
     *      SIMCONNECT_ADD_TO_DATA_DEFINITION(def_id, "VELOCITY WORLD Z", "feet/second")
     *      SIMCONNECT_ADD_TO_DATA_DEFINITION(def_id, "WING SPAN", "feet")
     *      SIMCONNECT_ADD_TO_DATA_DEFINITION(def_id, "FUSELAGE LENGTH", "feet")
     * 
     * 2. REQUEST CYCLE (every 100ms):
     *    - For each AI aircraft:
     *      SIMCONNECT_REQUEST_DATA_ON_SIM_OBJECT(request_id, def_id, obj_id)
     *      Where obj_id can be enumerated via SIMCONNECT_RECV_SIMOBJECT_DATA
     * 
     * 3. RECEIVE CYCLE (message pump):
     *    - Check for SIMCONNECT_RECV_SIMOBJECT_DATA messages
     *    - Extract SimConnectData structure
     *    - Feed into ATC/Collision systems
     * 
     * 4. COMMAND INJECTION:
     *    - For taxi guidance:
     *      SIMCONNECT_TRANSMIT_CLIENT_EVENT(obj_id, EVENT_TAXI_HOLD, data)
     *      SIMCONNECT_TRANSMIT_CLIENT_EVENT(obj_id, EVENT_TAXI_RUNWAY, data)
     * 
     *    - For clearances:
     *      Use navdata or guidance display systems (custom gauge)
     *      May require panel events or SimVar writes
     * 
     * 5. GAUGE INTEGRATION (for cockpit display):
     *    - Publish to SimVars:
     *      L:AICOPILOT_NEXT_CLEARANCE
     *      L:AICOPILOT_RUNWAY_ASSIGNED
     *      L:AICOPILOT_HOLD_PATTERN
     *    - Read from gauges via JavaScript/HTML5 SimConnect bridge
     */
    
private:
    // Connection handle would go here
    // HANDLE hSimConnect = nullptr;
    
public:
    SimConnectBridge() = default;
    
    // Pseudo-code for requesting aircraft data from SimConnect
    void request_aircraft_state(int aircraft_object_id) {
        // In real implementation:
        // SIMCONNECT_RECV_SIMOBJECT_DATA* pData = 
        //     (SIMCONNECT_RECV_SIMOBJECT_DATA*) pData;
        // 
        // Extract SimConnectData from SIMOBJECT_DATA structure
        // Feed to collision detection and routing systems
    }
    
    // Pseudo-code for sending taxi clearance to aircraft
    void transmit_taxi_clearance(
        int aircraft_object_id,
        const std::vector<int>& taxiway_node_sequence)
    {
        // In real implementation:
        // Could write route to L:CUSTOM_ROUTING_POINTS SimVar
        // Or transmit as series of position markers
    }
    
    // Pseudo-code for sending altitude clearance
    void transmit_altitude_clearance(int aircraft_object_id, double altitude_feet) {
        // Write to L:AICOPILOT_CLEARED_ALTITUDE SimVar
        // Frequency: every 1 second update
    }
    
    // Pseudo-code for receiving collision avoidance instruction
    void transmit_heading_instruction(int aircraft_object_id, double heading_true) {
        // Could write to autopilot target heading
        // Or display as advisory in custom gauge
    }
};

// ============================================================================
// 4.2: MASTER SYSTEM COORDINATOR
// ============================================================================

class AirportOperationSystem {
private:
    // Core systems
    std::unique_ptr<AirportMaster> airport;
    std::unique_ptr<GroundRouter> ground_router;
    std::unique_ptr<ATCSequencer> atc_sequencer;
    std::unique_ptr<RunwayAssignment> runway_assigner;
    std::unique_ptr<ConflictPredictor> conflict_predictor;
    std::unique_ptr<ConflictResolver> conflict_resolver;
    std::unique_ptr<ManeuverSelector> maneuver_selector;
    std::unique_ptr<HoldingPatternGenerator> holding_pattern_gen;
    
    // State tracking
    std::map<int, SimConnectBridge::SimConnectData> aircraft_states;
    std::map<int, ClearanceStateMachine> aircraft_clearances;
    
    // Timing
    std::chrono::system_clock::time_point last_collision_check;
    std::chrono::system_clock::time_point last_sequencing_update;
    double collision_check_interval_ms;
    double sequencing_update_interval_ms;
    
public:
    AirportOperationSystem()
        : collision_check_interval_ms(100.0),
          sequencing_update_interval_ms(1000.0)
    {
        ground_router = std::make_unique<GroundRouter>();
        atc_sequencer = std::make_unique<ATCSequencer>();
        runway_assigner = std::make_unique<RunwayAssignment>();
        conflict_predictor = std::make_unique<ConflictPredictor>(30.0);
        conflict_resolver = std::make_unique<ConflictResolver>();
        maneuver_selector = std::make_unique<ManeuverSelector>();
        holding_pattern_gen = std::make_unique<HoldingPatternGenerator>();
    }
    
    // ========================================================================
    // Initialization
    // ========================================================================
    
    void initialize_airport(const std::string& icao_code, const LatLonAlt& ref_point) {
        airport = std::make_unique<AirportMaster>(0, icao_code);
        airport->set_reference_point(ref_point);
        
        ground_router->set_taxiway_network(&airport->get_taxiway_network());
        runway_assigner->set_airport(airport.get());
        holding_pattern_gen->set_airport(airport.get());
    }
    
    // Load airport data (runways, taxiways, parking)
    void load_airport_data(
        const std::vector<Runway>& runways,
        const TaxiwayNetwork& taxiway_net,
        const std::vector<ParkingPosition>& parkings)
    {
        if (!airport) return;
        
        for (const auto& rwy : runways) {
            airport->add_runway(rwy);
        }
        
        // Deep copy taxiway network
        for (int node_id : taxiway_net.get_all_node_ids()) {
            const auto* node = taxiway_net.get_node_const(node_id);
            if (node) {
                airport->get_taxiway_network().add_node(*node);
            }
        }
        
        for (const auto& parking : parkings) {
            airport->add_parking(parking);
        }
    }
    
    // ========================================================================
    // Main Update Loop
    // ========================================================================
    
    void update(double delta_time_seconds) {
        auto now = std::chrono::system_clock::now();
        
        // 1. Update collision prediction (every 100ms)
        auto ms_since_collision_check = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_collision_check).count();
        
        if (ms_since_collision_check >= collision_check_interval_ms) {
            update_collision_system();
            last_collision_check = now;
        }
        
        // 2. Update ATC sequencing (every 1 second)
        auto ms_since_sequencing = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_sequencing_update).count();
        
        if (ms_since_sequencing >= sequencing_update_interval_ms) {
            update_atc_sequencing();
            last_sequencing_update = now;
        }
    }
    
    // ========================================================================
    // Collision Avoidance Update
    // ========================================================================
    
private:
    void update_collision_system() {
        if (!conflict_predictor) return;
        
        // 1. Update aircraft states in conflict predictor
        for (const auto& ac_state_pair : aircraft_states) {
            SeparationStandards::AircraftState sep_state;
            sep_state.aircraft_id = ac_state_pair.first;
            sep_state.position_local = ac_state_pair.second.position.to_local_xy(airport->get_reference_point());
            sep_state.velocity = ac_state_pair.second.velocity;
            sep_state.altitude_feet = ac_state_pair.second.altitude_feet;
            sep_state.heading_true = ac_state_pair.second.heading_true;
            sep_state.groundspeed_knots = ac_state_pair.second.groundspeed_knots;
            sep_state.wingspan_feet = ac_state_pair.second.wingspan_feet;
            sep_state.length_feet = ac_state_pair.second.fuselage_length_feet;
            
            conflict_predictor->update_aircraft_state(sep_state);
        }
        
        // 2. Predict conflicts
        auto conflicts = conflict_predictor->predict_conflicts();
        
        // 3. Generate resolution plan
        if (!conflicts.empty()) {
            std::map<int, SeparationStandards::AircraftState> ac_map;
            for (const auto& ac_state_pair : aircraft_states) {
                SeparationStandards::AircraftState sep_state;
                sep_state.aircraft_id = ac_state_pair.first;
                sep_state.position_local = ac_state_pair.second.position.to_local_xy(airport->get_reference_point());
                sep_state.velocity = ac_state_pair.second.velocity;
                sep_state.altitude_feet = ac_state_pair.second.altitude_feet;
                sep_state.heading_true = ac_state_pair.second.heading_true;
                sep_state.groundspeed_knots = ac_state_pair.second.groundspeed_knots;
                sep_state.wingspan_feet = ac_state_pair.second.wingspan_feet;
                sep_state.length_feet = ac_state_pair.second.fuselage_length_feet;
                ac_map[ac_state_pair.first] = sep_state;
            }
            
            auto resolution = conflict_resolver->resolve_multi_aircraft_conflicts(conflicts, ac_map);
            
            // 4. Apply resolution maneuvers to aircraft
            for (const auto& maneuver_pair : resolution.aircraft_maneuvers) {
                int aircraft_id = maneuver_pair.first;
                const auto& maneuver = maneuver_pair.second;
                
                // Send to SimConnect bridge or display
                apply_collision_avoidance_maneuver(aircraft_id, maneuver);
            }
        }
    }
    
    // ========================================================================
    // ATC Sequencing Update
    // ========================================================================
    
    void update_atc_sequencing() {
        // Process departure queue
        for (const auto& rwy : airport->get_runways()) {
            if (!rwy.is_active) continue;
            
            int next_to_depart = atc_sequencer->get_next_departure_clearance(
                rwy.runway_number,
                std::chrono::system_clock::now().time_since_epoch().count() / 1e9);
            
            if (next_to_depart >= 0) {
                auto* clearance = atc_sequencer->get_aircraft_clearance(next_to_depart);
                if (clearance && clearance->get_state() == ClearanceStateMachine::ClearanceState::PushbackRequested) {
                    clearance->approve_pushback();
                }
            }
        }
    }
    
    void apply_collision_avoidance_maneuver(
        int aircraft_id,
        const AvoidanceManeuver& maneuver)
    {
        // In real implementation, transmit maneuver to SimConnect
        // Example:
        // switch (maneuver.maneuver_type) {
        //     case AvoidanceManeuver::ManeuverType::TurnLeft:
        //     case AvoidanceManeuver::ManeuverType::TurnRight:
        //         simconnect_bridge->transmit_heading_instruction(aircraft_id, maneuver.new_heading_true);
        //         break;
        //     case AvoidanceManeuver::ManeuverType::ClimbTo:
        //         simconnect_bridge->transmit_altitude_clearance(aircraft_id, maneuver.new_altitude_feet);
        //         break;
        //     // ... other cases
        // }
    }

public:
    // ========================================================================
    // Public Interface: Aircraft Management
    // ========================================================================
    
    void update_aircraft_state(const SimConnectBridge::SimConnectData& data) {
        aircraft_states[data.aircraft_id] = data;
    }
    
    void remove_aircraft(int aircraft_id) {
        aircraft_states.erase(aircraft_id);
        conflict_predictor->remove_aircraft(aircraft_id);
    }
    
    // ========================================================================
    // Public Interface: Routing Requests
    // ========================================================================
    
    GroundRouter::RouteResult request_taxi_route(
        int start_node_id,
        int end_node_id,
        double max_speed_knots = 15.0)
    {
        return ground_router->find_shortest_path(start_node_id, end_node_id, max_speed_knots);
    }
    
    // ========================================================================
    // Public Interface: Runway Assignment
    // ========================================================================
    
    RunwayAssignment::RunwayCandidate assign_runway(
        double wind_direction,
        double wind_speed,
        bool is_departure)
    {
        std::map<int, int> queue_lengths;
        for (const auto& rwy : airport->get_runways()) {
            queue_lengths[rwy.runway_number] = atc_sequencer->get_queue_length(rwy.runway_number);
        }
        
        if (is_departure) {
            return runway_assigner->assign_runway_for_departure(wind_direction, wind_speed, queue_lengths);
        } else {
            return runway_assigner->assign_runway_for_arrival(wind_direction, wind_speed, queue_lengths, 5000.0);
        }
    }
    
    // ========================================================================
    // Accessors
    // ========================================================================
    
    AirportMaster* get_airport() { return airport.get(); }
    const AirportMaster* get_airport_const() const { return airport.get(); }
};

} // namespace Integration
} // namespace AICopilot

#endif // AICOPILOT_INTEGRATION_HPP

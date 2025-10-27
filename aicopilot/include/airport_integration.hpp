#pragma once

#include "airport_data.hpp"
#include "airport_manager.h"
#include "atc_routing.hpp"
#include "collision_avoidance.hpp"
#include "simconnect_wrapper.h"
#include <memory>
#include <chrono>
#include <functional>
#include <mutex>
#include <map>
#include <optional>
#include <string>
#include <vector>

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
    };

    struct SimConnectData {
        int aircraft_id;
        LatLonAlt position;
        Vector2D velocity;              // Local coordinate system
        double heading_true;
        double altitude_feet;
        double groundspeed_knots;
        double wingspan_feet;
        double fuselage_length_feet;
        std::chrono::system_clock::time_point timestamp;

        SimConnectData()
            : aircraft_id(-1), heading_true(0.0), altitude_feet(0.0),
              groundspeed_knots(0.0), wingspan_feet(100.0),
              fuselage_length_feet(100.0) {}
    };

    using AircraftStateCallback = std::function<void(const SimConnectData&)>;

    explicit SimConnectBridge(std::shared_ptr<SimConnectWrapper> simconnect);

    void set_state_callback(AircraftStateCallback callback);

    SimConnectData get_user_aircraft_state() const;

    struct AICommand {
        std::vector<int> taxiway_route;
        std::optional<double> target_heading_true;
        std::optional<double> target_altitude_feet;
        std::chrono::system_clock::time_point timestamp;
    };

    void transmit_taxi_clearance(int aircraft_object_id,
                                 const std::vector<int>& taxiway_node_sequence);

    void transmit_altitude_clearance(int aircraft_object_id, double altitude_feet);

    void transmit_heading_instruction(int aircraft_object_id, double heading_true);

    std::shared_ptr<SimConnectWrapper> get_simconnect() const { return simconnect_; }
    std::optional<AICommand> get_last_ai_command(int aircraft_id) const;

private:
    static constexpr int kUserAircraftId = 0;

    std::shared_ptr<SimConnectWrapper> simconnect_;
    AircraftStateCallback state_callback_;
    SimConnectData last_user_state_;
    mutable std::mutex data_mutex_;
    std::map<int, AICommand> ai_commands_;

    static SimConnectData convert_state(const AircraftState& state);
};

// ============================================================================
// 4.2: MASTER SYSTEM COORDINATOR
// ============================================================================

class AirportOperationSystem {
public:
    explicit AirportOperationSystem(std::shared_ptr<AirportManager> manager);

    void set_simconnect_bridge(std::shared_ptr<SimConnectBridge> bridge);

    bool initialize_airport(const std::string& icao_code, const LatLonAlt& ref_point);

    void load_airport_data(const std::vector<Runway>& runways,
                           const TaxiwayNetwork& taxiway_net,
                           const std::vector<ParkingPosition>& parkings);

    void update(double delta_time_seconds);

    void set_collision_update_interval(double milliseconds);
    void set_sequencing_update_interval(double milliseconds);

    void update_aircraft_state(const SimConnectBridge::SimConnectData& data);
    void remove_aircraft(int aircraft_id);

    GroundRouter::RouteResult request_taxi_route(int start_node_id,
                                                 int end_node_id,
                                                 double max_speed_knots = 15.0);

    RunwayAssignment::RunwayCandidate assign_runway(double wind_direction,
                                                    double wind_speed,
                                                    bool is_departure);

    void handle_atc_instruction(int aircraft_id, const std::string& instruction);
    void update_ai_traffic_state(const SimConnectBridge::SimConnectData& data);
    void clear_ai_traffic(int aircraft_id);

    const ClearanceStateMachine* get_clearance_state(int aircraft_id) const;

    AirportManager* get_airport_manager() { return airport_manager_.get(); }
    const AirportManager* get_airport_manager() const { return airport_manager_.get(); }
    std::shared_ptr<SimConnectBridge> get_simconnect_bridge() const { return simconnect_bridge_; }

private:
    std::shared_ptr<AirportManager> airport_manager_;
    std::shared_ptr<SimConnectBridge> simconnect_bridge_;

    std::unique_ptr<GroundRouter> ground_router_;
    std::unique_ptr<ATCSequencer> atc_sequencer_;
    std::unique_ptr<RunwayAssignment> runway_assigner_;
    std::unique_ptr<ConflictPredictor> conflict_predictor_;
    std::unique_ptr<ConflictResolver> conflict_resolver_;
    std::unique_ptr<ManeuverSelector> maneuver_selector_;
    std::unique_ptr<HoldingPatternGenerator> holding_pattern_gen_;

    std::map<int, SimConnectBridge::SimConnectData> aircraft_states_;

    double collision_check_interval_ms_;
    double sequencing_update_interval_ms_;
    double collision_elapsed_ms_;
    double sequencing_elapsed_ms_;

    static constexpr int kUserAircraftId = 0;

    ClearanceStateMachine& ensure_clearance_machine(int aircraft_id);
    void sync_airport_dependencies();
    void on_user_aircraft_state(const SimConnectBridge::SimConnectData& data);
    void update_collision_system();
    void update_atc_sequencing();
    void apply_collision_avoidance_maneuver(int aircraft_id, const AvoidanceManeuver& maneuver);
    RunwayAssignment::RunwayCandidate assign_runway_internal(double wind_direction,
                                                             double wind_speed,
                                                             bool is_departure);
};

} // namespace Integration
} // namespace AICopilot

#endif // AICOPILOT_INTEGRATION_HPP

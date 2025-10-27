#include "../../include/airport_integration.hpp"
#include "../../include/aicopilot_types.h"
#include <cmath>
#include <iostream>

namespace {
constexpr double kKnotsToFeetPerSecond = 1.6878098571011957;
}

namespace AICopilot {
namespace Integration {

// ===============================================
// SimConnectBridge
// ===============================================

SimConnectBridge::SimConnectBridge(std::shared_ptr<SimConnectWrapper> simconnect)
    : simconnect_(std::move(simconnect))
    , state_callback_(nullptr)
    , last_user_state_()
{
    if (simconnect_) {
        simconnect_->subscribeToAircraftState([this](const AircraftState& state) {
            SimConnectData data = convert_state(state);
            {
                std::lock_guard<std::mutex> lock(data_mutex_);
                last_user_state_ = data;
            }

            AircraftStateCallback callback_copy;
            {
                std::lock_guard<std::mutex> lock(data_mutex_);
                callback_copy = state_callback_;
            }

            if (callback_copy) {
                callback_copy(data);
            }
        });
    }
}

void SimConnectBridge::set_state_callback(AircraftStateCallback callback) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    state_callback_ = std::move(callback);
}

SimConnectBridge::SimConnectData SimConnectBridge::get_user_aircraft_state() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return last_user_state_;
}

void SimConnectBridge::transmit_taxi_clearance(int aircraft_object_id,
                                               const std::vector<int>& taxiway_node_sequence)
{
    if (taxiway_node_sequence.empty()) {
        return;
    }

    if (auto sim = simconnect_) {
        // TODO: Real AI aircraft taxi routing via SimConnect client events.
        (void)sim;
    }

    std::lock_guard<std::mutex> lock(data_mutex_);
    ai_commands_[aircraft_object_id] = AICommand{
        taxiway_node_sequence,
        std::nullopt,
        std::nullopt,
        std::chrono::system_clock::now()
    };
}

void SimConnectBridge::transmit_altitude_clearance(int aircraft_object_id, double altitude_feet) {
    if (!simconnect_) {
        return;
    }

    if (aircraft_object_id == kUserAircraftId) {
        simconnect_->setAutopilotAltitude(altitude_feet);
    }
    // For AI aircraft, integration with custom SimConnect events is required.

    std::lock_guard<std::mutex> lock(data_mutex_);
    auto& command = ai_commands_[aircraft_object_id];
    command.target_altitude_feet = altitude_feet;
    command.timestamp = std::chrono::system_clock::now();
}

void SimConnectBridge::transmit_heading_instruction(int aircraft_object_id, double heading_true) {
    if (!simconnect_) {
        return;
    }

    if (aircraft_object_id == kUserAircraftId) {
        simconnect_->setAutopilotHeading(heading_true);
    }
    // For AI aircraft, integration with custom SimConnect events is required.

    std::lock_guard<std::mutex> lock(data_mutex_);
    auto& command = ai_commands_[aircraft_object_id];
    command.target_heading_true = heading_true;
    command.timestamp = std::chrono::system_clock::now();
}

SimConnectBridge::SimConnectData SimConnectBridge::convert_state(const AircraftState& state) {
    SimConnectData data;
    data.aircraft_id = kUserAircraftId;
    data.position = LatLonAlt(state.position.latitude,
                              state.position.longitude,
                              state.position.altitude);
    double heading_rad = state.heading * M_PI / 180.0;
    double speed_ft_per_s = state.groundSpeed * kKnotsToFeetPerSecond;
    data.velocity = Vector2D(std::cos(heading_rad) * speed_ft_per_s,
                             std::sin(heading_rad) * speed_ft_per_s);
    data.heading_true = state.heading;
    data.altitude_feet = state.position.altitude;
    data.groundspeed_knots = state.groundSpeed;
    data.timestamp = std::chrono::system_clock::now();

    // Default dimensions until specific aircraft data is provided.
    data.wingspan_feet = 100.0;
    data.fuselage_length_feet = 100.0;

    return data;
}

std::optional<SimConnectBridge::AICommand> SimConnectBridge::get_last_ai_command(int aircraft_id) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = ai_commands_.find(aircraft_id);
    if (it == ai_commands_.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ===============================================
// AirportOperationSystem
// ===============================================

AirportOperationSystem::AirportOperationSystem(std::shared_ptr<AirportManager> manager)
    : airport_manager_(std::move(manager))
    , ground_router_(std::make_unique<GroundRouter>())
    , atc_sequencer_(std::make_unique<ATCSequencer>())
    , runway_assigner_(std::make_unique<RunwayAssignment>())
    , conflict_predictor_(std::make_unique<ConflictPredictor>(30.0))
    , conflict_resolver_(std::make_unique<ConflictResolver>())
    , maneuver_selector_(std::make_unique<ManeuverSelector>())
    , holding_pattern_gen_(std::make_unique<HoldingPatternGenerator>())
    , collision_check_interval_ms_(100.0)
    , sequencing_update_interval_ms_(1000.0)
    , collision_elapsed_ms_(0.0)
    , sequencing_elapsed_ms_(0.0)
{
    if (!airport_manager_) {
        airport_manager_ = std::make_shared<AirportManager>();
    }

    conflict_resolver_->set_maneuver_selector(maneuver_selector_.get());
}

void AirportOperationSystem::set_simconnect_bridge(std::shared_ptr<SimConnectBridge> bridge) {
    simconnect_bridge_ = std::move(bridge);

    if (simconnect_bridge_) {
        simconnect_bridge_->set_state_callback([this](const SimConnectBridge::SimConnectData& data) {
            on_user_aircraft_state(data);
        });
    }
}

bool AirportOperationSystem::initialize_airport(const std::string& icao_code, const LatLonAlt& ref_point) {
    bool initialized = airport_manager_->initialize(icao_code, ref_point);
    if (initialized) {
        sync_airport_dependencies();
    }
    return initialized;
}

void AirportOperationSystem::load_airport_data(const std::vector<Runway>& runways,
                                               const TaxiwayNetwork& taxiway_net,
                                               const std::vector<ParkingPosition>& parkings)
{
    airport_manager_->set_runways(runways);
    airport_manager_->set_taxiway_network(taxiway_net);
    airport_manager_->set_parking_positions(parkings);
    sync_airport_dependencies();
}

void AirportOperationSystem::update(double delta_time_seconds) {
    const auto* airport = airport_manager_->get_airport_const();
    if (!airport) {
        return;
    }

    auto now = std::chrono::system_clock::now();

    collision_elapsed_ms_ += delta_time_seconds * 1000.0;
    if (collision_elapsed_ms_ >= collision_check_interval_ms_) {
        update_collision_system();
        collision_elapsed_ms_ = 0.0;
    }

    sequencing_elapsed_ms_ += delta_time_seconds * 1000.0;
    if (sequencing_elapsed_ms_ >= sequencing_update_interval_ms_) {
        update_atc_sequencing();
        sequencing_elapsed_ms_ = 0.0;
    }
}

void AirportOperationSystem::set_collision_update_interval(double milliseconds) {
    collision_check_interval_ms_ = milliseconds;
}

void AirportOperationSystem::set_sequencing_update_interval(double milliseconds) {
    sequencing_update_interval_ms_ = milliseconds;
}

void AirportOperationSystem::update_aircraft_state(const SimConnectBridge::SimConnectData& data) {
    aircraft_states_[data.aircraft_id] = data;
    ensure_clearance_machine(data.aircraft_id);
}

void AirportOperationSystem::remove_aircraft(int aircraft_id) {
    aircraft_states_.erase(aircraft_id);
    conflict_predictor_->remove_aircraft(aircraft_id);
    aircraft_clearances_.erase(aircraft_id);
}

GroundRouter::RouteResult AirportOperationSystem::request_taxi_route(int start_node_id,
                                                                     int end_node_id,
                                                                     double max_speed_knots)
{
    if (!ground_router_) {
        return {};
    }
    return ground_router_->find_shortest_path(start_node_id, end_node_id, max_speed_knots);
}

RunwayAssignment::RunwayCandidate AirportOperationSystem::assign_runway(double wind_direction,
                                                                        double wind_speed,
                                                                        bool is_departure)
{
    return assign_runway_internal(wind_direction, wind_speed, is_departure);
}

void AirportOperationSystem::handle_atc_instruction(int aircraft_id, const std::string& instruction) {
    auto& clearance = ensure_clearance_machine(aircraft_id);
    std::string lower = instruction;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("pushback") != std::string::npos && lower.find("approved") != std::string::npos) {
        if (clearance.get_state() == ClearanceStateMachine::ClearanceState::Idle) {
            clearance.request_pushback(0);
        }
        clearance.approve_pushback();
    } else if (lower.find("pushback complete") != std::string::npos) {
        if (clearance.get_state() == ClearanceStateMachine::ClearanceState::Idle) {
            clearance.request_pushback(0);
            clearance.approve_pushback();
        }
        clearance.complete_pushback();
    } else if (lower.find("taxi to") != std::string::npos || lower.find("taxi") != std::string::npos) {
        if (lower.find("runway") != std::string::npos) {
            int runway_id = 0;
            const auto& airport = airport_manager_->get_airport_const();
            if (airport) {
                for (const auto& runway : airport->get_runways()) {
                    if (lower.find(runway.runway_ident) != std::string::npos) {
                        runway_id = runway.runway_number;
                        break;
                    }
                }
            }
            if (clearance.get_state() == ClearanceStateMachine::ClearanceState::Idle) {
                clearance.request_pushback(0);
                clearance.approve_pushback();
                clearance.complete_pushback();
            }
            clearance.request_taxi_clearance(runway_id);
        }
    } else if (lower.find("hold short") != std::string::npos || lower.find("hold position") != std::string::npos) {
        if (clearance.get_state() == ClearanceStateMachine::ClearanceState::TaxiToRunway ||
            clearance.get_state() == ClearanceStateMachine::ClearanceState::TaxiingToRunway) {
            clearance.arrive_at_runway_hold();
        }
    } else if (lower.find("cleared for takeoff") != std::string::npos) {
        if (clearance.get_state() != ClearanceStateMachine::ClearanceState::HoldingAtRunway) {
            clearance.arrive_at_runway_hold();
        }
        clearance.request_takeoff_clearance();
    } else if (lower.find("line up and wait") != std::string::npos) {
        clearance.arrive_at_runway_hold();
    } else if (lower.find("cleared to land") != std::string::npos) {
        if (clearance.get_state() != ClearanceStateMachine::ClearanceState::TaxiingToParking) {
            clearance.initiate_landing();
        }
        clearance.begin_landing_taxi(clearance.get_assigned_runway());
    } else if (lower.find("contact ground") != std::string::npos || lower.find("taxi to gate") != std::string::npos) {
        clearance.initiate_landing();
    }
}

ClearanceStateMachine& AirportOperationSystem::ensure_clearance_machine(int aircraft_id) {
    auto it = aircraft_clearances_.find(aircraft_id);
    if (it == aircraft_clearances_.end()) {
        it = aircraft_clearances_.emplace(aircraft_id, ClearanceStateMachine(aircraft_id)).first;
    }
    return it->second;
}

const ClearanceStateMachine* AirportOperationSystem::get_clearance_state(int aircraft_id) const {
    auto it = aircraft_clearances_.find(aircraft_id);
    if (it == aircraft_clearances_.end()) {
        return nullptr;
    }
    return &it->second;
}

void AirportOperationSystem::sync_airport_dependencies() {
    const auto* airport = airport_manager_->get_airport_const();
    if (!airport) {
        return;
    }

    ground_router_->set_taxiway_network(&airport->get_taxiway_network());
    runway_assigner_->set_airport(airport);
    holding_pattern_gen_->set_airport(airport);

    // Register runways for sequencing
    for (const auto& runway : airport->get_runways()) {
        atc_sequencer_->register_runway(runway.runway_number);
    }
}

void AirportOperationSystem::on_user_aircraft_state(const SimConnectBridge::SimConnectData& data) {
    update_aircraft_state(data);
}

void AirportOperationSystem::update_collision_system() {
    const auto* airport = airport_manager_->get_airport_const();
    if (!airport) {
        return;
    }

    const auto& reference = airport->get_reference_point();

    for (const auto& state_pair : aircraft_states_) {
        SeparationStandards::AircraftState sep_state;
        sep_state.aircraft_id = state_pair.first;
        sep_state.position_local = state_pair.second.position.to_local_xy(reference);
        sep_state.velocity = state_pair.second.velocity;
        sep_state.altitude_feet = state_pair.second.altitude_feet;
        sep_state.heading_true = state_pair.second.heading_true;
        sep_state.groundspeed_knots = state_pair.second.groundspeed_knots;
        sep_state.wingspan_feet = state_pair.second.wingspan_feet;
        sep_state.length_feet = state_pair.second.fuselage_length_feet;

        conflict_predictor_->update_aircraft_state(sep_state);
    }

    auto conflicts = conflict_predictor_->predict_conflicts();
    if (conflicts.empty()) {
        return;
    }

    std::map<int, SeparationStandards::AircraftState> aircraft_map;
    for (const auto& state_pair : aircraft_states_) {
        SeparationStandards::AircraftState sep_state;
        sep_state.aircraft_id = state_pair.first;
        sep_state.position_local = state_pair.second.position.to_local_xy(reference);
        sep_state.velocity = state_pair.second.velocity;
        sep_state.altitude_feet = state_pair.second.altitude_feet;
        sep_state.heading_true = state_pair.second.heading_true;
        sep_state.groundspeed_knots = state_pair.second.groundspeed_knots;
        sep_state.wingspan_feet = state_pair.second.wingspan_feet;
        sep_state.length_feet = state_pair.second.fuselage_length_feet;
        aircraft_map[state_pair.first] = sep_state;
    }

    auto resolution = conflict_resolver_->resolve_multi_aircraft_conflicts(conflicts, aircraft_map);

    for (const auto& maneuver_pair : resolution.aircraft_maneuvers) {
        apply_collision_avoidance_maneuver(maneuver_pair.first, maneuver_pair.second);
    }
}

void AirportOperationSystem::update_atc_sequencing() {
    const auto* airport = airport_manager_->get_airport_const();
    if (!airport) {
        return;
    }

    double current_time_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (const auto& runway : airport->get_runways()) {
        if (!runway.is_active) {
            continue;
        }

        int next_departure = atc_sequencer_->get_next_departure_clearance(runway.runway_number, current_time_seconds);
        if (next_departure < 0) {
            continue;
        }

        auto* clearance = atc_sequencer_->get_aircraft_clearance(next_departure);
        if (!clearance) {
            continue;
        }

        if (clearance->get_state() == ClearanceStateMachine::ClearanceState::PushbackRequested) {
            clearance->approve_pushback();
        }
    }
}

void AirportOperationSystem::apply_collision_avoidance_maneuver(int aircraft_id, const AvoidanceManeuver& maneuver) {
    if (!simconnect_bridge_) {
        return;
    }

    switch (maneuver.maneuver_type) {
        case AvoidanceManeuver::ManeuverType::TurnLeft:
        case AvoidanceManeuver::ManeuverType::TurnRight:
            simconnect_bridge_->transmit_heading_instruction(aircraft_id, maneuver.new_heading_true);
            break;
        case AvoidanceManeuver::ManeuverType::ClimbTo:
        case AvoidanceManeuver::ManeuverType::DescentTo:
            simconnect_bridge_->transmit_altitude_clearance(aircraft_id, maneuver.new_altitude_feet);
            break;
        default:
            break;
    }
}

RunwayAssignment::RunwayCandidate AirportOperationSystem::assign_runway_internal(double wind_direction,
                                                                                 double wind_speed,
                                                                                 bool is_departure)
{
    const auto* airport = airport_manager_->get_airport_const();
    if (!airport) {
        return {};
    }

    std::map<int, int> queue_lengths;
    for (const auto& runway : airport->get_runways()) {
        queue_lengths[runway.runway_number] = static_cast<int>(atc_sequencer_->get_queue_length(runway.runway_number));
    }

    if (is_departure) {
        return runway_assigner_->assign_runway_for_departure(wind_direction, wind_speed, queue_lengths);
    }

    return runway_assigner_->assign_runway_for_arrival(wind_direction, wind_speed, queue_lengths, 5000.0);
}

} // namespace Integration
} // namespace AICopilot

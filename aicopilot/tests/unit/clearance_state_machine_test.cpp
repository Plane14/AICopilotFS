#include <gtest/gtest.h>
#include "../../include/atc_routing.hpp"
#include "../../include/airport_integration.hpp"
#include "../../include/atc_controller.h"
#include "../../include/airport_manager.h"
#include "../../include/simconnect_wrapper.h"

using namespace AICopilot;
using namespace AICopilot::ATC;
using namespace AICopilot::Integration;

namespace {

class MockSimConnectWrapper : public SimConnectWrapper {
public:
    bool connect(SimulatorType, const std::string&) override { return true; }
    void processMessages() override {}
    bool isConnected() const override { return true; }
    void setAutopilotAltitude(double) override {}
    void setAutopilotHeading(double) override {}
    void subscribeToAircraftState(const AircraftStateCallback& callback) override { stateCallback_ = callback; }
    void subscribeToATCMessages(const ATCMessageCallback& callback) override { messageCallback_ = callback; }
    void sendATCMenuSelection(int) override {}
    void requestATCMenu() override {}

    void emitState(const AircraftState& state) {
        if (stateCallback_) stateCallback_(state);
    }

    void emitMessage(const ATCMessage& msg) {
        if (messageCallback_) messageCallback_(msg);
    }

private:
    AircraftStateCallback stateCallback_;
    ATCMessageCallback messageCallback_;
};

class AirportOpsTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        simconnect_ = std::make_shared<MockSimConnectWrapper>();
        airportManager_ = std::make_shared<AirportManager>();
        airportOps_ = std::make_unique<AirportOperationSystem>(airportManager_);
        simBridge_ = std::make_shared<SimConnectBridge>(simconnect_);
        airportOps_->set_simconnect_bridge(simBridge_);

        // Minimal airport data
        Airport::LatLonAlt ref(40.0, -73.0, 10.0);
        airportOps_->initialize_airport("TEST", ref);

        std::vector<Airport::Runway> runways;
        Airport::Runway runway;
        runway.runway_number = 4;
        runway.runway_ident = "04";
        runway.heading_true = 40.0;
        runway.length_feet = 10000.0;
        runway.width_feet = 150.0;
        runway.surface_type = Airport::Runway::Surface::Asphalt;
        runway.is_active = true;
        runways.push_back(runway);

        Airport::TaxiwayNetwork network;
        Airport::TaxiwayNode gate(1, Airport::LatLonAlt(ref.latitude + 0.0001, ref.longitude + 0.0001, ref.altitude));
        gate.type = Airport::TaxiwayNode::NodeType::ParkingArea;
        network.add_node(gate);
        Airport::TaxiwayNode hold(2, Airport::LatLonAlt(ref.latitude + 0.0003, ref.longitude + 0.0003, ref.altitude));
        hold.type = Airport::TaxiwayNode::NodeType::RunwayHold;
        network.add_node(hold);
        network.add_edge(Airport::TaxiwayEdge(1, 1, 2, 500.0));

        std::vector<Airport::ParkingPosition> parking;
        Airport::ParkingPosition spot(10, gate.position);
        spot.type = Airport::ParkingPosition::Type::Gate;
        parking.push_back(spot);

        airportOps_->load_airport_data(runways, network, parking);
    }

    std::shared_ptr<MockSimConnectWrapper> simconnect_;
    std::shared_ptr<AirportManager> airportManager_;
    std::unique_ptr<AirportOperationSystem> airportOps_;
    std::shared_ptr<SimConnectBridge> simBridge_;
};

} // namespace

TEST(ClearanceStateMachineTest, InitialStateIsIdle) {
    ClearanceStateMachine machine(0);
    EXPECT_EQ(machine.get_state(), ClearanceStateMachine::ClearanceState::Idle);
    EXPECT_EQ(machine.get_assigned_runway(), -1);
}

TEST(ClearanceStateMachineTest, FullDepartureSequence) {
    ClearanceStateMachine machine(1);
    EXPECT_TRUE(machine.request_pushback(10));
    EXPECT_TRUE(machine.approve_pushback());
    EXPECT_TRUE(machine.complete_pushback());
    EXPECT_TRUE(machine.request_taxi_clearance(4));
    EXPECT_TRUE(machine.arrive_at_runway_hold());
    EXPECT_TRUE(machine.request_takeoff_clearance());
    EXPECT_TRUE(machine.begin_takeoff());
    EXPECT_TRUE(machine.complete_takeoff());
    EXPECT_EQ(machine.get_state(), ClearanceStateMachine::ClearanceState::Airborne);
}

TEST(ClearanceStateMachineTest, RejectInvalidTransitions) {
    ClearanceStateMachine machine(2);
    EXPECT_FALSE(machine.complete_pushback());
    EXPECT_FALSE(machine.request_takeoff_clearance());
    EXPECT_TRUE(machine.request_pushback(0));
    EXPECT_FALSE(machine.request_taxi_clearance(4));
}

TEST_F(AirportOpsTestFixture, ParsesTakeoffInstruction) {
    airportOps_->handle_atc_instruction(0, "Cleared for takeoff runway 04");
    const auto* clearance = airportOps_->get_clearance_state(0);
    ASSERT_NE(clearance, nullptr);
    EXPECT_EQ(clearance->get_state(), ClearanceStateMachine::ClearanceState::TakeoffCleared);
}

TEST_F(AirportOpsTestFixture, TaxiInstructionRequestsRoute) {
    airportOps_->handle_atc_instruction(0, "Taxi to runway 04 via Alfa");
    const auto* clearance = airportOps_->get_clearance_state(0);
    ASSERT_NE(clearance, nullptr);
    EXPECT_EQ(clearance->get_state(), ClearanceStateMachine::ClearanceState::TaxiingToRunway);
}

TEST_F(AirportOpsTestFixture, UpdateLoopProcessesCollisionSequencing) {
    airportOps_->update_aircraft_state({0, Airport::LatLonAlt(40.0, -73.0, 10.0), Airport::Vector2D(0.0, 0.0), 0.0, 10.0, 0.0, 0.0, 0.0});
    airportOps_->set_collision_update_interval(10.0);
    airportOps_->set_sequencing_update_interval(20.0);
    airportOps_->update(0.02); // 20 ms
    airportOps_->update(0.6);  // 600 ms triggers both collision and sequencing updates
    SUCCEED();
}

TEST(ATCControllerTest, AnalyzesAltHeadingSpeed) {
    auto parsed = ATCController::analyzeInstruction("Climb and maintain flight level 180, heading 250, speed 250");
    ASSERT_TRUE(parsed.target_altitude_feet.has_value());
    EXPECT_NEAR(parsed.target_altitude_feet.value(), 18000.0, 1e-3);
    ASSERT_TRUE(parsed.target_heading_degrees.has_value());
    EXPECT_NEAR(parsed.target_heading_degrees.value(), 250.0, 1e-3);
    ASSERT_TRUE(parsed.target_speed_knots.has_value());
    EXPECT_NEAR(parsed.target_speed_knots.value(), 250.0, 1e-3);
}

TEST(ATCControllerTest, ParsesInstructionAndUpdatesPendingList) {
    auto sim = std::make_shared<MockSimConnectWrapper>();
    ATCController controller(sim);
    auto airportManager = std::make_shared<AirportManager>();
    AirportOperationSystem airportOps(airportManager);
    auto bridge = std::make_shared<SimConnectBridge>(sim);
    airportOps.set_simconnect_bridge(bridge);
    Airport::LatLonAlt ref(40.0, -73.0, 10.0);
    airportOps.initialize_airport("TEST", ref);
    controller.setAirportOperations(&airportOps);

    controller.processATCMessage({"Taxi to runway 04", {}, ATCMessage::MessageType::Clearance});
    controller.update();

    auto pending = controller.getPendingInstructions();
    ASSERT_FALSE(pending.empty());
    EXPECT_EQ(pending.back(), "Taxiing to runway");
}

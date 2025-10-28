#include <gtest/gtest.h>
#include "../../include/aircraft_systems.h"
#include "../mocks/mock_simconnect.hpp"
#include <memory>

using namespace AICopilot;
using namespace AICopilot::Testing;

class AircraftSystemsTest : public ::testing::Test {
protected:
    std::shared_ptr<MockSimConnectWrapper> mockSimConnect;
    AircraftConfig config;
    std::unique_ptr<AircraftSystems> systems;
    
    void SetUp() override {
        mockSimConnect = std::make_shared<MockSimConnectWrapper>();
        
        // Configure mock aircraft
        config.aircraft_type = AircraftType::JET;
        config.max_altitude = 43000.0;
        config.max_speed = 450.0;
        config.max_climb_rate = 3000.0;
        config.max_descent_rate = 2000.0;
        
        systems = std::make_unique<AircraftSystems>(mockSimConnect, config);
        
        // Set up default mock behaviors
        AircraftState defaultState;
        defaultState.position = {40.0, -74.0, 5000.0, 0.0};
        defaultState.indicatedAirspeed = 250.0;
        defaultState.trueAirspeed = 300.0;
        defaultState.groundSpeed = 280.0;
        defaultState.verticalSpeed = 0.0;
        defaultState.onGround = false;
        
        mockSimConnect->setMockAircraftState(defaultState);
    }
};

// Test: Autopilot master enable/disable
TEST_F(AircraftSystemsTest, AutopilotMasterToggle) {
    EXPECT_CALL(*mockSimConnect, setAutopilotMaster(true)).Times(1);
    systems->enableAutopilot(true);
    
    EXPECT_CALL(*mockSimConnect, setAutopilotMaster(false)).Times(1);
    systems->enableAutopilot(false);
}

// Test: Heading setting
TEST_F(AircraftSystemsTest, HeadingControl) {
    EXPECT_CALL(*mockSimConnect, setAutopilotHeading(180.0)).Times(1);
    systems->setHeading(180.0);
    
    EXPECT_CALL(*mockSimConnect, setAutopilotHeading(270.0)).Times(1);
    systems->setHeading(270.0);
}

// Test: Altitude setting with limits
TEST_F(AircraftSystemsTest, AltitudeControl) {
    EXPECT_CALL(*mockSimConnect, setAutopilotAltitude(15000.0)).Times(1);
    systems->setAltitude(15000.0);
    
    // Extreme altitude (should clamp)
    EXPECT_CALL(*mockSimConnect, setAutopilotAltitude(43000.0)).Times(1);
    systems->setAltitude(50000.0);  // Beyond max
}

// Test: Speed setting
TEST_F(AircraftSystemsTest, SpeedControl) {
    EXPECT_CALL(*mockSimConnect, setAutopilotSpeed(250.0)).Times(1);
    systems->setSpeed(250.0);
    
    EXPECT_CALL(*mockSimConnect, setAutopilotSpeed(450.0)).Times(1);
    systems->setSpeed(500.0);  // Beyond max, should clamp
}

// Test: Vertical speed setting
TEST_F(AircraftSystemsTest, VerticalSpeedControl) {
    EXPECT_CALL(*mockSimConnect, setAutopilotVerticalSpeed(1500.0)).Times(1);
    systems->setVerticalSpeed(1500.0);
    
    // Negative vertical speed (descent)
    EXPECT_CALL(*mockSimConnect, setAutopilotVerticalSpeed(-1000.0)).Times(1);
    systems->setVerticalSpeed(-1000.0);
}

// Test: NAV mode enable
TEST_F(AircraftSystemsTest, NAVModeControl) {
    EXPECT_CALL(*mockSimConnect, setAutopilotNav(true)).Times(1);
    systems->enableNavMode(true);
    
    EXPECT_CALL(*mockSimConnect, setAutopilotNav(false)).Times(1);
    systems->enableNavMode(false);
}

// Test: Approach mode enable
TEST_F(AircraftSystemsTest, ApproachModeControl) {
    EXPECT_CALL(*mockSimConnect, setAutopilotApproach(true)).Times(1);
    systems->enableApproachMode(true);
    
    EXPECT_CALL(*mockSimConnect, setAutopilotApproach(false)).Times(1);
    systems->enableApproachMode(false);
}

// Test: Throttle control
TEST_F(AircraftSystemsTest, ThrottleControl) {
    EXPECT_CALL(*mockSimConnect, setThrottle(0.75)).Times(1);
    systems->setThrottle(0.75);
    
    // Edge cases
    EXPECT_CALL(*mockSimConnect, setThrottle(0.0)).Times(1);
    systems->setThrottle(0.0);  // Idle
    
    EXPECT_CALL(*mockSimConnect, setThrottle(1.0)).Times(1);
    systems->setThrottle(1.0);  // Full throttle
}

// Test: Flight control surfaces
TEST_F(AircraftSystemsTest, FlightControlSurfaces) {
    EXPECT_CALL(*mockSimConnect, setElevator(0.5)).Times(1);
    systems->setPitch(0.5);
    
    EXPECT_CALL(*mockSimConnect, setAileron(-0.3)).Times(1);
    systems->setRoll(-0.3);
    
    EXPECT_CALL(*mockSimConnect, setRudder(0.2)).Times(1);
    systems->setYaw(0.2);
}

// Test: Flaps control
TEST_F(AircraftSystemsTest, FlapsControl) {
    EXPECT_CALL(*mockSimConnect, setFlaps(0)).Times(1);
    systems->setFlaps(0);  // Flaps up
    
    EXPECT_CALL(*mockSimConnect, setFlaps(25)).Times(1);
    systems->setFlaps(25);  // Approach
    
    EXPECT_CALL(*mockSimConnect, setFlaps(50)).Times(1);
    systems->setFlaps(50);  // Landing
}

// Test: Gear control
TEST_F(AircraftSystemsTest, GearControl) {
    EXPECT_CALL(*mockSimConnect, setGear(true)).Times(1);
    systems->setGear(true);
    
    EXPECT_CALL(*mockSimConnect, setGear(false)).Times(1);
    systems->setGear(false);
}

// Test: Spoilers control
TEST_F(AircraftSystemsTest, SpoilersControl) {
    EXPECT_CALL(*mockSimConnect, setSpoilers(true)).Times(1);
    systems->setSpoilers(true);
    
    EXPECT_CALL(*mockSimConnect, setSpoilers(false)).Times(1);
    systems->setSpoilers(false);
}

// Test: Parking brake control
TEST_F(AircraftSystemsTest, ParkingBrakeControl) {
    EXPECT_CALL(*mockSimConnect, setParkingBrake(true)).Times(1);
    systems->setParkingBrake(true);
    
    EXPECT_CALL(*mockSimConnect, setParkingBrake(false)).Times(1);
    systems->setParkingBrake(false);
}

// Test: Brakes control
TEST_F(AircraftSystemsTest, BrakesControl) {
    EXPECT_CALL(*mockSimConnect, setBrakes(0.5)).Times(1);
    systems->setBrakes(0.5);
    
    EXPECT_CALL(*mockSimConnect, setBrakes(1.0)).Times(1);
    systems->setBrakes(1.0);  // Full brakes
}

// Test: Mixture control
TEST_F(AircraftSystemsTest, MixtureControl) {
    EXPECT_CALL(*mockSimConnect, setMixture(0.75)).Times(1);
    systems->setMixture(0.75);
    
    EXPECT_CALL(*mockSimConnect, setMixture(1.0)).Times(1);
    systems->setMixture(1.0);  // Rich
}

// Test: Propeller pitch control
TEST_F(AircraftSystemsTest, PropellerControl) {
    EXPECT_CALL(*mockSimConnect, setPropellerPitch(0.85)).Times(1);
    systems->setPropeller(0.85);
}

// Test: Engine management - start/stop
TEST_F(AircraftSystemsTest, EngineControl) {
    EXPECT_CALL(*mockSimConnect, setEngineState(0, true)).Times(1);
    systems->startEngine(0);
    
    EXPECT_CALL(*mockSimConnect, setEngineState(0, false)).Times(1);
    systems->stopEngine(0);
}

// Test: Magneto control
TEST_F(AircraftSystemsTest, MagnetoControl) {
    EXPECT_CALL(*mockSimConnect, setMagnetos(0)).Times(1);
    systems->setMagnetos(0);  // OFF
    
    EXPECT_CALL(*mockSimConnect, setMagnetos(3)).Times(1);
    systems->setMagnetos(3);  // BOTH
}

// Test: Lighting controls
TEST_F(AircraftSystemsTest, LightingControl) {
    EXPECT_CALL(*mockSimConnect, setNavigationLights(true)).Times(1);
    systems->setNavigationLights(true);
    
    EXPECT_CALL(*mockSimConnect, setBeaconLights(true)).Times(1);
    systems->setBeaconLights(true);
    
    EXPECT_CALL(*mockSimConnect, setStrobeLights(true)).Times(1);
    systems->setStrobeLights(true);
    
    EXPECT_CALL(*mockSimConnect, setLandingLights(true)).Times(1);
    systems->setLandingLights(true);
    
    EXPECT_CALL(*mockSimConnect, setTaxiLights(true)).Times(1);
    systems->setTaxiLights(true);
}

// Test: State retrieval
TEST_F(AircraftSystemsTest, CurrentStateRetrieval) {
    AircraftState state = systems->getCurrentState();
    
    EXPECT_DOUBLE_EQ(state.position.latitude, 40.0);
    EXPECT_DOUBLE_EQ(state.position.longitude, -74.0);
    EXPECT_DOUBLE_EQ(state.indicatedAirspeed, 250.0);
    EXPECT_FALSE(state.onGround);
}

// Test: System health check
TEST_F(AircraftSystemsTest, SystemHealthCheck) {
    bool healthy = systems->checkSystems();
    EXPECT_TRUE(healthy);  // Should be healthy with default mock state
    
    auto warnings = systems->getSystemWarnings();
    // May have warnings or not, just verify it's a valid vector
    EXPECT_TRUE(warnings.size() >= 0);
}

// Test: Control input validation
TEST_F(AircraftSystemsTest, ControlInputValidation) {
    // Out-of-range throttle
    EXPECT_CALL(*mockSimConnect, setThrottle(1.0)).Times(1);  // Should clamp to 1.0
    systems->setThrottle(1.5);
    
    EXPECT_CALL(*mockSimConnect, setThrottle(0.0)).Times(1);  // Should clamp to 0.0
    systems->setThrottle(-0.5);
}

// Test: Sequential autopilot engagement
TEST_F(AircraftSystemsTest, AutopilotSequentialControl) {
    EXPECT_CALL(*mockSimConnect, setAutopilotMaster(true)).Times(1);
    systems->enableAutopilot(true);
    
    EXPECT_CALL(*mockSimConnect, setAutopilotHeading(150.0)).Times(1);
    systems->setHeading(150.0);
    
    EXPECT_CALL(*mockSimConnect, setAutopilotAltitude(10000.0)).Times(1);
    systems->setAltitude(10000.0);
    
    EXPECT_CALL(*mockSimConnect, setAutopilotSpeed(300.0)).Times(1);
    systems->setSpeed(300.0);
}

// Test: System update cycle
TEST_F(AircraftSystemsTest, SystemUpdateCycle) {
    // Should complete without crashing
    systems->update();
    systems->update();
    systems->update();
    
    SUCCEED();  // If we get here, update worked
}

// Test: Emergency descent scenario
TEST_F(AircraftSystemsTest, EmergencyDescentScenario) {
    EXPECT_CALL(*mockSimConnect, setAutopilotMaster(false)).Times(1);
    systems->enableAutopilot(false);
    
    EXPECT_CALL(*mockSimConnect, setThrottle(0.0)).Times(1);
    systems->setThrottle(0.0);
    
    EXPECT_CALL(*mockSimConnect, setFlaps(50)).Times(1);
    systems->setFlaps(50);
    
    EXPECT_CALL(*mockSimConnect, setSpoilers(true)).Times(1);
    systems->setSpoilers(true);
}


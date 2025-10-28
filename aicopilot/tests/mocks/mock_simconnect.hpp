#pragma once

#include "../../include/simconnect_wrapper.h"
#include <gtest/gtest.h>
#include <queue>
#include <mutex>

namespace AICopilot {
namespace Testing {

class MockSimConnectWrapper : public SimConnectWrapper {
public:
    MockSimConnectWrapper();
    ~MockSimConnectWrapper() override = default;

    // Connection management
    MOCK_METHOD(bool, connect, (SimulatorType simType, const std::string& appName), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    
    // Process messages
    MOCK_METHOD(void, processMessages, (), (override));
    
    // Aircraft state queries
    MOCK_METHOD(AircraftState, getAircraftState, (), (override));
    MOCK_METHOD(AutopilotState, getAutopilotState, (), (override));
    MOCK_METHOD(Position, getPosition, (), (override));
    
    // Aircraft control
    MOCK_METHOD(void, setAutopilotMaster, (bool enabled), (override));
    MOCK_METHOD(void, setAutopilotHeading, (double heading), (override));
    MOCK_METHOD(void, setAutopilotAltitude, (double altitude), (override));
    MOCK_METHOD(void, setAutopilotSpeed, (double speed), (override));
    MOCK_METHOD(void, setAutopilotVerticalSpeed, (double verticalSpeed), (override));
    MOCK_METHOD(void, setAutopilotNav, (bool enabled), (override));
    MOCK_METHOD(void, setAutopilotApproach, (bool enabled), (override));
    
    // Flight controls
    MOCK_METHOD(void, setThrottle, (double value), (override));
    MOCK_METHOD(void, setElevator, (double value), (override));
    MOCK_METHOD(void, setAileron, (double value), (override));
    MOCK_METHOD(void, setRudder, (double value), (override));
    MOCK_METHOD(void, setFlaps, (int position), (override));
    MOCK_METHOD(void, setGear, (bool down), (override));
    MOCK_METHOD(void, setSpoilers, (bool deployed), (override));
    MOCK_METHOD(void, setParkingBrake, (bool set), (override));
    MOCK_METHOD(void, setBrakes, (double value), (override));
    
    // Engine controls
    MOCK_METHOD(void, setMixture, (double value), (override));
    MOCK_METHOD(void, setPropellerPitch, (double value), (override));
    MOCK_METHOD(void, setMagnetos, (int position), (override));
    MOCK_METHOD(void, toggleEngineStarter, (int engineIndex), (override));
    MOCK_METHOD(void, setEngineState, (int engineIndex, bool running), (override));
    
    // Lighting
    MOCK_METHOD(void, setLight, (const std::string& lightName, bool on), (override));
    
    // ATC interaction
    MOCK_METHOD(void, sendATCMenuSelection, (int menuIndex), (override));
    MOCK_METHOD(void, requestATCMenu, (), (override));
    MOCK_METHOD(std::vector<std::string>, getATCMenuOptions, (), (override));
    
    // Subscriptions
    MOCK_METHOD(void, subscribeToAircraftState, (StateCallback callback), (override));
    MOCK_METHOD(void, subscribeToATCMessages, (ATCCallback callback), (override));

    // Test helpers
    void setMockAircraftState(const AircraftState& state);
    void setMockPosition(const Position& pos);
    void emitAircraftState(const AircraftState& state);
    void emitATCMessage(const ATCMessage& msg);
    void setConnected(bool connected);

private:
    AircraftState mockAircraftState_;
    Position mockPosition_;
    bool isConnected_;
    StateCallback stateCallback_;
    ATCCallback atcCallback_;
};

} // namespace Testing
} // namespace AICopilot

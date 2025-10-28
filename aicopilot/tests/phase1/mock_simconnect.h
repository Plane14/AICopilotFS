/*****************************************************************************
* Copyright 2025 AI Copilot FS Project - Phase 1 Test Infrastructure
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef MOCK_SIMCONNECT_H
#define MOCK_SIMCONNECT_H

#include "../include/aicopilot_types.h"
#include "../include/simconnect_wrapper.h"
#include <gmock/gmock.h>
#include <memory>
#include <vector>

namespace AICopilot {
namespace Testing {

/**
 * Mock SimConnect implementation for unit testing
 * Provides isolated testing without requiring SimConnect or simulator connection
 */
class MockSimConnect {
public:
    MockSimConnect() = default;
    ~MockSimConnect() = default;

    // Connection state
    bool connected_ = false;
    SimulatorType simulatorType_ = SimulatorType::MSFS2024;

    // Aircraft state storage
    AircraftState currentState_;
    AutopilotState autopilotState_;
    Position currentPosition_;

    // Control values storage
    double throttleValue_ = 0.0;
    double elevatorValue_ = 0.0;
    double aileronValue_ = 0.0;
    double rudderValue_ = 0.0;
    int flapsPosition_ = 0;
    bool gearDown_ = false;
    bool spoilersDeployed_ = false;
    bool parkingBrakeSet_ = false;
    double brakeValue_ = 0.0;

    // Engine controls
    double mixtureValue_ = 0.0;
    double propPitchValue_ = 0.0;
    int magnetos_ = 0;
    std::vector<bool> engineRunning_;

    // Lighting
    std::map<std::string, bool> lights_;

    // ATC data
    std::vector<std::string> atcMenuOptions_;
    std::vector<ATCMessage> atcMessages_;

    // Mock methods
    MOCK_METHOD(bool, connect, (SimulatorType, const std::string&));
    MOCK_METHOD(void, disconnect, ());
    MOCK_METHOD(bool, isConnected, (), (const));
    MOCK_METHOD(void, processMessages, ());

    // State queries
    MOCK_METHOD(AircraftState, getAircraftState, ());
    MOCK_METHOD(AutopilotState, getAutopilotState, ());
    MOCK_METHOD(Position, getPosition, ());

    // Autopilot controls
    MOCK_METHOD(void, setAutopilotMaster, (bool));
    MOCK_METHOD(void, setAutopilotHeading, (double));
    MOCK_METHOD(void, setAutopilotAltitude, (double));
    MOCK_METHOD(void, setAutopilotSpeed, (double));
    MOCK_METHOD(void, setAutopilotVerticalSpeed, (double));
    MOCK_METHOD(void, setAutopilotNav, (bool));
    MOCK_METHOD(void, setAutopilotApproach, (bool));

    // Flight controls
    MOCK_METHOD(void, setThrottle, (double));
    MOCK_METHOD(void, setElevator, (double));
    MOCK_METHOD(void, setAileron, (double));
    MOCK_METHOD(void, setRudder, (double));
    MOCK_METHOD(void, setFlaps, (int));
    MOCK_METHOD(void, setGear, (bool));
    MOCK_METHOD(void, setSpoilers, (bool));
    MOCK_METHOD(void, setParkingBrake, (bool));
    MOCK_METHOD(void, setBrakes, (double));

    // Engine controls
    MOCK_METHOD(void, setMixture, (double));
    MOCK_METHOD(void, setPropellerPitch, (double));
    MOCK_METHOD(void, setMagnetos, (int));
    MOCK_METHOD(void, toggleEngineStarter, (int));
    MOCK_METHOD(void, setEngineState, (int, bool));

    // Lighting
    MOCK_METHOD(void, setLight, (const std::string&, bool));

    // ATC
    MOCK_METHOD(void, sendATCMenuSelection, (int));
    MOCK_METHOD(void, requestATCMenu, ());
    MOCK_METHOD(std::vector<std::string>, getATCMenuOptions, ());

    // Helper methods for tests
    void setMockAircraftState(const AircraftState& state) {
        currentState_ = state;
    }

    void setMockAutopilotState(const AutopilotState& state) {
        autopilotState_ = state;
    }

    void setMockPosition(const Position& pos) {
        currentPosition_ = pos;
    }

    void setConnected(bool connected) {
        connected_ = connected;
    }

    void recordThrottleValue(double value) {
        throttleValue_ = value;
    }

    void recordFlapsPosition(int position) {
        flapsPosition_ = position;
    }

    void recordGearState(bool down) {
        gearDown_ = down;
    }

    double getRecordedThrottle() const { return throttleValue_; }
    int getRecordedFlapsPosition() const { return flapsPosition_; }
    bool getRecordedGearState() const { return gearDown_; }
};

/**
 * Factory for creating test fixtures with mock SimConnect
 */
class MockSimConnectFactory {
public:
    static std::shared_ptr<MockSimConnect> createMock() {
        return std::make_shared<MockSimConnect>();
    }

    static MockSimConnect* createRawMock() {
        return new MockSimConnect();
    }

    // Helper to create a default aircraft state for testing
    static AircraftState createDefaultAircraftState() {
        AircraftState state{};
        state.position.latitude = 37.7749;
        state.position.longitude = -122.4194;
        state.position.altitude = 500.0;
        state.position.heading = 0.0;
        state.indicatedAirspeed = 0.0;
        state.trueAirspeed = 0.0;
        state.groundSpeed = 0.0;
        state.verticalSpeed = 0.0;
        state.pitch = 0.0;
        state.bank = 0.0;
        state.heading = 0.0;
        state.altimeter = 29.92;
        state.onGround = true;
        state.fuelQuantity = 50.0;
        state.engineRPM = 0.0;
        state.parkingBrakeSet = true;
        state.gearDown = true;
        state.flapsPosition = 0;
        state.masterBattery = true;
        state.masterAlternator = true;
        state.batteryVoltage = 14.0;
        state.batteryLoad = 0.0;
        state.generatorVoltage = 14.0;
        state.generatorLoad = 0.0;
        return state;
    }

    static AutopilotState createDefaultAutopilotState() {
        AutopilotState state{};
        state.masterEnabled = false;
        state.headingHold = false;
        state.altitudeHold = false;
        state.speedHold = false;
        state.verticalSpeedHold = false;
        state.navMode = false;
        state.approachMode = false;
        state.targetHeading = 0.0;
        state.targetAltitude = 0.0;
        state.targetSpeed = 0.0;
        state.targetVerticalSpeed = 0.0;
        return state;
    }

    static Position createPosition(double lat, double lon, double alt, double hdg = 0.0) {
        return {lat, lon, alt, hdg};
    }
};

} // namespace Testing
} // namespace AICopilot

#endif // MOCK_SIMCONNECT_H

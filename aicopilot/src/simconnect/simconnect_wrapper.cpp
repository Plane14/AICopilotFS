/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "../include/simconnect_wrapper.h"
#include <iostream>

// NOTE: This is a stub implementation. Full SimConnect integration requires:
// - SimConnect.h header from MSFS2024/Prepar3D SDK
// - Linking against SimConnect.lib
// - Implementation of all SimConnect callbacks and data structures

namespace AICopilot {

class SimConnectWrapper::Impl {
public:
    bool connected = false;
    SimulatorType simType = SimulatorType::UNKNOWN;
    AircraftState currentState;
    AutopilotState autopilotState;
    StateCallback stateCallback;
    ATCCallback atcCallback;
    
    // SimConnect handle would go here
    // HANDLE hSimConnect = nullptr;
};

SimConnectWrapper::SimConnectWrapper() : pImpl(std::make_unique<Impl>()) {}

SimConnectWrapper::~SimConnectWrapper() {
    disconnect();
}

bool SimConnectWrapper::connect(SimulatorType simType, const std::string& appName) {
    std::cout << "Connecting to simulator: " << appName << std::endl;
    
    // TODO: Implement actual SimConnect connection
    // HRESULT hr = SimConnect_Open(&pImpl->hSimConnect, appName.c_str(), nullptr, 0, 0, 0);
    
    pImpl->simType = simType;
    pImpl->connected = true; // Stub - would check actual connection
    
    return pImpl->connected;
}

void SimConnectWrapper::disconnect() {
    if (pImpl->connected) {
        // TODO: SimConnect_Close(pImpl->hSimConnect);
        pImpl->connected = false;
        std::cout << "Disconnected from simulator" << std::endl;
    }
}

bool SimConnectWrapper::isConnected() const {
    return pImpl->connected;
}

void SimConnectWrapper::processMessages() {
    if (!pImpl->connected) return;
    
    // TODO: Process SimConnect messages
    // SimConnect_CallDispatch(pImpl->hSimConnect, DispatchProc, this);
}

AircraftState SimConnectWrapper::getAircraftState() {
    // TODO: Request and return actual aircraft state from SimConnect
    return pImpl->currentState;
}

AutopilotState SimConnectWrapper::getAutopilotState() {
    // TODO: Request and return actual autopilot state from SimConnect
    return pImpl->autopilotState;
}

Position SimConnectWrapper::getPosition() {
    return pImpl->currentState.position;
}

void SimConnectWrapper::setAutopilotMaster(bool enabled) {
    std::cout << "Setting autopilot master: " << (enabled ? "ON" : "OFF") << std::endl;
    // TODO: SimConnect_TransmitClientEvent for AUTOPILOT_MASTER
}

void SimConnectWrapper::setAutopilotHeading(double heading) {
    std::cout << "Setting autopilot heading: " << heading << std::endl;
    // TODO: Set AP_HDG_HOLD and AP_HEADING_BUG
}

void SimConnectWrapper::setAutopilotAltitude(double altitude) {
    std::cout << "Setting autopilot altitude: " << altitude << std::endl;
    // TODO: Set AP_ALT_HOLD and AP_ALT_VAR
}

void SimConnectWrapper::setAutopilotSpeed(double speed) {
    std::cout << "Setting autopilot speed: " << speed << std::endl;
    // TODO: Set AP_AIRSPEED_HOLD and AP_AIRSPEED_SET
}

void SimConnectWrapper::setAutopilotVerticalSpeed(double verticalSpeed) {
    std::cout << "Setting autopilot vertical speed: " << verticalSpeed << " fpm" << std::endl;
    // TODO: Set AP_VS_HOLD and AP_VS_VAR_SET
}

void SimConnectWrapper::setAutopilotNav(bool enabled) {
    std::cout << "Setting autopilot NAV: " << (enabled ? "ON" : "OFF") << std::endl;
    // TODO: Set AP_NAV1_HOLD
}

void SimConnectWrapper::setAutopilotApproach(bool enabled) {
    std::cout << "Setting autopilot approach: " << (enabled ? "ON" : "OFF") << std::endl;
    // TODO: Set AP_APR_HOLD
}

void SimConnectWrapper::setThrottle(double value) {
    std::cout << "Setting throttle: " << (value * 100.0) << "%" << std::endl;
    // TODO: Set THROTTLE_SET event
}

void SimConnectWrapper::setElevator(double value) {
    // TODO: Set ELEVATOR_SET event
}

void SimConnectWrapper::setAileron(double value) {
    // TODO: Set AILERON_SET event
}

void SimConnectWrapper::setRudder(double value) {
    // TODO: Set RUDDER_SET event
}

void SimConnectWrapper::setFlaps(int position) {
    std::cout << "Setting flaps: " << position << "%" << std::endl;
    // TODO: Set FLAPS_SET event
}

void SimConnectWrapper::setGear(bool down) {
    std::cout << "Setting gear: " << (down ? "DOWN" : "UP") << std::endl;
    // TODO: Set GEAR_UP or GEAR_DOWN event
}

void SimConnectWrapper::setSpoilers(bool deployed) {
    std::cout << "Setting spoilers: " << (deployed ? "DEPLOYED" : "RETRACTED") << std::endl;
    // TODO: Set SPOILERS_SET event
}

void SimConnectWrapper::setParkingBrake(bool set) {
    std::cout << "Setting parking brake: " << (set ? "ON" : "OFF") << std::endl;
    // TODO: Set PARKING_BRAKES event
}

void SimConnectWrapper::setBrakes(double value) {
    std::cout << "Setting brakes: " << (value * 100.0) << "%" << std::endl;
    // TODO: Set AXIS_BRAKE_SET or BRAKES event
}

void SimConnectWrapper::setMixture(double value) {
    std::cout << "Setting mixture: " << (value * 100.0) << "%" << std::endl;
    // TODO: Set MIXTURE_SET event
}

void SimConnectWrapper::setPropellerPitch(double value) {
    std::cout << "Setting propeller pitch: " << (value * 100.0) << "%" << std::endl;
    // TODO: Set PROP_PITCH_SET event
}

void SimConnectWrapper::setMagnetos(int position) {
    std::cout << "Setting magnetos: " << position << std::endl;
    // TODO: Set MAGNETO_SET event
}

void SimConnectWrapper::toggleEngineStarter(int engineIndex) {
    std::cout << "Toggling starter for engine " << engineIndex << std::endl;
    // TODO: Set TOGGLE_STARTER event
}

void SimConnectWrapper::setEngineState(int engineIndex, bool running) {
    std::cout << "Setting engine " << engineIndex << " state: " << (running ? "RUNNING" : "OFF") << std::endl;
    // TODO: Control engine state
}

void SimConnectWrapper::setLight(const std::string& lightName, bool on) {
    std::cout << "Setting " << lightName << " light: " << (on ? "ON" : "OFF") << std::endl;
    // TODO: Set appropriate light event (e.g., TOGGLE_NAV_LIGHTS, LANDING_LIGHTS_ON, etc.)
}

void SimConnectWrapper::sendATCMenuSelection(int menuIndex) {
    std::cout << "Selecting ATC menu option: " << menuIndex << std::endl;
    // TODO: Send ATC_MENU_[0-9] event
}

void SimConnectWrapper::requestATCMenu() {
    std::cout << "Requesting ATC menu" << std::endl;
    // TODO: Send ATC_MENU_OPEN event
}

std::vector<std::string> SimConnectWrapper::getATCMenuOptions() {
    // TODO: Read ATC menu state (this is complex and may require memory reading)
    return {};
}

void SimConnectWrapper::subscribeToAircraftState(StateCallback callback) {
    pImpl->stateCallback = callback;
    // TODO: Request periodic aircraft state data
}

void SimConnectWrapper::subscribeToATCMessages(ATCCallback callback) {
    pImpl->atcCallback = callback;
    // TODO: Subscribe to ATC message events
}

} // namespace AICopilot

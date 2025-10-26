/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef SIMCONNECT_WRAPPER_H
#define SIMCONNECT_WRAPPER_H

#include "aicopilot_types.h"
#include <functional>
#include <memory>
#include <string>

namespace AICopilot {

/**
 * Wrapper class for SimConnect API to interface with MSFS2024 and Prepar3D V6
 * Provides abstraction layer for simulator communication
 */
class SimConnectWrapper {
public:
    SimConnectWrapper();
    ~SimConnectWrapper();

    // Connection management
    bool connect(SimulatorType simType, const std::string& appName = "AICopilot");
    void disconnect();
    bool isConnected() const;
    
    // Process SimConnect messages
    void processMessages();
    
    // Aircraft state queries
    AircraftState getAircraftState();
    AutopilotState getAutopilotState();
    Position getPosition();
    
    // Aircraft control
    void setAutopilotMaster(bool enabled);
    void setAutopilotHeading(double heading);
    void setAutopilotAltitude(double altitude);
    void setAutopilotSpeed(double speed);
    void setAutopilotNav(bool enabled);
    void setAutopilotApproach(bool enabled);
    
    // Flight controls
    void setThrottle(double value);      // 0.0 to 1.0
    void setElevator(double value);      // -1.0 to 1.0
    void setAileron(double value);       // -1.0 to 1.0
    void setRudder(double value);        // -1.0 to 1.0
    void setFlaps(int position);         // 0 to 100%
    void setGear(bool down);
    void setParkingBrake(bool set);
    
    // Engine controls
    void setMixture(double value);       // 0.0 to 1.0
    void setPropellerPitch(double value); // 0.0 to 1.0
    void setMagnetos(int position);      // 0=off, 1=right, 2=left, 3=both
    void toggleEngineStarter(int engineIndex);
    
    // ATC interaction
    void sendATCMenuSelection(int menuIndex);
    void requestATCMenu();
    std::vector<std::string> getATCMenuOptions();
    
    // Data subscription callbacks
    using StateCallback = std::function<void(const AircraftState&)>;
    using ATCCallback = std::function<void(const ATCMessage&)>;
    
    void subscribeToAircraftState(StateCallback callback);
    void subscribeToATCMessages(ATCCallback callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace AICopilot

#endif // SIMCONNECT_WRAPPER_H

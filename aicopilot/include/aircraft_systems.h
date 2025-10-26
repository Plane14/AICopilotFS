/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef AIRCRAFT_SYSTEMS_H
#define AIRCRAFT_SYSTEMS_H

#include "aicopilot_types.h"
#include "simconnect_wrapper.h"
#include "aircraft_config.h"
#include <memory>

namespace AICopilot {

/**
 * Controller for all aircraft systems
 * Manages autopilot, flight controls, engines, and other systems
 */
class AircraftSystems {
public:
    AircraftSystems(std::shared_ptr<SimConnectWrapper> simConnect,
                   const AircraftConfig& config);
    ~AircraftSystems();
    
    // Update system state (call regularly)
    void update();
    
    // Autopilot control
    void enableAutopilot(bool enable);
    void setHeading(double heading);
    void setAltitude(double altitude);
    void setSpeed(double speed);
    void setVerticalSpeed(double verticalSpeed);
    void enableNavMode(bool enable);
    void enableApproachMode(bool enable);
    
    // Manual flight controls
    void setPitch(double pitch);       // -1.0 to 1.0
    void setRoll(double roll);         // -1.0 to 1.0
    void setYaw(double yaw);           // -1.0 to 1.0
    
    // Power management
    void setThrottle(double throttle); // 0.0 to 1.0
    void setMixture(double mixture);   // 0.0 to 1.0
    void setPropeller(double prop);    // 0.0 to 1.0
    
    // Flight surfaces
    void setFlaps(int position);       // 0 to 100%
    void setGear(bool down);
    void setSpoilers(bool deployed);
    
    // Ground operations
    void setParkingBrake(bool set);
    void setBrakes(double value);      // 0.0 to 1.0
    
    // Engine management
    void startEngine(int engineIndex);
    void stopEngine(int engineIndex);
    void setMagnetos(int position);
    
    // Lighting
    void setNavigationLights(bool on);
    void setBeaconLights(bool on);
    void setStrobeLights(bool on);
    void setLandingLights(bool on);
    void setTaxiLights(bool on);
    
    // Current state
    AircraftState getCurrentState() const;
    AutopilotState getAutopilotState() const;
    
    // Check system health
    bool checkSystems();
    std::vector<std::string> getSystemWarnings();
    
private:
    std::shared_ptr<SimConnectWrapper> simConnect_;
    AircraftConfig config_;
    AircraftState currentState_;
    AutopilotState autopilotState_;
    std::vector<std::string> warnings_;
    
    void updateState();
    void checkFuel();
    void checkEngines();
    void checkElectrical();
};

} // namespace AICopilot

#endif // AIRCRAFT_SYSTEMS_H

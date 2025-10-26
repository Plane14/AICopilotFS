/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef AI_PILOT_H
#define AI_PILOT_H

#include "aicopilot_types.h"
#include "simconnect_wrapper.h"
#include "aircraft_systems.h"
#include "atc_controller.h"
#include "navigation.h"
#include "aircraft_config.h"
#include <memory>
#include <string>

namespace AICopilot {

/**
 * Autonomous AI Pilot
 * Main controller that coordinates all systems for autonomous flight
 */
class AIPilot {
public:
    AIPilot();
    ~AIPilot();
    
    // Initialize the AI pilot
    bool initialize(SimulatorType simType);
    
    // Load aircraft configuration
    bool loadAircraftConfig(const std::string& configPath);
    
    // Load flight plan
    bool loadFlightPlan(const std::string& planPath);
    
    // Start autonomous flight
    void startAutonomousFlight();
    
    // Stop autonomous flight
    void stopAutonomousFlight();
    
    // Check if pilot is active
    bool isActive() const { return active_; }
    
    // Main update loop (call regularly, e.g., 10-20 Hz)
    void update();
    
    // Get current status
    std::string getStatusReport() const;
    
    // Get current flight phase
    FlightPhase getCurrentPhase() const { return currentPhase_; }
    
    // Emergency handling
    void handleEmergency(const std::string& emergencyType);
    
    // Manual override (temporarily disable AI)
    void setManualOverride(bool override);
    
private:
    // Core components
    std::shared_ptr<SimConnectWrapper> simConnect_;
    std::unique_ptr<AircraftSystems> systems_;
    std::unique_ptr<ATCController> atc_;
    std::unique_ptr<Navigation> navigation_;
    AircraftConfig aircraftConfig_;
    
    // State
    bool active_;
    bool manualOverride_;
    FlightPhase currentPhase_;
    AircraftState currentState_;
    
    // Phase management
    void updateFlightPhase();
    void executePhase();
    
    // Phase-specific logic
    void executePreflight();
    void executeTaxiOut();
    void executeTakeoff();
    void executeClimb();
    void executeCruise();
    void executeDescent();
    void executeApproach();
    void executeLanding();
    void executeTaxiIn();
    void executeShutdown();
    
    // Decision making
    bool shouldStartTakeoff();
    bool shouldStartClimb();
    bool shouldStartDescent();
    bool shouldStartApproach();
    
    // Flight control logic
    void controlHeading();
    void controlAltitude();
    void controlSpeed();
    
    // Safety checks
    bool performSafetyChecks();
    void handleLowFuel();
    void handleBadWeather();
    void handleEngineFailure();
    void handleFire();
    void handleLossOfControl();
    
    // Terrain awareness
    bool checkTerrainClearance();
    double getTerrainElevation(const Position& pos);
    
    // Weather assessment
    WeatherConditions assessWeather();
    bool isWeatherSuitable();
    
    // Logging
    void log(const std::string& message);
};

} // namespace AICopilot

#endif // AI_PILOT_H

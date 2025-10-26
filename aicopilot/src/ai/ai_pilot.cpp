/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "../include/ai_pilot.h"
#include <iostream>
#include <sstream>
#include <cmath>

namespace AICopilot {

AIPilot::AIPilot() 
    : active_(false)
    , manualOverride_(false)
    , currentPhase_(FlightPhase::UNKNOWN) {
}

AIPilot::~AIPilot() {
    stopAutonomousFlight();
}

bool AIPilot::initialize(SimulatorType simType) {
    log("Initializing AI Pilot");
    
    simConnect_ = std::make_shared<SimConnectWrapper>();
    if (!simConnect_->connect(simType, "AI Copilot FS")) {
        log("ERROR: Failed to connect to simulator");
        return false;
    }
    
    log("Connected to simulator");
    return true;
}

bool AIPilot::loadAircraftConfig(const std::string& configPath) {
    log("Loading aircraft configuration: " + configPath);
    
    AircraftConfigParser parser;
    if (!parser.parse(configPath)) {
        log("ERROR: Failed to parse aircraft configuration");
        return false;
    }
    
    aircraftConfig_ = parser.getConfig();
    
    // Initialize systems with loaded configuration
    systems_ = std::make_unique<AircraftSystems>(simConnect_, aircraftConfig_);
    
    log("Aircraft configuration loaded: " + aircraftConfig_.title);
    return true;
}

bool AIPilot::loadFlightPlan(const std::string& planPath) {
    log("Loading flight plan: " + planPath);
    
    navigation_ = std::make_unique<Navigation>();
    if (!navigation_->loadFlightPlan(planPath)) {
        log("ERROR: Failed to load flight plan");
        return false;
    }
    
    log("Flight plan loaded");
    return true;
}

void AIPilot::startAutonomousFlight() {
    if (!simConnect_ || !simConnect_->isConnected()) {
        log("ERROR: Not connected to simulator");
        return;
    }
    
    if (!systems_) {
        log("ERROR: Aircraft systems not initialized");
        return;
    }
    
    log("Starting autonomous flight");
    active_ = true;
    currentPhase_ = FlightPhase::PREFLIGHT;
    
    // Initialize ATC controller
    atc_ = std::make_unique<ATCController>(simConnect_);
    if (navigation_) {
        atc_->setFlightPlan(navigation_->getFlightPlan());
    }
}

void AIPilot::stopAutonomousFlight() {
    if (active_) {
        log("Stopping autonomous flight");
        active_ = false;
    }
}

void AIPilot::update() {
    if (!active_ || manualOverride_) {
        return;
    }
    
    // Process SimConnect messages
    simConnect_->processMessages();
    
    // Update current state
    currentState_ = systems_->getCurrentState();
    
    // Update subsystems
    systems_->update();
    if (atc_) {
        atc_->update();
    }
    
    // Perform safety checks
    if (!performSafetyChecks()) {
        log("WARNING: Safety check failed");
        return;
    }
    
    // Update flight phase
    updateFlightPhase();
    
    // Execute phase-specific logic
    executePhase();
}

std::string AIPilot::getStatusReport() const {
    std::ostringstream oss;
    oss << "AI Pilot Status\n";
    oss << "===============\n";
    oss << "Active: " << (active_ ? "YES" : "NO") << "\n";
    oss << "Phase: ";
    
    switch (currentPhase_) {
        case FlightPhase::PREFLIGHT: oss << "PREFLIGHT"; break;
        case FlightPhase::TAXI_OUT: oss << "TAXI OUT"; break;
        case FlightPhase::TAKEOFF: oss << "TAKEOFF"; break;
        case FlightPhase::CLIMB: oss << "CLIMB"; break;
        case FlightPhase::CRUISE: oss << "CRUISE"; break;
        case FlightPhase::DESCENT: oss << "DESCENT"; break;
        case FlightPhase::APPROACH: oss << "APPROACH"; break;
        case FlightPhase::LANDING: oss << "LANDING"; break;
        case FlightPhase::TAXI_IN: oss << "TAXI IN"; break;
        case FlightPhase::SHUTDOWN: oss << "SHUTDOWN"; break;
        default: oss << "UNKNOWN"; break;
    }
    
    oss << "\n";
    oss << "Altitude: " << currentState_.position.altitude << " ft\n";
    oss << "Airspeed: " << currentState_.indicatedAirspeed << " kts\n";
    oss << "Heading: " << currentState_.heading << " deg\n";
    
    return oss.str();
}

void AIPilot::handleEmergency(const std::string& emergencyType) {
    log("EMERGENCY: " + emergencyType);
    
    // TODO: Implement emergency procedures
    // - Engine failure: find nearest airport, glide, emergency landing
    // - Fire: follow checklist
    // - Loss of control: recover from unusual attitudes
}

void AIPilot::setManualOverride(bool override) {
    manualOverride_ = override;
    log(override ? "Manual override ENABLED" : "Manual override DISABLED");
}

void AIPilot::updateFlightPhase() {
    // Determine flight phase based on current state
    
    if (currentState_.onGround) {
        if (currentState_.groundSpeed < 5.0) {
            if (currentPhase_ == FlightPhase::LANDING || currentPhase_ == FlightPhase::TAXI_IN) {
                currentPhase_ = FlightPhase::TAXI_IN;
            } else if (currentPhase_ == FlightPhase::SHUTDOWN) {
                // Stay in shutdown
            } else {
                currentPhase_ = FlightPhase::PREFLIGHT;
            }
        } else if (currentState_.groundSpeed < 40.0) {
            currentPhase_ = FlightPhase::TAXI_OUT;
        } else {
            currentPhase_ = FlightPhase::TAKEOFF;
        }
    } else {
        // In the air
        double targetAltitude = navigation_ ? navigation_->getFlightPlan().cruiseAltitude : 10000.0;
        
        if (currentState_.position.altitude < 1000.0) {
            currentPhase_ = FlightPhase::TAKEOFF;
        } else if (currentState_.verticalSpeed > 300.0 && 
                   currentState_.position.altitude < targetAltitude - 1000.0) {
            currentPhase_ = FlightPhase::CLIMB;
        } else if (std::abs(currentState_.verticalSpeed) < 300.0) {
            currentPhase_ = FlightPhase::CRUISE;
        } else if (currentState_.verticalSpeed < -300.0) {
            if (currentState_.position.altitude < 5000.0) {
                currentPhase_ = FlightPhase::APPROACH;
            } else {
                currentPhase_ = FlightPhase::DESCENT;
            }
        }
    }
    
    // Update ATC controller with current phase
    if (atc_) {
        atc_->setFlightPhase(currentPhase_);
    }
}

void AIPilot::executePhase() {
    switch (currentPhase_) {
        case FlightPhase::PREFLIGHT:
            executePreflight();
            break;
        case FlightPhase::TAXI_OUT:
            executeTaxiOut();
            break;
        case FlightPhase::TAKEOFF:
            executeTakeoff();
            break;
        case FlightPhase::CLIMB:
            executeClimb();
            break;
        case FlightPhase::CRUISE:
            executeCruise();
            break;
        case FlightPhase::DESCENT:
            executeDescent();
            break;
        case FlightPhase::APPROACH:
            executeApproach();
            break;
        case FlightPhase::LANDING:
            executeLanding();
            break;
        case FlightPhase::TAXI_IN:
            executeTaxiIn();
            break;
        case FlightPhase::SHUTDOWN:
            executeShutdown();
            break;
        default:
            break;
    }
}

void AIPilot::executePreflight() {
    // Preflight checks
    static bool checksComplete = false;
    
    if (!checksComplete) {
        log("Performing preflight checks");
        
        // Set parking brake
        systems_->setParkingBrake(true);
        
        // Set initial systems
        systems_->setNavigationLights(true);
        systems_->setBeaconLights(true);
        
        checksComplete = true;
    }
}

void AIPilot::executeTaxiOut() {
    log("Taxiing to runway");
    
    // Enable taxi lights
    systems_->setTaxiLights(true);
    
    // TODO: Follow taxi route
}

void AIPilot::executeTakeoff() {
    log("Taking off");
    
    // Enable landing lights
    systems_->setLandingLights(true);
    systems_->setStrobeLights(true);
    
    if (currentState_.onGround) {
        // Apply takeoff power
        systems_->setThrottle(1.0);
        
        // Release parking brake
        systems_->setParkingBrake(false);
        
        // Rotate at appropriate speed
        if (currentState_.indicatedAirspeed > aircraftConfig_.stallSpeed * 1.3) {
            systems_->setPitch(0.15); // Gentle pitch up
        }
    } else {
        // After liftoff, retract gear
        if (currentState_.position.altitude > 50.0 && currentState_.verticalSpeed > 0) {
            systems_->setGear(false);
            systems_->setFlaps(0);
        }
    }
}

void AIPilot::executeClimb() {
    log("Climbing");
    
    // Enable autopilot
    systems_->enableAutopilot(true);
    
    // Set climb parameters
    double targetAltitude = navigation_ ? navigation_->getFlightPlan().cruiseAltitude : 10000.0;
    systems_->setAltitude(targetAltitude);
    
    // Set climb speed
    double climbSpeed = aircraftConfig_.cruiseSpeed * 0.75;
    systems_->setSpeed(climbSpeed);
    
    controlHeading();
}

void AIPilot::executeCruise() {
    log("Cruising");
    
    // Enable autopilot
    systems_->enableAutopilot(true);
    
    // Set cruise parameters
    double cruiseAltitude = navigation_ ? navigation_->getFlightPlan().cruiseAltitude : 10000.0;
    systems_->setAltitude(cruiseAltitude);
    systems_->setSpeed(aircraftConfig_.cruiseSpeed);
    
    // Follow navigation
    controlHeading();
    
    // Check if we need to start descent
    if (navigation_ && shouldStartDescent()) {
        currentPhase_ = FlightPhase::DESCENT;
    }
}

void AIPilot::executeDescent() {
    log("Descending");
    
    // Continue with autopilot
    systems_->enableAutopilot(true);
    
    // Set descent parameters
    systems_->setAltitude(3000.0); // Initial descent altitude
    
    controlHeading();
}

void AIPilot::executeApproach() {
    log("On approach");
    
    // Enable approach mode
    systems_->enableApproachMode(true);
    
    // Configure for landing
    if (currentState_.position.altitude < 3000.0) {
        systems_->setFlaps(50);
        systems_->setGear(true);
        systems_->setLandingLights(true);
    }
}

void AIPilot::executeLanding() {
    log("Landing");
    
    // Final approach configuration
    systems_->setFlaps(100);
    systems_->setGear(true);
    
    // TODO: Flare and touchdown logic
}

void AIPilot::executeTaxiIn() {
    log("Taxiing to gate");
    
    // Reduce power
    systems_->setThrottle(0.1);
    
    // Enable taxi lights
    systems_->setTaxiLights(true);
    systems_->setLandingLights(false);
    systems_->setStrobeLights(false);
}

void AIPilot::executeShutdown() {
    log("Shutting down");
    
    // Set parking brake
    systems_->setParkingBrake(true);
    
    // Reduce throttle
    systems_->setThrottle(0.0);
    
    // TODO: Shutdown checklist
}

bool AIPilot::shouldStartTakeoff() {
    // Check if we have clearance and conditions are good
    return currentState_.onGround && currentState_.parkingBrakeSet == false;
}

bool AIPilot::shouldStartClimb() {
    return !currentState_.onGround && currentState_.position.altitude > 100.0;
}

bool AIPilot::shouldStartDescent() {
    if (!navigation_) return false;
    
    // Calculate distance to destination
    Waypoint dest = navigation_->getNextWaypoint();
    double distance = navigation_->distanceToWaypoint(currentState_.position, dest);
    
    // Start descent 30nm out (rough calculation)
    return distance < 30.0;
}

bool AIPilot::shouldStartApproach() {
    return currentState_.position.altitude < 5000.0;
}

void AIPilot::controlHeading() {
    if (!navigation_) return;
    
    Waypoint active = navigation_->getActiveWaypoint();
    double targetHeading = navigation_->bearingToWaypoint(currentState_.position, active);
    
    systems_->setHeading(targetHeading);
    
    // Check if waypoint reached
    if (navigation_->isWaypointReached(currentState_.position)) {
        navigation_->advanceWaypoint();
        log("Waypoint reached, advancing");
    }
}

void AIPilot::controlAltitude() {
    // Handled by autopilot
}

void AIPilot::controlSpeed() {
    // Handled by autopilot
}

bool AIPilot::performSafetyChecks() {
    bool safe = systems_->checkSystems();
    
    if (!safe) {
        auto warnings = systems_->getSystemWarnings();
        for (const auto& warning : warnings) {
            log(warning);
        }
    }
    
    // Check fuel
    if (currentState_.fuelQuantity < 5.0) {
        handleLowFuel();
        return false;
    }
    
    return true;
}

void AIPilot::handleLowFuel() {
    log("WARNING: Low fuel - finding nearest airport");
    // TODO: Find nearest airport and divert
}

void AIPilot::handleBadWeather() {
    log("WARNING: Bad weather detected");
    // TODO: Weather avoidance or diversion
}

void AIPilot::log(const std::string& message) {
    std::cout << "[AI Pilot] " << message << std::endl;
}

} // namespace AICopilot

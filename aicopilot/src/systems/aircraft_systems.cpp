/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "../include/aircraft_systems.h"
#include <iostream>

namespace AICopilot {

AircraftSystems::AircraftSystems(std::shared_ptr<SimConnectWrapper> simConnect,
                               const AircraftConfig& config)
    : simConnect_(simConnect)
    , config_(config) {
    
    // Initialize states
    currentState_ = simConnect_->getAircraftState();
    autopilotState_ = simConnect_->getAutopilotState();
}

AircraftSystems::~AircraftSystems() = default;

void AircraftSystems::update() {
    updateState();
    checkSystems();
}

void AircraftSystems::enableAutopilot(bool enable) {
    simConnect_->setAutopilotMaster(enable);
    autopilotState_.masterEnabled = enable;
}

void AircraftSystems::setHeading(double heading) {
    simConnect_->setAutopilotHeading(heading);
    autopilotState_.targetHeading = heading;
    autopilotState_.headingHold = true;
}

void AircraftSystems::setAltitude(double altitude) {
    simConnect_->setAutopilotAltitude(altitude);
    autopilotState_.targetAltitude = altitude;
    autopilotState_.altitudeHold = true;
}

void AircraftSystems::setSpeed(double speed) {
    simConnect_->setAutopilotSpeed(speed);
    autopilotState_.targetSpeed = speed;
    autopilotState_.speedHold = true;
}

void AircraftSystems::setVerticalSpeed(double verticalSpeed) {
    std::cout << "Setting vertical speed: " << verticalSpeed << " fpm" << std::endl;
    simConnect_->setAutopilotVerticalSpeed(verticalSpeed);
}

void AircraftSystems::enableNavMode(bool enable) {
    simConnect_->setAutopilotNav(enable);
    autopilotState_.navMode = enable;
}

void AircraftSystems::enableApproachMode(bool enable) {
    simConnect_->setAutopilotApproach(enable);
    autopilotState_.approachMode = enable;
}

void AircraftSystems::setPitch(double pitch) {
    simConnect_->setElevator(pitch);
}

void AircraftSystems::setRoll(double roll) {
    simConnect_->setAileron(roll);
}

void AircraftSystems::setYaw(double yaw) {
    simConnect_->setRudder(yaw);
}

void AircraftSystems::setThrottle(double throttle) {
    simConnect_->setThrottle(throttle);
}

void AircraftSystems::setMixture(double mixture) {
    simConnect_->setMixture(mixture);
}

void AircraftSystems::setPropeller(double prop) {
    simConnect_->setPropellerPitch(prop);
}

void AircraftSystems::setFlaps(int position) {
    simConnect_->setFlaps(position);
    currentState_.flapsPosition = position;
}

void AircraftSystems::setGear(bool down) {
    simConnect_->setGear(down);
    currentState_.gearDown = down;
}

void AircraftSystems::setSpoilers(bool deployed) {
    std::cout << "Setting spoilers: " << (deployed ? "DEPLOYED" : "RETRACTED") << std::endl;
    simConnect_->setSpoilers(deployed);
}

void AircraftSystems::setParkingBrake(bool set) {
    simConnect_->setParkingBrake(set);
    currentState_.parkingBrakeSet = set;
}

void AircraftSystems::setBrakes(double value) {
    std::cout << "Setting brakes: " << (value * 100.0) << "%" << std::endl;
    simConnect_->setBrakes(value);
}

void AircraftSystems::startEngine(int engineIndex) {
    std::cout << "Starting engine " << engineIndex << std::endl;
    simConnect_->toggleEngineStarter(engineIndex);
}

void AircraftSystems::stopEngine(int engineIndex) {
    std::cout << "Stopping engine " << engineIndex << std::endl;
    simConnect_->setEngineState(engineIndex, false);
}

void AircraftSystems::setMagnetos(int position) {
    std::cout << "Setting magnetos: " << position << std::endl;
    simConnect_->setMagnetos(position);
}

void AircraftSystems::setNavigationLights(bool on) {
    std::cout << "Navigation lights: " << (on ? "ON" : "OFF") << std::endl;
    simConnect_->setLight("NAV", on);
}

void AircraftSystems::setBeaconLights(bool on) {
    std::cout << "Beacon lights: " << (on ? "ON" : "OFF") << std::endl;
    simConnect_->setLight("BEACON", on);
}

void AircraftSystems::setStrobeLights(bool on) {
    std::cout << "Strobe lights: " << (on ? "ON" : "OFF") << std::endl;
    simConnect_->setLight("STROBE", on);
}

void AircraftSystems::setLandingLights(bool on) {
    std::cout << "Landing lights: " << (on ? "ON" : "OFF") << std::endl;
    simConnect_->setLight("LANDING", on);
}

void AircraftSystems::setTaxiLights(bool on) {
    std::cout << "Taxi lights: " << (on ? "ON" : "OFF") << std::endl;
    simConnect_->setLight("TAXI", on);
}

AircraftState AircraftSystems::getCurrentState() const {
    return currentState_;
}

AutopilotState AircraftSystems::getAutopilotState() const {
    return autopilotState_;
}

bool AircraftSystems::checkSystems() {
    warnings_.clear();
    checkFuel();
    checkEngines();
    checkElectrical();
    return warnings_.empty();
}

std::vector<std::string> AircraftSystems::getSystemWarnings() {
    return warnings_;
}

void AircraftSystems::updateState() {
    currentState_ = simConnect_->getAircraftState();
    autopilotState_ = simConnect_->getAutopilotState();
}

void AircraftSystems::checkFuel() {
    double fuelPercentage = (currentState_.fuelQuantity / config_.fuelCapacity) * 100.0;
    
    if (fuelPercentage < 10.0) {
        warnings_.push_back("CRITICAL: Fuel below 10%");
    } else if (fuelPercentage < 20.0) {
        warnings_.push_back("WARNING: Fuel below 20%");
    }
}

void AircraftSystems::checkEngines() {
    if (currentState_.engineRPM < 100 && !currentState_.onGround) {
        warnings_.push_back("CRITICAL: Engine failure");
    }
}

void AircraftSystems::checkElectrical() {
    // Check if battery master is on
    if (!currentState_.masterBattery && !currentState_.onGround) {
        warnings_.push_back("CRITICAL: Battery master switch is OFF during flight");
    }
    
    // Check battery voltage - typical aircraft battery is 12V or 24V
    // Low voltage threshold: < 11V for 12V system, < 22V for 24V system
    if (currentState_.masterBattery) {
        if (currentState_.batteryVoltage < 11.0 && currentState_.batteryVoltage > 0.0) {
            warnings_.push_back("CRITICAL: Battery voltage critically low (" + 
                              std::to_string(static_cast<int>(currentState_.batteryVoltage)) + "V)");
        } else if (currentState_.batteryVoltage < 12.5 && currentState_.batteryVoltage >= 11.0) {
            warnings_.push_back("WARNING: Battery voltage low (" + 
                              std::to_string(static_cast<int>(currentState_.batteryVoltage)) + "V)");
        } else if (currentState_.batteryVoltage < 22.0 && currentState_.batteryVoltage >= 20.0) {
            // For 24V systems
            warnings_.push_back("CRITICAL: Battery voltage critically low (" + 
                              std::to_string(static_cast<int>(currentState_.batteryVoltage)) + "V)");
        } else if (currentState_.batteryVoltage < 24.5 && currentState_.batteryVoltage >= 22.0) {
            warnings_.push_back("WARNING: Battery voltage low (" + 
                              std::to_string(static_cast<int>(currentState_.batteryVoltage)) + "V)");
        }
    }
    
    // Check alternator/generator status during flight
    if (!currentState_.onGround && currentState_.engineRPM > 800) {
        // Engine is running, alternator should be providing power
        if (!currentState_.masterAlternator) {
            warnings_.push_back("WARNING: Alternator is OFF with engine running");
        }
        
        // Check generator voltage output
        if (currentState_.masterAlternator && currentState_.generatorVoltage < 13.5) {
            // For 12V system, alternator should output 13.5-14.5V
            if (currentState_.batteryVoltage < 20.0) { // Assume 12V system
                warnings_.push_back("WARNING: Alternator output low - possible alternator failure");
            }
        } else if (currentState_.masterAlternator && currentState_.generatorVoltage < 27.0) {
            // For 24V system, alternator should output 27-28.5V
            if (currentState_.batteryVoltage >= 20.0) { // Assume 24V system
                warnings_.push_back("WARNING: Alternator output low - possible alternator failure");
            }
        }
    }
    
    // Check for excessive electrical load
    if (currentState_.batteryLoad > 60.0) {
        warnings_.push_back("WARNING: High electrical load (" + 
                          std::to_string(static_cast<int>(currentState_.batteryLoad)) + "A)");
    } else if (currentState_.batteryLoad > 80.0) {
        warnings_.push_back("CRITICAL: Excessive electrical load (" + 
                          std::to_string(static_cast<int>(currentState_.batteryLoad)) + "A) - risk of electrical failure");
    }
    
    // Check if battery is discharging in flight (alternator not keeping up)
    if (!currentState_.onGround && currentState_.masterBattery && currentState_.masterAlternator) {
        // If alternator is on but battery voltage is dropping
        if (currentState_.engineRPM > 1000 && currentState_.batteryVoltage < 12.0 && currentState_.batteryVoltage > 0.0) {
            warnings_.push_back("CRITICAL: Battery discharging despite alternator running - electrical system failure");
        } else if (currentState_.engineRPM > 1000 && currentState_.batteryVoltage < 22.0 && currentState_.batteryVoltage >= 20.0) {
            warnings_.push_back("CRITICAL: Battery discharging despite alternator running - electrical system failure");
        }
    }
    
    // Check for complete electrical failure
    if (currentState_.masterBattery && currentState_.batteryVoltage < 5.0 && currentState_.batteryVoltage > 0.0) {
        warnings_.push_back("CRITICAL: COMPLETE ELECTRICAL FAILURE - Emergency procedures required");
    }
}

} // namespace AICopilot

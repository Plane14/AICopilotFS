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
    // TODO: Check electrical system
}

} // namespace AICopilot

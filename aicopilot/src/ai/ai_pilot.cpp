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
    , currentPhase_(FlightPhase::UNKNOWN)
    , fuelWarning20Shown_(false)
    , fuelWarning10Shown_(false) {
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
    
    if (emergencyType == "engine_failure") {
        handleEngineFailure();
    } else if (emergencyType == "low_fuel") {
        handleLowFuel();
    } else if (emergencyType == "fire") {
        handleFire();
    } else if (emergencyType == "loss_of_control") {
        handleLossOfControl();
    } else {
        // Generic emergency response
        log("Executing generic emergency procedures");
        // Squawk 7700
        // Find nearest suitable airport
        // Declare emergency with ATC
    }
}

void AIPilot::handleEngineFailure() {
    log("Engine failure - executing emergency procedures");
    
    // Maintain airspeed - best glide speed
    double bestGlideSpeed = aircraftConfig_.stallSpeed * 1.5;
    systems_->setSpeed(bestGlideSpeed);
    
    // Find nearest airport
    // TODO: Integrate with navdata to find nearest suitable airport
    
    // Configure for forced landing
    currentPhase_ = FlightPhase::APPROACH;
    
    // Attempt restart if altitude permits
    if (currentState_.position.altitude > 3000.0) {
        log("Attempting engine restart");
        systems_->setMagnetos(3); // BOTH
        systems_->setMixture(1.0); // Full rich
        systems_->startEngine(0);
    }
}

void AIPilot::handleFire() {
    log("Fire detected - executing fire procedures");
    
    // Shut down affected system
    systems_->stopEngine(0);
    systems_->setMixture(0.0);
    systems_->setThrottle(0.0);
    
    // Descend immediately
    if (!currentState_.onGround) {
        systems_->setAltitude(std::max(currentState_.position.altitude - 2000.0, 1000.0));
    }
    
    // Find nearest airport for emergency landing
    handleLowFuel(); // Reuse diversion logic
}

void AIPilot::handleLossOfControl() {
    log("Loss of control - executing recovery procedures");
    
    // Disable autopilot
    systems_->enableAutopilot(false);
    
    // Power to idle
    systems_->setThrottle(0.0);
    
    // Wings level recovery
    systems_->setRoll(0.0);
    
    // Recover from unusual attitude
    if (currentState_.pitch > 30.0) {
        // Nose high - push forward
        systems_->setPitch(-0.3);
    } else if (currentState_.pitch < -30.0) {
        // Nose low - pull back
        systems_->setPitch(0.3);
    }
    
    // Once recovered, re-engage autopilot
    if (std::abs(currentState_.bank) < 10.0 && std::abs(currentState_.pitch) < 15.0) {
        systems_->enableAutopilot(true);
        log("Aircraft control recovered");
    }
}

void AIPilot::setManualOverride(bool override) {
    manualOverride_ = override;
    log(override ? "Manual override ENABLED" : "Manual override DISABLED");
}

void AIPilot::enableOllamaATC(bool enable, const std::string& host) {
    if (atc_) {
        atc_->enableOllama(enable, host);
    } else {
        log("WARNING: ATC controller not initialized yet");
    }
}

void AIPilot::setOllamaModel(const std::string& model) {
    if (atc_) {
        atc_->setOllamaModel(model);
    } else {
        log("WARNING: ATC controller not initialized yet");
    }
}

bool AIPilot::isOllamaEnabled() const {
    if (atc_) {
        return atc_->isOllamaEnabled();
    }
    return false;
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
    systems_->setNavigationLights(true);
    systems_->setBeaconLights(true);
    
    // Release parking brake if set
    if (currentState_.parkingBrakeSet) {
        systems_->setParkingBrake(false);
    }
    
    // Taxi speed control (< 15 knots)
    if (currentState_.groundSpeed < 10.0) {
        systems_->setThrottle(0.2);
    } else if (currentState_.groundSpeed > 15.0) {
        systems_->setThrottle(0.0);
        systems_->setBrakes(0.5);
    }
    
    // Follow taxi route if available
    if (navigation_ && navigation_->getFlightPlan().waypoints.size() > 0) {
        Waypoint firstWaypoint = navigation_->getFlightPlan().waypoints[0];
        double bearing = navigation_->bearingToWaypoint(currentState_.position, firstWaypoint);
        
        // Simple heading control for taxi
        double headingDiff = bearing - currentState_.heading;
        if (headingDiff > 180.0) headingDiff -= 360.0;
        if (headingDiff < -180.0) headingDiff += 360.0;
        
        // Apply gentle rudder input
        if (std::abs(headingDiff) > 5.0) {
            double rudderInput = std::max(-0.3, std::min(0.3, headingDiff / 30.0));
            systems_->setYaw(rudderInput);
        }
    }
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
    
    // Landing speed (1.3 * stall speed)
    double landingSpeed = aircraftConfig_.stallSpeed * 1.3;
    
    // Flare logic
    if (currentState_.position.altitude < 50.0 && currentState_.position.altitude > 5.0) {
        log("Flaring for landing");
        // Gradual pitch increase for flare
        double flareAmount = (50.0 - currentState_.position.altitude) / 50.0;
        systems_->setPitch(0.05 + flareAmount * 0.1);
        
        // Reduce power gradually
        systems_->setThrottle(0.3 * (1.0 - flareAmount));
    }
    else if (currentState_.position.altitude <= 5.0) {
        log("Touchdown");
        // Touchdown - idle power
        systems_->setThrottle(0.0);
        systems_->setPitch(0.02); // Keep nose up slightly
    }
    
    // After touchdown
    if (currentState_.onGround && currentState_.groundSpeed > 0) {
        log("Rollout - applying brakes");
        // Progressive braking
        if (currentState_.groundSpeed > 60.0) {
            systems_->setBrakes(0.3);
        } else if (currentState_.groundSpeed > 30.0) {
            systems_->setBrakes(0.5);
        } else {
            systems_->setBrakes(0.7);
        }
        
        // Retract flaps after slowing down
        if (currentState_.groundSpeed < 40.0) {
            systems_->setFlaps(0);
        }
        
        // Transition to taxi when slow
        if (currentState_.groundSpeed < 10.0) {
            currentPhase_ = FlightPhase::TAXI_IN;
        }
    }
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
    
    static bool shutdownComplete = false;
    
    if (!shutdownComplete) {
        // Shutdown checklist
        
        // 1. Set parking brake
        systems_->setParkingBrake(true);
        log("Parking brake - SET");
        
        // 2. Throttle to idle
        systems_->setThrottle(0.0);
        log("Throttle - IDLE");
        
        // 3. Mixture to idle cutoff
        systems_->setMixture(0.0);
        log("Mixture - IDLE CUTOFF");
        
        // 4. Shut down engines
        for (int i = 0; i < aircraftConfig_.numberOfEngines; i++) {
            systems_->stopEngine(i);
        }
        log("Engines - SHUTDOWN");
        
        // 5. Magnetos OFF
        systems_->setMagnetos(0);
        log("Magnetos - OFF");
        
        // 6. Avionics and electrical
        // Note: These would need additional SimConnect controls
        log("Avionics - OFF");
        
        // 7. Lights OFF
        systems_->setNavigationLights(false);
        systems_->setBeaconLights(false);
        systems_->setStrobeLights(false);
        systems_->setLandingLights(false);
        systems_->setTaxiLights(false);
        log("Lights - OFF");
        
        // 8. Complete
        shutdownComplete = true;
        log("Shutdown checklist complete");
        
        // Stop autonomous flight
        stopAutonomousFlight();
    }
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
    
    // Fuel warnings at 20% and 10% (show once)
    double fuelPercent = (currentState_.fuelQuantity / aircraftConfig_.fuelCapacity) * 100.0;
    if (fuelPercent < 20.0 && !fuelWarning20Shown_) {
        log("WARNING: Fuel at 20%");
        fuelWarning20Shown_ = true;
    }
    if (fuelPercent < 10.0 && !fuelWarning10Shown_) {
        log("CAUTION: Fuel at 10% - diversion recommended");
        fuelWarning10Shown_ = true;
    }
    
    // Check terrain clearance
    if (!currentState_.onGround) {
        if (!checkTerrainClearance()) {
            log("WARNING: Terrain clearance issue");
        }
    }
    
    // Check weather suitability
    if (!isWeatherSuitable()) {
        handleBadWeather();
    }
    
    return safe;
}

void AIPilot::handleLowFuel() {
    log("WARNING: Low fuel - finding nearest airport");
    
    // Calculate remaining flight time
    double fuelRemaining = currentState_.fuelQuantity;
    double fuelBurnRate = 10.0; // gallons per hour (estimate)
    double remainingMinutes = (fuelRemaining / fuelBurnRate) * 60.0;
    
    log("Estimated fuel remaining: " + std::to_string(remainingMinutes) + " minutes");
    
    // Reduce power for best range
    double economySpeed = aircraftConfig_.cruiseSpeed * 0.75;
    systems_->setSpeed(economySpeed);
    
    // Find nearest suitable airport
    // TODO: Integrate with navdata database to find nearest airport
    log("Searching for nearest suitable airport for diversion");
    
    // Declare fuel emergency with ATC
    if (atc_) {
        log("Declaring minimum fuel with ATC");
        // ATC would be notified through SimConnect
    }
    
    // Update flight plan to divert
    if (fuelRemaining < 3.0) {
        log("CRITICAL: Fuel emergency - preparing for emergency landing");
        handleEmergency("low_fuel");
    }
}

void AIPilot::handleBadWeather() {
    log("WARNING: Bad weather detected");
    
    // Get current weather conditions
    WeatherConditions weather = assessWeather();
    
    // Decision tree for weather
    log("Assessing weather severity and options");
    
    // Check for severe weather conditions
    if (weather.icing && currentState_.position.altitude < 10000.0) {
        log("Icing conditions detected - climbing to avoid");
        double weatherAvoidanceAlt = std::min(currentState_.position.altitude + 2000.0, 
                                             aircraftConfig_.serviceceiling * 0.9);
        systems_->setAltitude(weatherAvoidanceAlt);
    }
    
    if (weather.visibility < 3.0 && currentPhase_ == FlightPhase::APPROACH) {
        log("Low visibility - consider missed approach");
        // Could trigger missed approach procedure
    }
    
    if (weather.turbulence) {
        log("Turbulence detected - reducing speed for comfort");
        double turbulenceSpeed = aircraftConfig_.cruiseSpeed * 0.85;
        systems_->setSpeed(turbulenceSpeed);
    }
    
    // Consider diversion if weather is too severe
    if (!isWeatherSuitable()) {
        log("Weather below minimums - considering diversion");
        if (atc_) {
            log("Requesting weather information from ATC");
        }
    }
}

bool AIPilot::checkTerrainClearance() {
    // Check if current altitude provides adequate terrain clearance
    double terrainElevation = getTerrainElevation(currentState_.position);
    double agl = currentState_.position.altitude - terrainElevation;
    
    // Minimum safe altitude is typically 1000 ft AGL in mountainous areas,
    // 500 ft AGL elsewhere
    double minimumClearance = 1000.0;
    
    if (agl < minimumClearance) {
        log("WARNING: Low terrain clearance - " + std::to_string(agl) + " ft AGL");
        
        // Take corrective action
        if (agl < minimumClearance * 0.5) {
            log("CRITICAL: Terrain proximity warning - climbing immediately");
            systems_->enableAutopilot(false);
            systems_->setPitch(0.2); // Pitch up
            systems_->setThrottle(1.0); // Full power
        }
        
        return false;
    }
    
    return true;
}

double AIPilot::getTerrainElevation(const Position& pos) {
    // TODO: Integrate with actual terrain database
    // For now, return a conservative estimate based on position
    // This would typically come from a terrain elevation database
    
    // Simplified terrain elevation (sea level default)
    double elevation = 0.0;
    
    // In a real implementation, this would:
    // 1. Query a terrain database (e.g., SRTM data)
    // 2. Use the navdata reader for terrain information
    // 3. Interpolate between known elevation points
    
    return elevation;
}

WeatherConditions AIPilot::assessWeather() {
    WeatherConditions weather;
    
    // TODO: Get actual weather from SimConnect
    // For now, return default conditions
    weather.windSpeed = 10.0;
    weather.windDirection = 270.0;
    weather.visibility = 10.0;
    weather.cloudBase = 3000.0;
    weather.temperature = 15.0;
    weather.icing = false;
    weather.turbulence = false;
    weather.precipitation = false;
    
    // In a real implementation, this would:
    // 1. Query SimConnect for current weather
    // 2. Parse METAR data if available
    // 3. Check for hazardous conditions
    
    return weather;
}

bool AIPilot::isWeatherSuitable() {
    WeatherConditions weather = assessWeather();
    
    // Check VFR minimums (simplified)
    bool vfrMinimums = (weather.visibility >= 3.0 && weather.cloudBase >= 1000.0);
    
    // Check for hazardous conditions
    bool safe = !weather.icing && weather.windSpeed < 35.0;
    
    // For IFR operations, different minimums would apply
    return vfrMinimums && safe;
}

void AIPilot::log(const std::string& message) {
    std::cout << "[AI Pilot] " << message << std::endl;
}

} // namespace AICopilot

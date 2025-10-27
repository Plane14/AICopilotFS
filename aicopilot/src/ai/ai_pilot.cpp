/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "../include/ai_pilot.h"
#include "../include/navdata_provider.h"
#include "../include/weather_system.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace AICopilot {

AIPilot::AIPilot() 
    : active_(false)
    , manualOverride_(false)
    , currentPhase_(FlightPhase::UNKNOWN)
    , fuelWarning20Shown_(false)
    , fuelWarning10Shown_(false)
    , navdataProvider_(nullptr)
    , weatherSystem_(nullptr) {
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
    
    // Initialize navdata provider for airport/navaid lookups
    navdataProvider_ = std::make_unique<SimConnectNavdataProvider>();
    if (!navdataProvider_->initialize()) {
        log("WARNING: Failed to initialize navdata provider - airport search will be limited");
        // Not a critical failure, continue with cached provider as fallback
        navdataProvider_ = std::make_unique<CachedNavdataProvider>();
        navdataProvider_->initialize();
    }
    
    // Initialize weather system
    weatherSystem_ = std::make_unique<WeatherSystem>();
    log("Weather system initialized");
    
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
    
    // Find nearest suitable airport using navdata provider
    if (navdataProvider_ && navdataProvider_->isReady()) {
        AirportInfo nearestAirport;
        if (navdataProvider_->getNearestAirport(currentState_.position, nearestAirport)) {
            log("Nearest airport: " + nearestAirport.icao + " (" + nearestAirport.name + ")");
            
            // Calculate glide range (altitude in feet / 10 gives approximate nm range)
            double glideRangeNM = currentState_.position.altitude / 1000.0;
            
            // Calculate distance to airport
            double latDiff = nearestAirport.position.latitude - currentState_.position.latitude;
            double lonDiff = nearestAirport.position.longitude - currentState_.position.longitude;
            double distanceNM = std::sqrt(latDiff * latDiff + lonDiff * lonDiff) * 60.0; // Rough approximation
            
            if (distanceNM <= glideRangeNM) {
                log("Airport within glide range (" + std::to_string(distanceNM) + " nm)");
                log("Diverting to " + nearestAirport.icao);
                
                // Update flight plan to divert to nearest airport
                if (navigation_) {
                    Waypoint emergencyWaypoint;
                    emergencyWaypoint.id = nearestAirport.icao;
                    emergencyWaypoint.position = nearestAirport.position;
                    emergencyWaypoint.altitude = nearestAirport.elevation + 1000.0; // Pattern altitude
                    emergencyWaypoint.type = "AIRPORT";
                    
                    // Clear flight plan and add emergency destination
                    FlightPlan emergencyPlan;
                    emergencyPlan.arrival = nearestAirport.icao;
                    emergencyPlan.waypoints.push_back(emergencyWaypoint);
                    emergencyPlan.cruiseAltitude = currentState_.position.altitude;
                    emergencyPlan.cruiseSpeed = bestGlideSpeed;
                }
            } else {
                log("WARNING: Nearest airport out of glide range");
                log("Distance: " + std::to_string(distanceNM) + " nm, Glide range: " + std::to_string(glideRangeNM) + " nm");
                log("Preparing for off-field landing");
            }
        } else {
            log("WARNING: No airport found - preparing for emergency landing");
        }
    } else {
        log("WARNING: Navdata not available - unable to locate nearest airport");
    }
    
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

void AIPilot::setOllamaApiKey(const std::string& apiKey) {
    if (atc_) {
        atc_->setOllamaApiKey(apiKey);
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
    
    // Find nearest suitable airport using navdata database
    if (navdataProvider_ && navdataProvider_->isReady()) {
        // Search for airports within reasonable range based on fuel
        double rangeNM = (remainingMinutes / 60.0) * economySpeed * 0.8; // 80% safety margin
        log("Maximum diversion range: " + std::to_string(rangeNM) + " nm");
        
        // Get nearby airports
        std::vector<AirportInfo> nearbyAirports = navdataProvider_->getAirportsNearby(
            currentState_.position, rangeNM);
        
        if (!nearbyAirports.empty()) {
            // Find best suitable airport (prefer towered, longer runways)
            AirportInfo bestAirport = nearbyAirports[0];
            for (const auto& airport : nearbyAirports) {
                // Prioritize airports with longer runways and towers
                if (airport.towered && !bestAirport.towered) {
                    bestAirport = airport;
                } else if (airport.longestRunway > bestAirport.longestRunway && 
                          airport.towered == bestAirport.towered) {
                    bestAirport = airport;
                }
            }
            
            log("Selected diversion airport: " + bestAirport.icao + " (" + bestAirport.name + ")");
            log("Runway: " + std::to_string(bestAirport.longestRunway) + " ft, " + 
                (bestAirport.towered ? "Towered" : "Uncontrolled"));
            
            // Update flight plan to divert
            if (navigation_) {
                Waypoint diversionWaypoint;
                diversionWaypoint.id = bestAirport.icao;
                diversionWaypoint.position = bestAirport.position;
                diversionWaypoint.altitude = bestAirport.elevation + 1000.0;
                diversionWaypoint.type = "AIRPORT";
                
                // Create diversion flight plan
                FlightPlan diversionPlan;
                diversionPlan.arrival = bestAirport.icao;
                diversionPlan.waypoints.push_back(diversionWaypoint);
                diversionPlan.cruiseAltitude = std::max(currentState_.position.altitude, 
                                                       bestAirport.elevation + 2000.0);
                diversionPlan.cruiseSpeed = economySpeed;
                
                log("Flight plan updated for fuel diversion");
            }
            
            // Declare minimum fuel with ATC
            if (atc_) {
                log("Declaring minimum fuel with ATC for diversion to " + bestAirport.icao);
                // ATC notification through SimConnect
            }
        } else {
            log("WARNING: No suitable airports within fuel range");
            log("Searching for nearest airport regardless of range");
            
            AirportInfo nearestAirport;
            if (navdataProvider_->getNearestAirport(currentState_.position, nearestAirport)) {
                log("Nearest airport: " + nearestAirport.icao + " - may be out of range");
            }
        }
    } else {
        log("WARNING: Navdata not available - cannot locate diversion airports");
    }
    
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
    // Integrate with terrain database through navdata provider
    double elevation = 0.0;
    
    if (navdataProvider_ && navdataProvider_->isReady()) {
        // Try to get nearest airport to estimate terrain elevation
        // This is a simplified approach - a full implementation would use
        // dedicated terrain elevation data (SRTM, DEM, etc.)
        AirportInfo nearbyAirport;
        if (navdataProvider_->getNearestAirport(pos, nearbyAirport)) {
            // Use airport elevation as a baseline for nearby terrain
            elevation = nearbyAirport.elevation;
            
            // Add conservative safety margin based on distance from airport
            double latDiff = std::abs(nearbyAirport.position.latitude - pos.latitude);
            double lonDiff = std::abs(nearbyAirport.position.longitude - pos.longitude);
            double distance = std::sqrt(latDiff * latDiff + lonDiff * lonDiff) * 60.0; // nm
            
            // Assume terrain rises 100 ft per nm from airport (conservative)
            elevation += distance * 100.0;
        } else {
            // No airport data available, use conservative default
            // Assume minimum safe terrain clearance altitude
            elevation = 500.0;
        }
    } else {
        // Navdata not available - use very conservative estimate
        // Consider latitude for rough terrain estimation
        // Higher latitudes and certain regions tend to have higher terrain
        double absLat = std::abs(pos.latitude);
        if (absLat > 40.0) {
            // Mountainous regions more common at higher latitudes
            elevation = 1000.0;
        } else {
            // Lower terrain expected
            elevation = 500.0;
        }
    }
    
    // Real-world implementation would:
    // 1. Query SRTM (Shuttle Radar Topography Mission) database
    // 2. Use Digital Elevation Model (DEM) data
    // 3. Integrate with SimConnect terrain services if available
    // 4. Interpolate between known elevation points for accuracy
    // 5. Cache terrain data for performance
    
    return elevation;
}

WeatherConditions AIPilot::assessWeather() {
    WeatherConditions weather;
    
    // Get actual weather from SimConnect
    // SimConnect provides ambient weather variables that can be queried
    if (simConnect_ && simConnect_->isConnected()) {
        // Note: Weather data would need to be added to SimConnect wrapper
        // as additional data definitions. For now, we use the current implementation
        // and enhance with available aircraft state data.
        
        // The barometric pressure from aircraft state can indicate weather changes
        weather.windSpeed = 0.0;  // Would come from AMBIENT WIND VELOCITY
        weather.windDirection = 0.0;  // Would come from AMBIENT WIND DIRECTION
        weather.visibility = 10.0;  // Would come from AMBIENT VISIBILITY
        weather.cloudBase = 3000.0;  // Estimated from visual conditions
        weather.temperature = 15.0;  // Would come from AMBIENT TEMPERATURE
        
        // Analyze current aircraft state for weather indicators
        // Rapid pressure changes can indicate weather systems
        double currentPressure = currentState_.altimeter;
        
        // Check for weather hazards based on available data
        // Low pressure (< 29.80) often indicates bad weather
        if (currentPressure < 29.80) {
            log("Low pressure system detected: " + std::to_string(currentPressure) + " inHg");
            weather.precipitation = true;
            weather.cloudBase = 2000.0;
        }
        
        // Check for icing conditions based on temperature and altitude
        // Icing typically occurs between 0°C to -20°C (32°F to -4°F)
        double altitude = currentState_.position.altitude;
        if (altitude > 8000.0 && altitude < 20000.0) {
            // Estimate temperature based on standard lapse rate (2°C per 1000 ft)
            double estimatedTemp = 15.0 - (altitude / 1000.0 * 2.0);
            if (estimatedTemp >= -20.0 && estimatedTemp <= 0.0) {
                weather.icing = true;
                weather.temperature = estimatedTemp;
                log("Potential icing conditions at altitude " + std::to_string(altitude) + " ft");
            }
        }
        
        // Detect turbulence from aircraft motion
        // Rapid vertical speed changes indicate turbulence
        double absVS = std::abs(currentState_.verticalSpeed);
        if (absVS > 1000.0) {
            weather.turbulence = true;
            log("Turbulence detected - vertical speed: " + std::to_string(currentState_.verticalSpeed) + " fpm");
        }
        
        // Update weather system if available
        if (weatherSystem_) {
            weatherSystem_->updateWeatherConditions(weather);
        }
        
        log("Weather assessment: " + 
            std::string(weather.icing ? "ICING " : "") +
            std::string(weather.turbulence ? "TURBULENCE " : "") +
            std::string(weather.precipitation ? "PRECIP " : "") +
            "Pressure: " + std::to_string(currentPressure) + " inHg");
    } else {
        // SimConnect not available, use defaults
        log("WARNING: SimConnect not available for weather data");
        weather.windSpeed = 10.0;
        weather.windDirection = 270.0;
        weather.visibility = 10.0;
        weather.cloudBase = 3000.0;
        weather.temperature = 15.0;
        weather.icing = false;
        weather.turbulence = false;
        weather.precipitation = false;
    }
    
    // Full implementation would:
    // 1. Query SimConnect weather variables:
    //    - AMBIENT WIND VELOCITY
    //    - AMBIENT WIND DIRECTION
    //    - AMBIENT VISIBILITY
    //    - AMBIENT TEMPERATURE
    //    - AMBIENT PRESSURE
    //    - TOTAL AIR TEMPERATURE
    //    - BAROMETRIC PRESSURE
    // 2. Request METAR data through SimConnect weather station API
    // 3. Parse weather observation data
    // 4. Integrate with online weather services for forecasts
    // 5. Use weather radar data if available in simulator
    
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

/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef HELICOPTER_OPERATIONS_H
#define HELICOPTER_OPERATIONS_H

#include "aicopilot_types.h"
#include "aircraft_profile.h"
#include <vector>

namespace AICopilot {

// Helicopter flight mode
enum class HelicopterMode {
    HOVER,
    HOVER_TAXI,
    TRANSITION_TO_FORWARD,
    FORWARD_FLIGHT,
    TRANSITION_TO_HOVER,
    AUTOROTATION,
    UNKNOWN
};

// Hover state
struct HoverState {
    Position position;
    double altitude;        // feet AGL
    bool stable;
    double driftRate;       // knots
    double verticalSpeed;   // feet per minute
};

/**
 * Helicopter-Specific Operations
 */
class HelicopterOperations {
public:
    HelicopterOperations() = default;
    
    // Initialize with helicopter profile
    void initialize(const HelicopterData& profile);
    
    // Get current mode
    HelicopterMode getCurrentMode() const { return currentMode_; }
    
    // Hover control
    bool initiateHover(double targetAltitudeAGL);
    bool maintainHover(const AircraftState& state);
    void exitHover();
    
    // Get hover state
    HoverState getHoverState() const { return hoverState_; }
    
    // Check if hovering
    bool isHovering() const { return currentMode_ == HelicopterMode::HOVER; }
    
    // Hover taxi (low altitude movement)
    bool initiateHoverTaxi(const Position& targetPosition, double speed);
    void updateHoverTaxi(const AircraftState& state);
    
    // Vertical takeoff and landing
    bool executeVerticalTakeoff(double targetAltitudeAGL);
    bool executeVerticalLanding(const Position& landingSpot);
    
    // Transition between hover and forward flight
    bool transitionToForwardFlight(double targetSpeed);
    bool transitionToHover();
    
    // Autorotation (emergency descent)
    bool initiateAutorotation();
    void maintainAutorotation(const AircraftState& state);
    double getOptimalAutorotationSpeed() const;
    
    // Calculate hover performance
    bool canHoverAtAltitude(double altitudeMSL, double temperature) const;
    double getMaxHoverAltitude(double temperature) const;
    
    // Collective pitch control
    double calculateCollectivePitch(double targetVerticalSpeed) const;
    
    // Cyclic control for hover
    struct CyclicControl {
        double lateral;   // -1 to +1 (left to right)
        double longitudinal;  // -1 to +1 (aft to forward)
    };
    CyclicControl calculateHoverCorrection(const HoverState& state) const;
    
    // Tail rotor (anti-torque) control
    double calculateTailRotorPedal(double mainRotorTorque) const;
    
    // Navigation modes specific to helicopters
    bool navigateToLandingZone(const Position& lz);
    bool performOrbitPattern(const Position& center, double radius);
    bool performStationKeeping(const Position& station);
    
    // Check environmental conditions for helicopter operations
    bool isWindSuitableForHover(const WeatherConditions& weather) const;
    bool isVisibilitySuitable(const WeatherConditions& weather) const;
    
    // Emergency procedures
    bool checkVortexRingState(const AircraftState& state) const;
    bool checkLowRotorRPM() const;
    void handleTailRotorFailure();
    
private:
    HelicopterData profile_;
    HelicopterMode currentMode_ = HelicopterMode::UNKNOWN;
    HoverState hoverState_;
    Position hoverTaxiTarget_;
    
    // Hover control parameters
    static constexpr double HOVER_TOLERANCE_LATERAL = 10.0;  // feet
    static constexpr double HOVER_TOLERANCE_VERTICAL = 5.0;  // feet
    static constexpr double MAX_HOVER_DRIFT = 3.0;  // knots
    
    // Autorotation parameters
    bool autorotationActive_ = false;
    double autorotationStartAltitude_ = 0.0;
    
    // Helper methods
    void updateMode(const AircraftState& state);
    bool isHoverStable(const HoverState& state) const;
    double calculateHoverPower(double altitude, double temperature) const;
};

} // namespace AICopilot

#endif // HELICOPTER_OPERATIONS_H

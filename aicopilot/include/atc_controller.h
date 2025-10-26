/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef ATC_CONTROLLER_H
#define ATC_CONTROLLER_H

#include "aicopilot_types.h"
#include "simconnect_wrapper.h"
#include <memory>
#include <queue>

namespace AICopilot {

/**
 * ATC interaction controller
 * Handles communication with ATC, parses instructions, and selects appropriate responses
 */
class ATCController {
public:
    ATCController(std::shared_ptr<SimConnectWrapper> simConnect);
    ~ATCController();
    
    // Update ATC state (call regularly)
    void update();
    
    // Process incoming ATC message
    void processATCMessage(const ATCMessage& message);
    
    // Request ATC menu
    void requestMenu();
    
    // Set current flight phase for context-aware responses
    void setFlightPhase(FlightPhase phase);
    
    // Set flight plan for ATC coordination
    void setFlightPlan(const FlightPlan& plan);
    
    // Get pending instructions
    std::vector<std::string> getPendingInstructions() const;
    
    // Check if waiting for ATC response
    bool isWaitingForATC() const;
    
    // Get last ATC clearance
    std::string getLastClearance() const;
    
private:
    std::shared_ptr<SimConnectWrapper> simConnect_;
    FlightPhase currentPhase_;
    FlightPlan flightPlan_;
    std::queue<ATCMessage> messageQueue_;
    std::vector<std::string> pendingInstructions_;
    std::string lastClearance_;
    bool waitingForResponse_;
    
    // AI decision making for ATC menu selection
    int selectBestMenuOption(const ATCMessage& message);
    
    // Parse ATC instruction
    void parseInstruction(const std::string& instruction);
    
    // Context analysis
    bool isRelevantForPhase(const std::string& option, FlightPhase phase);
    int scoreOption(const std::string& option, FlightPhase phase);
};

} // namespace AICopilot

#endif // ATC_CONTROLLER_H

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
#include "ollama_client.h"
#include <memory>
#include <queue>

namespace AICopilot {

namespace Integration {
class AirportOperationSystem;
}

/**
 * ATC interaction controller
 * Handles communication with ATC, parses instructions, and selects appropriate responses
 * Supports both rule-based and AI-powered (Ollama) menu selection
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

    // Connect to airport operations system for ground coordination
    void setAirportOperations(Integration::AirportOperationSystem* operations);
    
    // Get pending instructions
    std::vector<std::string> getPendingInstructions() const;
    
    // Check if waiting for ATC response
    bool isWaitingForATC() const;
    
    // Get last ATC clearance
    std::string getLastClearance() const;
    
    // Enable/disable Ollama AI assistance
    void enableOllama(bool enable, const std::string& host = "http://localhost:11434");
    
    // Check if Ollama is available and enabled
    bool isOllamaEnabled() const;
    
    // Set Ollama model
    void setOllamaModel(const std::string& model);
    
    // Set Ollama API key (optional, for enterprise/cloud deployments)
    void setOllamaApiKey(const std::string& apiKey);
    
private:
    std::shared_ptr<SimConnectWrapper> simConnect_;
    std::unique_ptr<OllamaClient> ollamaClient_;
    FlightPhase currentPhase_;
    FlightPlan flightPlan_;
    std::queue<ATCMessage> messageQueue_;
    std::vector<std::string> pendingInstructions_;
    std::string lastClearance_;
    bool waitingForResponse_;
    bool ollamaEnabled_;
    Integration::AirportOperationSystem* airportOps_;
    
    // AI decision making for ATC menu selection
    int selectBestMenuOption(const ATCMessage& message);
    
    // Rule-based menu selection (fallback)
    int selectBestMenuOptionRuleBased(const ATCMessage& message);
    
    // Parse ATC instruction
    void parseInstruction(const std::string& instruction);
    void handleInstruction(const std::string& instruction);
    
    // Context analysis
    bool isRelevantForPhase(const std::string& option, FlightPhase phase);
    int scoreOption(const std::string& option, FlightPhase phase);
    
    // Get flight phase as string
    std::string getFlightPhaseString() const;
};

} // namespace AICopilot

#endif // ATC_CONTROLLER_H

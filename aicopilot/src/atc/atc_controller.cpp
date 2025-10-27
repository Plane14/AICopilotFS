/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "../include/atc_controller.h"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace AICopilot {

ATCController::ATCController(std::shared_ptr<SimConnectWrapper> simConnect)
    : simConnect_(simConnect)
    , ollamaClient_(std::make_unique<OllamaClient>())
    , currentPhase_(FlightPhase::UNKNOWN)
    , waitingForResponse_(false)
    , ollamaEnabled_(false) {
    
    // Subscribe to ATC messages
    simConnect_->subscribeToATCMessages([this](const ATCMessage& msg) {
        processATCMessage(msg);
    });
}

ATCController::~ATCController() = default;

void ATCController::update() {
    // Process queued messages
    while (!messageQueue_.empty()) {
        ATCMessage msg = messageQueue_.front();
        messageQueue_.pop();
        
        // Select best menu option
        int selectedOption = selectBestMenuOption(msg);
        
        if (selectedOption >= 0) {
            simConnect_->sendATCMenuSelection(selectedOption);
            lastClearance_ = msg.menuOptions[selectedOption];
            parseInstruction(lastClearance_);
            waitingForResponse_ = false;
        }
    }
}

void ATCController::processATCMessage(const ATCMessage& message) {
    std::cout << "ATC: " << message.message << std::endl;
    messageQueue_.push(message);
    waitingForResponse_ = true;
}

void ATCController::requestMenu() {
    simConnect_->requestATCMenu();
}

void ATCController::setFlightPhase(FlightPhase phase) {
    currentPhase_ = phase;
}

void ATCController::setFlightPlan(const FlightPlan& plan) {
    flightPlan_ = plan;
}

std::vector<std::string> ATCController::getPendingInstructions() const {
    return pendingInstructions_;
}

bool ATCController::isWaitingForATC() const {
    return waitingForResponse_;
}

std::string ATCController::getLastClearance() const {
    return lastClearance_;
}

void ATCController::enableOllama(bool enable, const std::string& host) {
    if (enable) {
        std::cout << "[ATC] Enabling Ollama AI assistance..." << std::endl;
        if (ollamaClient_->connect(host)) {
            ollamaEnabled_ = true;
            std::cout << "[ATC] Ollama AI enabled successfully" << std::endl;
        } else {
            ollamaEnabled_ = false;
            std::cout << "[ATC] Failed to enable Ollama, falling back to rule-based selection" << std::endl;
        }
    } else {
        ollamaEnabled_ = false;
        std::cout << "[ATC] Ollama AI disabled, using rule-based selection" << std::endl;
    }
}

bool ATCController::isOllamaEnabled() const {
    return ollamaEnabled_ && ollamaClient_->isAvailable();
}

void ATCController::setOllamaModel(const std::string& model) {
    if (ollamaClient_) {
        ollamaClient_->setModel(model);
    }
}

void ATCController::setOllamaApiKey(const std::string& apiKey) {
    if (ollamaClient_) {
        ollamaClient_->setApiKey(apiKey);
    }
}

std::string ATCController::getFlightPhaseString() const {
    switch (currentPhase_) {
        case FlightPhase::PREFLIGHT: return "PREFLIGHT";
        case FlightPhase::TAXI_OUT: return "TAXI OUT";
        case FlightPhase::TAKEOFF: return "TAKEOFF";
        case FlightPhase::CLIMB: return "CLIMB";
        case FlightPhase::CRUISE: return "CRUISE";
        case FlightPhase::DESCENT: return "DESCENT";
        case FlightPhase::APPROACH: return "APPROACH";
        case FlightPhase::LANDING: return "LANDING";
        case FlightPhase::TAXI_IN: return "TAXI IN";
        case FlightPhase::SHUTDOWN: return "SHUTDOWN";
        default: return "UNKNOWN";
    }
}

int ATCController::selectBestMenuOption(const ATCMessage& message) {
    if (message.menuOptions.empty()) {
        return -1;
    }
    
    // Try Ollama AI first if enabled
    if (isOllamaEnabled()) {
        std::cout << "[ATC] Using Ollama AI for menu selection" << std::endl;
        
        // Build context string
        std::ostringstream context;
        if (!flightPlan_.departure.empty() && !flightPlan_.arrival.empty()) {
            context << "Flight plan: " << flightPlan_.departure 
                   << " to " << flightPlan_.arrival;
        }
        
        int ollamaChoice = ollamaClient_->selectATCMenuOption(
            message.message,
            message.menuOptions,
            getFlightPhaseString(),
            context.str()
        );
        
        if (ollamaChoice >= 0 && ollamaChoice < static_cast<int>(message.menuOptions.size())) {
            std::cout << "[ATC] Ollama selected option " << ollamaChoice << ": " 
                     << message.menuOptions[ollamaChoice] << std::endl;
            return ollamaChoice;
        } else {
            std::cout << "[ATC] Ollama failed to select, falling back to rule-based" << std::endl;
        }
    }
    
    // Fall back to rule-based selection
    return selectBestMenuOptionRuleBased(message);
}

int ATCController::selectBestMenuOptionRuleBased(const ATCMessage& message) {
    if (message.menuOptions.empty()) {
        return -1;
    }
    
    std::cout << "[ATC] Using rule-based menu selection" << std::endl;
    
    int bestScore = -1;
    int bestOption = 0;
    
    for (size_t i = 0; i < message.menuOptions.size(); i++) {
        int score = scoreOption(message.menuOptions[i], currentPhase_);
        if (score > bestScore) {
            bestScore = score;
            bestOption = static_cast<int>(i);
        }
    }
    
    std::cout << "Selected ATC option " << bestOption << ": " 
              << message.menuOptions[bestOption] << std::endl;
    
    return bestOption;
}

void ATCController::parseInstruction(const std::string& instruction) {
    // Parse common ATC instructions
    std::string lower = instruction;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Extract altitude instructions (with numeric values)
    if (lower.find("climb") != std::string::npos || 
        lower.find("descend") != std::string::npos ||
        lower.find("maintain") != std::string::npos) {
        
        // Try to extract altitude number
        std::string altitudeInstruction = "Altitude: " + instruction;
        pendingInstructions_.push_back(altitudeInstruction);
    }
    
    // Extract heading instructions (with numeric values)
    if (lower.find("turn") != std::string::npos || 
        lower.find("heading") != std::string::npos ||
        lower.find("fly heading") != std::string::npos) {
        
        std::string headingInstruction = "Heading: " + instruction;
        pendingInstructions_.push_back(headingInstruction);
    }
    
    // Extract speed instructions
    if (lower.find("speed") != std::string::npos ||
        lower.find("slow") != std::string::npos ||
        lower.find("reduce speed") != std::string::npos) {
        
        std::string speedInstruction = "Speed: " + instruction;
        pendingInstructions_.push_back(speedInstruction);
    }
    
    // Extract clearances
    if (lower.find("cleared") != std::string::npos) {
        if (lower.find("takeoff") != std::string::npos) {
            pendingInstructions_.push_back("Cleared for takeoff");
        } else if (lower.find("land") != std::string::npos) {
            pendingInstructions_.push_back("Cleared to land");
        } else if (lower.find("approach") != std::string::npos) {
            pendingInstructions_.push_back("Cleared for approach");
        } else {
            pendingInstructions_.push_back(instruction);
        }
    }
    
    // Extract frequency changes
    if (lower.find("contact") != std::string::npos ||
        lower.find("frequency") != std::string::npos) {
        std::string freqInstruction = "Frequency change: " + instruction;
        pendingInstructions_.push_back(freqInstruction);
    }
    
    // Extract holding instructions
    if (lower.find("hold") != std::string::npos) {
        pendingInstructions_.push_back("Hold: " + instruction);
    }
    
    // Extract squawk codes
    if (lower.find("squawk") != std::string::npos) {
        pendingInstructions_.push_back("Squawk: " + instruction);
    }
}

bool ATCController::isRelevantForPhase(const std::string& option, FlightPhase phase) {
    std::string lower = option;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    switch (phase) {
        case FlightPhase::PREFLIGHT:
            return lower.find("clearance") != std::string::npos ||
                   lower.find("ready") != std::string::npos;
            
        case FlightPhase::TAXI_OUT:
            return lower.find("taxi") != std::string::npos ||
                   lower.find("ready") != std::string::npos;
            
        case FlightPhase::TAKEOFF:
            return lower.find("takeoff") != std::string::npos ||
                   lower.find("departure") != std::string::npos;
            
        case FlightPhase::CLIMB:
        case FlightPhase::CRUISE:
            return lower.find("altitude") != std::string::npos ||
                   lower.find("heading") != std::string::npos;
            
        case FlightPhase::DESCENT:
        case FlightPhase::APPROACH:
            return lower.find("approach") != std::string::npos ||
                   lower.find("landing") != std::string::npos ||
                   lower.find("descend") != std::string::npos;
            
        case FlightPhase::LANDING:
        case FlightPhase::TAXI_IN:
            return lower.find("land") != std::string::npos ||
                   lower.find("taxi") != std::string::npos ||
                   lower.find("gate") != std::string::npos;
            
        default:
            return false;
    }
}

int ATCController::scoreOption(const std::string& option, FlightPhase phase) {
    int score = 0;
    
    // High priority if relevant to current phase
    if (isRelevantForPhase(option, phase)) {
        score += 100;
    }
    
    std::string lower = option;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Positive keywords - actions we want to take
    if (lower.find("request") != std::string::npos) score += 50;
    if (lower.find("ready") != std::string::npos) score += 40;
    if (lower.find("roger") != std::string::npos) score += 30;
    if (lower.find("wilco") != std::string::npos) score += 30;
    if (lower.find("affirmative") != std::string::npos) score += 30;
    if (lower.find("yes") != std::string::npos) score += 25;
    if (lower.find("acknowledge") != std::string::npos) score += 25;
    
    // Phase-specific high-value keywords
    if (lower.find("clearance delivery") != std::string::npos && 
        phase == FlightPhase::PREFLIGHT) score += 80;
    if (lower.find("pushback") != std::string::npos && 
        phase == FlightPhase::PREFLIGHT) score += 70;
    if (lower.find("taxi to") != std::string::npos && 
        phase == FlightPhase::TAXI_OUT) score += 80;
    if (lower.find("takeoff clearance") != std::string::npos && 
        phase == FlightPhase::TAKEOFF) score += 90;
    if (lower.find("departure") != std::string::npos && 
        (phase == FlightPhase::TAKEOFF || phase == FlightPhase::CLIMB)) score += 70;
    if (lower.find("approach clearance") != std::string::npos && 
        phase == FlightPhase::DESCENT) score += 80;
    if (lower.find("landing clearance") != std::string::npos && 
        phase == FlightPhase::APPROACH) score += 90;
    if (lower.find("taxi to gate") != std::string::npos && 
        phase == FlightPhase::TAXI_IN) score += 80;
    
    // Negative keywords - typically don't want these unless specific situation
    if (lower.find("unable") != std::string::npos) score -= 50;
    if (lower.find("standby") != std::string::npos) score -= 20;
    if (lower.find("cancel") != std::string::npos) score -= 50;
    if (lower.find("reject") != std::string::npos) score -= 40;
    if (lower.find("negative") != std::string::npos) score -= 30;
    if (lower.find("deny") != std::string::npos) score -= 40;
    
    // Emergency keywords - high priority when needed
    if (lower.find("emergency") != std::string::npos) score += 200;
    if (lower.find("mayday") != std::string::npos) score += 200;
    if (lower.find("pan pan") != std::string::npos) score += 150;
    
    return score;
}

} // namespace AICopilot

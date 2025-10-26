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
    , currentPhase_(FlightPhase::UNKNOWN)
    , waitingForResponse_(false) {
    
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

int ATCController::selectBestMenuOption(const ATCMessage& message) {
    if (message.menuOptions.empty()) {
        return -1;
    }
    
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
    
    // Extract altitude instructions
    if (lower.find("climb") != std::string::npos || 
        lower.find("descend") != std::string::npos ||
        lower.find("maintain") != std::string::npos) {
        pendingInstructions_.push_back(instruction);
    }
    
    // Extract heading instructions
    if (lower.find("turn") != std::string::npos || 
        lower.find("heading") != std::string::npos) {
        pendingInstructions_.push_back(instruction);
    }
    
    // Extract clearances
    if (lower.find("cleared") != std::string::npos) {
        pendingInstructions_.push_back(instruction);
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
    
    // Positive keywords
    if (lower.find("request") != std::string::npos) score += 50;
    if (lower.find("ready") != std::string::npos) score += 40;
    if (lower.find("roger") != std::string::npos) score += 30;
    if (lower.find("wilco") != std::string::npos) score += 30;
    
    // Negative keywords (typically don't want these unless specific situation)
    if (lower.find("unable") != std::string::npos) score -= 50;
    if (lower.find("standby") != std::string::npos) score -= 20;
    if (lower.find("cancel") != std::string::npos) score -= 50;
    
    return score;
}

} // namespace AICopilot

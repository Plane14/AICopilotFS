/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "ollama_client.h"
#include <iostream>

// This is a stub implementation for when CURL/jsoncpp are not available
// To enable full Ollama integration, build with -DENABLE_OLLAMA=ON

namespace AICopilot {

// Define the Impl class for PIMPL pattern
class OllamaClient::Impl {
public:
    std::string url;
    std::string apiKey;
    std::string model;
    bool connected = false;
};

OllamaClient::OllamaClient() 
    : pImpl_(std::make_unique<Impl>())
    , host_("http://localhost:11434")
    , model_("llama2")
    , available_(false) {
    std::cout << "OllamaClient stub initialized (Ollama disabled - build with -DENABLE_OLLAMA=ON to enable)" << std::endl;
}

OllamaClient::~OllamaClient() = default;

bool OllamaClient::connect(const std::string& url) {
    host_ = url;
    std::cout << "OllamaClient::connect() - Stub implementation, Ollama not available" << std::endl;
    return false;
}

void OllamaClient::setApiKey(const std::string& apiKey) {
    apiKey_ = apiKey;
}

void OllamaClient::setModel(const std::string& model) {
    model_ = model;
}

bool OllamaClient::isAvailable() const {
    return available_;  // Stub always returns false
}

int OllamaClient::selectATCMenuOption(
    const std::string& situation,
    const std::vector<std::string>& options,
    const std::string& aircraftType,
    const std::string& flightPhase) {
    
    std::cout << "OllamaClient::selectATCMenuOption() - Stub implementation" << std::endl;
    std::cout << "  Situation: " << situation << std::endl;
    std::cout << "  Options: " << options.size() << " available" << std::endl;
    
    // Return first option as fallback
    return options.empty() ? -1 : 0;
}

} // namespace AICopilot

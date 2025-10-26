/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef OLLAMA_CLIENT_H
#define OLLAMA_CLIENT_H

#include <string>
#include <vector>
#include <memory>

namespace AICopilot {

/**
 * Ollama LLM client for AI-powered ATC menu selection
 * Provides intelligent decision making using local Ollama models
 */
class OllamaClient {
public:
    OllamaClient();
    ~OllamaClient();
    
    // Connect to Ollama server
    bool connect(const std::string& host = "http://localhost:11434");
    
    // Set API key for authentication (optional, for enterprise/cloud Ollama deployments)
    void setApiKey(const std::string& apiKey);
    
    // Check if Ollama is available
    bool isAvailable() const;
    
    // Set model to use (default: llama2)
    void setModel(const std::string& model);
    
    // Get current model
    std::string getModel() const { return model_; }
    
    /**
     * Select best ATC menu option using LLM reasoning
     * @param atcMessage The ATC message text
     * @param menuOptions Available menu options
     * @param flightPhase Current flight phase
     * @param context Additional context (flight plan, aircraft state, etc.)
     * @return Index of best option, or -1 if LLM unavailable
     */
    int selectATCMenuOption(
        const std::string& atcMessage,
        const std::vector<std::string>& menuOptions,
        const std::string& flightPhase,
        const std::string& context = ""
    );
    
    /**
     * Generate a prompt for Ollama
     * @param atcMessage The ATC message
     * @param menuOptions Available options
     * @param flightPhase Current phase
     * @param context Additional context
     * @return Formatted prompt string
     */
    std::string generatePrompt(
        const std::string& atcMessage,
        const std::vector<std::string>& menuOptions,
        const std::string& flightPhase,
        const std::string& context
    ) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
    
    std::string host_;
    std::string model_;
    std::string apiKey_;
    bool available_;
    
    // Parse LLM response to extract option number
    int parseResponse(const std::string& response, size_t numOptions) const;
    
    // Send request to Ollama API
    std::string sendRequest(const std::string& prompt) const;
};

} // namespace AICopilot

#endif // OLLAMA_CLIENT_H

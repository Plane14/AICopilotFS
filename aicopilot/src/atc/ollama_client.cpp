/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "../include/ollama_client.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <curl/curl.h>
#include <json/json.h>

namespace AICopilot {

// Callback for libcurl to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

class OllamaClient::Impl {
public:
    CURL* curl = nullptr;
    
    Impl() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
    }
    
    ~Impl() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
    }
};

OllamaClient::OllamaClient() 
    : pImpl_(std::make_unique<Impl>())
    , host_("http://localhost:11434")
    , model_("llama2")
    , available_(false) {
}

OllamaClient::~OllamaClient() = default;

bool OllamaClient::connect(const std::string& host) {
    host_ = host;
    
    // Test connection by trying to list models
    if (!pImpl_->curl) {
        std::cerr << "[Ollama] CURL not initialized" << std::endl;
        available_ = false;
        return false;
    }
    
    std::string url = host_ + "/api/tags";
    std::string response;
    
    curl_easy_setopt(pImpl_->curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(pImpl_->curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(pImpl_->curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(pImpl_->curl, CURLOPT_TIMEOUT, 5L);
    
    CURLcode res = curl_easy_perform(pImpl_->curl);
    
    if (res != CURLE_OK) {
        std::cerr << "[Ollama] Connection failed: " << curl_easy_strerror(res) << std::endl;
        available_ = false;
        return false;
    }
    
    std::cout << "[Ollama] Connected to " << host_ << std::endl;
    available_ = true;
    return true;
}

bool OllamaClient::isAvailable() const {
    return available_;
}

void OllamaClient::setModel(const std::string& model) {
    model_ = model;
    std::cout << "[Ollama] Model set to: " << model_ << std::endl;
}

std::string OllamaClient::generatePrompt(
    const std::string& atcMessage,
    const std::vector<std::string>& menuOptions,
    const std::string& flightPhase,
    const std::string& context
) const {
    std::ostringstream prompt;
    
    prompt << "You are an AI assistant helping a pilot select the best ATC response.\n\n";
    prompt << "Current Flight Phase: " << flightPhase << "\n";
    
    if (!context.empty()) {
        prompt << "Context: " << context << "\n";
    }
    
    prompt << "\nATC Message: \"" << atcMessage << "\"\n\n";
    prompt << "Available Response Options:\n";
    
    for (size_t i = 0; i < menuOptions.size(); i++) {
        prompt << i << ". " << menuOptions[i] << "\n";
    }
    
    prompt << "\nBased on the flight phase, ATC message, and available options, ";
    prompt << "select the MOST APPROPRIATE response by responding with ONLY the option number (0-" 
           << (menuOptions.size() - 1) << "). ";
    prompt << "Consider standard aviation procedures and ATC communications. ";
    prompt << "Respond with just the number, nothing else.";
    
    return prompt.str();
}

std::string OllamaClient::sendRequest(const std::string& prompt) const {
    if (!available_ || !pImpl_->curl) {
        return "";
    }
    
    // Build JSON request
    Json::Value request;
    request["model"] = model_;
    request["prompt"] = prompt;
    request["stream"] = false;
    request["options"]["temperature"] = 0.3; // Lower temperature for more deterministic responses
    
    Json::StreamWriterBuilder writer;
    std::string jsonRequest = Json::writeString(writer, request);
    
    // Prepare CURL request
    std::string url = host_ + "/api/generate";
    std::string response;
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(pImpl_->curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(pImpl_->curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(pImpl_->curl, CURLOPT_POSTFIELDS, jsonRequest.c_str());
    curl_easy_setopt(pImpl_->curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(pImpl_->curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(pImpl_->curl, CURLOPT_TIMEOUT, 30L); // 30 second timeout for LLM
    
    CURLcode res = curl_easy_perform(pImpl_->curl);
    
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        std::cerr << "[Ollama] Request failed: " << curl_easy_strerror(res) << std::endl;
        return "";
    }
    
    return response;
}

int OllamaClient::parseResponse(const std::string& response, size_t numOptions) const {
    if (response.empty()) {
        return -1;
    }
    
    // Parse JSON response
    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errors;
    std::istringstream iss(response);
    
    if (!Json::parseFromStream(reader, iss, &root, &errors)) {
        std::cerr << "[Ollama] JSON parse error: " << errors << std::endl;
        return -1;
    }
    
    if (!root.isMember("response")) {
        std::cerr << "[Ollama] No 'response' field in JSON" << std::endl;
        return -1;
    }
    
    std::string llmResponse = root["response"].asString();
    
    // Extract first number from response
    std::regex numberRegex("\\b([0-9])\\b");
    std::smatch match;
    
    if (std::regex_search(llmResponse, match, numberRegex)) {
        int option = std::stoi(match[1].str());
        
        if (option >= 0 && option < static_cast<int>(numOptions)) {
            std::cout << "[Ollama] Selected option " << option << " from LLM response" << std::endl;
            return option;
        }
    }
    
    std::cerr << "[Ollama] Could not parse valid option from: " << llmResponse << std::endl;
    return -1;
}

int OllamaClient::selectATCMenuOption(
    const std::string& atcMessage,
    const std::vector<std::string>& menuOptions,
    const std::string& flightPhase,
    const std::string& context
) {
    if (!available_ || menuOptions.empty()) {
        return -1;
    }
    
    std::cout << "[Ollama] Querying LLM for ATC menu selection..." << std::endl;
    
    // Generate prompt
    std::string prompt = generatePrompt(atcMessage, menuOptions, flightPhase, context);
    
    // Send to Ollama
    std::string response = sendRequest(prompt);
    
    // Parse response
    return parseResponse(response, menuOptions.size());
}

} // namespace AICopilot

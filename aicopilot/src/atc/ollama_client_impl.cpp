/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Full implementation of OllamaClient with CURL and jsoncpp
* This file is only compiled when ENABLE_OLLAMA is ON
*****************************************************************************/

#ifdef ENABLE_OLLAMA

#include "ollama_client.h"
#include <curl/curl.h>
#include <json/json.h>
#include <sstream>
#include <iostream>

namespace AICopilot {

// Helper for CURL response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

class OllamaClient::Impl {
public:
    CURL* curl = nullptr;
    std::string lastError;
    
    ~Impl() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }
    
    bool initCurl() {
        if (!curl) {
            curl = curl_easy_init();
        }
        return curl != nullptr;
    }
    
    std::string makeRequest(const std::string& url, const std::string& jsonData) {
        if (!initCurl()) {
            lastError = "Failed to initialize CURL";
            return "";
        }
        
        std::string response;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        
        if (res != CURLE_OK) {
            lastError = curl_easy_strerror(res);
            return "";
        }
        
        return response;
    }
};

OllamaClient::OllamaClient() 
    : pImpl_(std::make_unique<Impl>())
    , host_("http://localhost:11434")
    , model_("llama2")
    , available_(false) {
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::cout << "OllamaClient initialized with Ollama support" << std::endl;
}

OllamaClient::~OllamaClient() {
    curl_global_cleanup();
}

bool OllamaClient::connect(const std::string& url) {
    host_ = url;
    
    // Test connection by listing models
    std::string testUrl = host_ + "/api/tags";
    std::string response = pImpl_->makeRequest(testUrl, "");
    
    if (!response.empty()) {
        available_ = true;
        std::cout << "Successfully connected to Ollama at " << host_ << std::endl;
        return true;
    }
    
    std::cout << "Failed to connect to Ollama: " << pImpl_->lastError << std::endl;
    available_ = false;
    return false;
}

void OllamaClient::setApiKey(const std::string& apiKey) {
    apiKey_ = apiKey;
}

void OllamaClient::setModel(const std::string& model) {
    model_ = model;
}

bool OllamaClient::isAvailable() const {
    return available_;
}

int OllamaClient::selectATCMenuOption(
    const std::string& situation,
    const std::vector<std::string>& options,
    const std::string& aircraftType,
    const std::string& flightPhase) {
    
    if (!available_ || options.empty()) {
        return options.empty() ? -1 : 0;
    }
    
    // Build prompt
    std::string prompt = generatePrompt(situation, options, flightPhase, aircraftType);
    
    // Create JSON request
    Json::Value root;
    root["model"] = model_;
    root["prompt"] = prompt;
    root["stream"] = false;
    
    Json::StreamWriterBuilder writer;
    std::string jsonData = Json::writeString(writer, root);
    
    // Make request
    std::string url = host_ + "/api/generate";
    std::string response = pImpl_->makeRequest(url, jsonData);
    
    if (response.empty()) {
        std::cout << "Ollama request failed, using default option" << std::endl;
        return 0;
    }
    
    // Parse response
    Json::CharReaderBuilder reader;
    Json::Value responseJson;
    std::string errors;
    std::istringstream responseStream(response);
    
    if (!Json::parseFromStream(reader, responseStream, &responseJson, &errors)) {
        std::cout << "Failed to parse Ollama response: " << errors << std::endl;
        return 0;
    }
    
    // Extract the answer
    std::string answer = responseJson["response"].asString();
    
    // Parse the number from the response
    for (char c : answer) {
        if (isdigit(c)) {
            int selection = c - '0';
            if (selection > 0 && selection <= static_cast<int>(options.size())) {
                std::cout << "Ollama selected option " << selection << ": " 
                         << options[selection - 1] << std::endl;
                return selection - 1;
            }
        }
    }
    
    // Default to first option if parsing failed
    return 0;
}

std::string OllamaClient::generatePrompt(
    const std::string& atcMessage,
    const std::vector<std::string>& menuOptions,
    const std::string& flightPhase,
    const std::string& context) const {
    
    std::ostringstream prompt;
    prompt << "You are an AI pilot assistant. You must select the most appropriate ATC response.\n\n";
    prompt << "Current situation:\n";
    prompt << "ATC Message: " << atcMessage << "\n";
    prompt << "Flight Phase: " << flightPhase << "\n";
    if (!context.empty()) {
        prompt << "Context: " << context << "\n";
    }
    prompt << "\nAvailable options:\n";
    
    for (size_t i = 0; i < menuOptions.size(); ++i) {
        prompt << (i + 1) << ". " << menuOptions[i] << "\n";
    }
    
    prompt << "\nRespond with only the number (1-" << menuOptions.size() 
           << ") of the best option. No explanation needed.";
    
    return prompt.str();
}

} // namespace AICopilot

#endif // ENABLE_OLLAMA

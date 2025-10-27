/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*****************************************************************************/

#include "voice_interface.h"
#include <algorithm>
#include <cctype>

namespace AICopilot {

bool VoiceInterface::initialize() {
    active_ = true;
    return true;
}

void VoiceInterface::shutdown() {
    active_ = false;
}

void VoiceInterface::setEnabled(bool enabled) {
    enabled_ = enabled;
}

VoiceCommand VoiceInterface::processVoiceInput(const std::string& text, double confidence) {
    if (confidence < confidenceThreshold_) {
        VoiceCommand cmd;
        cmd.type = VoiceCommandType::UNKNOWN;
        cmd.confidence = confidence;
        return cmd;
    }
    
    VoiceCommandType type = determineCommandType(text);
    VoiceCommand cmd;
    cmd.type = type;
    cmd.text = text;
    cmd.confidence = confidence;
    cmd.parameters = extractParameters(text, type);
    
    lastCommand_ = cmd;
    return cmd;
}

VoiceCommand VoiceInterface::parseATCResponse(const std::string& text) {
    return processVoiceInput(text, 1.0);
}

VoiceResponse VoiceInterface::generateResponse(const VoiceCommand& command) {
    VoiceResponse response;
    response.playAudio = true;
    response.text = "Roger";
    return response;
}

void VoiceInterface::setSTTEngine(const std::string& engineType, const std::string& endpoint) {
    sttEngine_ = engineType;
    sttEndpoint_ = endpoint;
}

VoiceCommandType VoiceInterface::determineCommandType(const std::string& text) const {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (containsATCPhrase(lower)) return VoiceCommandType::ATC_RESPONSE;
    if (containsAutopilotPhrase(lower)) return VoiceCommandType::AUTOPILOT_CONTROL;
    if (containsNavigationPhrase(lower)) return VoiceCommandType::NAVIGATION;
    if (containsEmergencyPhrase(lower)) return VoiceCommandType::EMERGENCY;
    
    return VoiceCommandType::UNKNOWN;
}

bool VoiceInterface::containsATCPhrase(const std::string& text) const {
    return text.find("cleared") != std::string::npos ||
           text.find("roger") != std::string::npos ||
           text.find("wilco") != std::string::npos;
}

bool VoiceInterface::containsAutopilotPhrase(const std::string& text) const {
    return text.find("autopilot") != std::string::npos ||
           text.find("altitude") != std::string::npos ||
           text.find("heading") != std::string::npos;
}

bool VoiceInterface::containsNavigationPhrase(const std::string& text) const {
    return text.find("direct") != std::string::npos ||
           text.find("waypoint") != std::string::npos;
}

bool VoiceInterface::containsEmergencyPhrase(const std::string& text) const {
    return text.find("mayday") != std::string::npos ||
           text.find("emergency") != std::string::npos;
}

std::map<std::string, std::string> VoiceInterface::extractParameters(
    const std::string& text,
    VoiceCommandType type) const {
    
    std::map<std::string, std::string> params;
    
    if (type == VoiceCommandType::AUTOPILOT_CONTROL) {
        double alt = extractAltitude(text);
        if (alt > 0) params["altitude"] = std::to_string(alt);
        
        double hdg = extractHeading(text);
        if (hdg >= 0) params["heading"] = std::to_string(hdg);
    }
    
    return params;
}

double VoiceInterface::extractAltitude(const std::string& text) const {
    // Simplified extraction - real implementation would use NLP
    return 0.0;
}

double VoiceInterface::extractHeading(const std::string& text) const {
    return 0.0;
}

} // namespace AICopilot

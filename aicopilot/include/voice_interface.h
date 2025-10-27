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

#ifndef VOICE_INTERFACE_H
#define VOICE_INTERFACE_H

#include "aicopilot_types.h"
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace AICopilot {

// Voice command types
enum class VoiceCommandType {
    ATC_RESPONSE,
    AUTOPILOT_CONTROL,
    NAVIGATION,
    SYSTEM_CONTROL,
    EMERGENCY,
    UNKNOWN
};

// Voice command
struct VoiceCommand {
    VoiceCommandType type;
    std::string text;
    std::map<std::string, std::string> parameters;
    double confidence;  // 0.0 to 1.0
};

// Speech-to-text callback
using SpeechCallback = std::function<void(const std::string& text, double confidence)>;

// Voice response
struct VoiceResponse {
    std::string text;
    bool playAudio;
    std::string audioFile;
};

/**
 * Voice Recognition Interface for ATC
 * Integrates with external voice recognition tools
 */
class VoiceInterface {
public:
    VoiceInterface() = default;
    
    // Initialize voice recognition
    bool initialize();
    
    // Shutdown voice recognition
    void shutdown();
    
    // Check if voice recognition is active
    bool isActive() const { return active_; }
    
    // Enable/disable voice recognition
    void setEnabled(bool enabled);
    
    // Process voice input (from external STT system)
    VoiceCommand processVoiceInput(const std::string& text, double confidence);
    
    // Parse ATC response
    VoiceCommand parseATCResponse(const std::string& text);
    
    // Parse autopilot command
    VoiceCommand parseAutopilotCommand(const std::string& text);
    
    // Parse navigation command
    VoiceCommand parseNavigationCommand(const std::string& text);
    
    // Generate voice response
    VoiceResponse generateResponse(const VoiceCommand& command);
    
    // Register speech callback
    void registerSpeechCallback(SpeechCallback callback);
    
    // Set minimum confidence threshold
    void setConfidenceThreshold(double threshold) { confidenceThreshold_ = threshold; }
    
    // Get last recognized command
    VoiceCommand getLastCommand() const { return lastCommand_; }
    
    // Voice command vocabulary for ATC
    std::vector<std::string> getATCVocabulary() const;
    
    // Integration with external voice tools
    // These methods provide hooks for external speech-to-text engines
    
    // Set speech-to-text engine endpoint (e.g., Google Cloud, Azure, Whisper)
    void setSTTEngine(const std::string& engineType, const std::string& endpoint);
    
    // Set text-to-speech engine endpoint
    void setTTSEngine(const std::string& engineType, const std::string& endpoint);
    
    // Process audio input (integration point)
    void processAudioInput(const std::vector<uint8_t>& audioData);
    
private:
    bool active_ = false;
    bool enabled_ = false;
    double confidenceThreshold_ = 0.7;
    VoiceCommand lastCommand_;
    SpeechCallback speechCallback_;
    
    std::string sttEngine_;
    std::string sttEndpoint_;
    std::string ttsEngine_;
    std::string ttsEndpoint_;
    
    // Command parsing helpers
    VoiceCommandType determineCommandType(const std::string& text) const;
    std::map<std::string, std::string> extractParameters(
        const std::string& text,
        VoiceCommandType type) const;
    
    // ATC phrase recognition
    bool containsATCPhrase(const std::string& text) const;
    bool containsAutopilotPhrase(const std::string& text) const;
    bool containsNavigationPhrase(const std::string& text) const;
    bool containsEmergencyPhrase(const std::string& text) const;
    
    // Number extraction
    double extractNumber(const std::string& text) const;
    double extractAltitude(const std::string& text) const;
    double extractHeading(const std::string& text) const;
    double extractSpeed(const std::string& text) const;
};

} // namespace AICopilot

#endif // VOICE_INTERFACE_H

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

#ifndef VOICE_OUTPUT_HPP
#define VOICE_OUTPUT_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <cstdint>

namespace AICopilot {

/**
 * Voice Output/Feedback System
 * 
 * Provides Text-to-Speech synthesis, pilot readback confirmation,
 * system status announcements, and warning alerts in synthesized voice.
 */

// Voice output types
enum class VoiceOutputType {
    COMMAND_CONFIRMATION,    // Confirmation of recognized command
    ACTION_READBACK,        // Readback of action to be executed
    STATUS_ANNOUNCEMENT,    // Current system status
    WARNING_ALERT,          // Warning or alert
    INFORMATION,            // General information
    QUERY_RESPONSE,         // Response to query
    ERROR_MESSAGE,          // Error notification
    HELP_TEXT               // Help information
};

// Speaker profiles (voice characteristics)
enum class SpeakerProfile {
    MALE_PROFESSIONAL,      // Professional male voice
    FEMALE_PROFESSIONAL,    // Professional female voice
    STANDARD,               // Standard synthesized voice
    CLEAR_ENUNCIATION,      // Slow, clear speech
    NEUTRAL                 // Neutral voice
};

// Text-to-Speech configuration
struct TTSConfig {
    SpeakerProfile speaker;
    float speech_rate = 1.0f;      // 0.5 (slow) to 2.0 (fast)
    float pitch = 1.0f;            // 0.5 (low) to 2.0 (high)
    float volume = 0.8f;           // 0.0 to 1.0
    bool add_pause_after = true;   // Add pause after utterance
    float pause_duration_ms = 500; // Pause duration
};

// Synthesized speech output
struct SynthesizedSpeech {
    std::string text;
    std::vector<int16_t> audio_samples;  // PCM audio
    uint32_t duration_ms;
    VoiceOutputType output_type;
    bool is_critical;  // Critical alert (always play)
};

/**
 * SpeechSynthesizer - Text-to-Speech synthesis
 */
class SpeechSynthesizer {
public:
    SpeechSynthesizer();
    ~SpeechSynthesizer();
    
    /**
     * Initialize TTS engine
     */
    bool initialize(const TTSConfig& config = TTSConfig());
    
    /**
     * Synthesize text to speech
     */
    SynthesizedSpeech synthesize(const std::string& text,
                               VoiceOutputType output_type = VoiceOutputType::INFORMATION);
    
    /**
     * Configure TTS parameters
     */
    void configure(const TTSConfig& config);
    
    /**
     * Get current configuration
     */
    TTSConfig getConfig() const { return config_; }
    
    /**
     * Set speaker profile
     */
    void setSpeakerProfile(SpeakerProfile profile);
    
    /**
     * Set speech rate (0.5 to 2.0)
     */
    void setSpeechRate(float rate);
    
    /**
     * Set volume (0.0 to 1.0)
     */
    void setVolume(float volume);
    
    /**
     * Check if TTS is ready
     */
    bool isReady() const { return is_ready_; }
    
private:
    TTSConfig config_;
    bool is_ready_;
    
    // Simple speech synthesis (phoneme-based for no external API requirement)
    std::vector<int16_t> generateAudioSamples(const std::string& text);
    uint32_t estimateDuration(const std::string& text) const;
};

/**
 * ReadbackGenerator - Generates pilot readback confirmations
 */
class ReadbackGenerator {
public:
    ReadbackGenerator();
    
    /**
     * Generate command confirmation readback
     */
    std::string generateCommandConfirmation(const std::string& command);
    
    /**
     * Generate action readback
     */
    std::string generateActionReadback(const std::string& action_description);
    
    /**
     * Generate parameter confirmation
     */
    std::string generateParameterConfirmation(
        const std::string& param_name,
        const std::string& param_value);
    
    /**
     * Generate altitude readback
     */
    std::string generateAltitudeReadback(double altitude_feet);
    
    /**
     * Generate heading readback
     */
    std::string generateHeadingReadback(double heading_degrees);
    
    /**
     * Generate speed readback
     */
    std::string generateSpeedReadback(double speed_knots);
    
    /**
     * Generate waypoint readback
     */
    std::string generateWaypointReadback(const std::string& waypoint_id);
    
    /**
     * Generate standardized readback format
     */
    std::string formatStandardReadback(
        const std::map<std::string, std::string>& parameters);
    
private:
    // Phonetic alphabet for spelling
    std::map<char, std::string> phonetic_alphabet_;
    
    std::string numberToWords(double number) const;
    std::string degreeToHeading(double degrees) const;
    std::string formatAltitude(double feet) const;
};

/**
 * AnnouncementGenerator - System status and warning announcements
 */
class AnnouncementGenerator {
public:
    AnnouncementGenerator();
    
    /**
     * Generate status announcement
     */
    std::string generateStatusAnnouncement(
        const std::string& status_type,
        const std::map<std::string, std::string>& status_data);
    
    /**
     * Generate weather announcement
     */
    std::string generateWeatherAnnouncement(
        const std::string& airport,
        const std::string& conditions);
    
    /**
     * Generate terrain warning
     */
    std::string generateTerrainWarning(double distance_nm, double altitude_diff);
    
    /**
     * Generate system warning
     */
    std::string generateSystemWarning(const std::string& system, 
                                     const std::string& issue);
    
    /**
     * Generate flight phase announcement
     */
    std::string generateFlightPhaseAnnouncement(const std::string& phase);
    
    /**
     * Generate checklist announcement
     */
    std::string generateChecklistAnnouncement(const std::string& checklist_name);
    
    /**
     * Generate navigation status
     */
    std::string generateNavigationStatus(
        const std::string& current_waypoint,
        double distance_remaining);
    
private:
    std::string formatWeatherData(const std::string& conditions) const;
    std::string formatSystemData(const std::string& system_info) const;
};

/**
 * VoiceOutput - Main voice output system
 */
class VoiceOutput {
public:
    VoiceOutput();
    ~VoiceOutput();
    
    /**
     * Initialize voice output system
     */
    bool initialize();
    
    /**
     * Shutdown voice output system
     */
    void shutdown();
    
    /**
     * Play command confirmation
     */
    void playCommandConfirmation(const std::string& command);
    
    /**
     * Play action readback
     */
    void playActionReadback(const std::string& action_description);
    
    /**
     * Play status announcement
     */
    void playStatusAnnouncement(const std::string& status);
    
    /**
     * Play warning alert (critical, always plays)
     */
    void playWarningAlert(const std::string& warning_text);
    
    /**
     * Play information message
     */
    void playInformation(const std::string& info_text);
    
    /**
     * Play query response
     */
    void playQueryResponse(const std::string& response);
    
    /**
     * Play synthesized speech directly
     */
    void playSynthesizedSpeech(const SynthesizedSpeech& speech);
    
    /**
     * Stop current playback
     */
    void stopPlayback();
    
    /**
     * Check if audio is currently playing
     */
    bool isPlaying() const { return is_playing_; }
    
    /**
     * Set TTS configuration
     */
    void setTTSConfig(const TTSConfig& config);
    
    /**
     * Register audio output callback (for external audio handling)
     */
    using AudioCallback = std::function<void(const SynthesizedSpeech&)>;
    void registerAudioCallback(AudioCallback callback);
    
    /**
     * Get available speaker profiles
     */
    std::vector<SpeakerProfile> getAvailableSpeakers() const;
    
    /**
     * Set active speaker profile
     */
    void setActiveSpeaker(SpeakerProfile profile);
    
    /**
     * Enable/disable voice output
     */
    void setEnabled(bool enabled) { is_enabled_ = enabled; }
    
    /**
     * Check if voice output is enabled
     */
    bool isEnabled() const { return is_enabled_; }
    
private:
    std::unique_ptr<SpeechSynthesizer> synthesizer_;
    std::unique_ptr<ReadbackGenerator> readback_generator_;
    std::unique_ptr<AnnouncementGenerator> announcement_generator_;
    
    AudioCallback audio_callback_;
    bool is_playing_;
    bool is_enabled_;
    
    // Queue for announcements
    std::vector<SynthesizedSpeech> announcement_queue_;
    
    void processAnnouncement(const SynthesizedSpeech& speech);
};

} // namespace AICopilot

#endif // VOICE_OUTPUT_HPP

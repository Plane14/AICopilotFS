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

#include "voice_output.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace AICopilot {

// ============================================================================
// SpeechSynthesizer Implementation
// ============================================================================

SpeechSynthesizer::SpeechSynthesizer()
    : is_ready_(false) {
}

SpeechSynthesizer::~SpeechSynthesizer() {
}

bool SpeechSynthesizer::initialize(const TTSConfig& config) {
    config_ = config;
    is_ready_ = true;
    return true;
}

SynthesizedSpeech SpeechSynthesizer::synthesize(
    const std::string& text,
    VoiceOutputType output_type) {
    
    SynthesizedSpeech speech;
    speech.text = text;
    speech.output_type = output_type;
    speech.is_critical = (output_type == VoiceOutputType::WARNING_ALERT ||
                         output_type == VoiceOutputType::ERROR_MESSAGE);
    
    // Generate audio samples
    speech.audio_samples = generateAudioSamples(text);
    speech.duration_ms = estimateDuration(text);
    
    return speech;
}

void SpeechSynthesizer::configure(const TTSConfig& config) {
    config_ = config;
}

void SpeechSynthesizer::setSpeakerProfile(SpeakerProfile profile) {
    config_.speaker = profile;
}

void SpeechSynthesizer::setSpeechRate(float rate) {
    config_.speech_rate = std::max(0.5f, std::min(2.0f, rate));
}

void SpeechSynthesizer::setVolume(float volume) {
    config_.volume = std::max(0.0f, std::min(1.0f, volume));
}

std::vector<int16_t> SpeechSynthesizer::generateAudioSamples(
    const std::string& text) {
    
    std::vector<int16_t> samples;
    
    // Generate a simple audio representation
    // In a real implementation, this would use a TTS engine
    const int sample_rate = 16000;
    const float duration_ms = (text.length() / 5.0f) * 1000;  // ~5 chars per second
    const int num_samples = static_cast<int>(sample_rate * duration_ms / 1000.0f);
    
    // Generate tone-based representation (simple beep pattern)
    float frequency = 440.0f + (text.length() * 10);  // Vary frequency by text length
    frequency = std::min(frequency, 3000.0f);  // Cap at 3kHz
    
    for (int i = 0; i < num_samples; ++i) {
        float t = (float)i / sample_rate;
        float sample = std::sin(2 * 3.14159f * frequency * t);
        sample *= config_.volume;
        
        // Apply envelope to avoid clicks
        float envelope = 1.0f;
        if (i < sample_rate / 100) {  // 10ms attack
            envelope = (float)i / (sample_rate / 100);
        } else if (i > num_samples - sample_rate / 100) {  // 10ms release
            envelope = (float)(num_samples - i) / (sample_rate / 100);
        }
        
        sample *= envelope;
        samples.push_back(static_cast<int16_t>(sample * 30000));
    }
    
    return samples;
}

uint32_t SpeechSynthesizer::estimateDuration(const std::string& text) const {
    // Estimate: ~5 characters per second at normal speed
    float chars_per_second = 5.0f * config_.speech_rate;
    float duration_seconds = text.length() / chars_per_second;
    
    return static_cast<uint32_t>(duration_seconds * 1000);
}

// ============================================================================
// ReadbackGenerator Implementation
// ============================================================================

ReadbackGenerator::ReadbackGenerator() {
    // Initialize phonetic alphabet
    phonetic_alphabet_ = {
        {'A', "Alpha"}, {'B', "Bravo"}, {'C', "Charlie"}, {'D', "Delta"},
        {'E', "Echo"}, {'F', "Foxtrot"}, {'G', "Golf"}, {'H', "Hotel"},
        {'I', "India"}, {'J', "Juliett"}, {'K', "Kilo"}, {'L', "Lima"},
        {'M', "Mike"}, {'N', "November"}, {'O', "Oscar"}, {'P', "Papa"},
        {'Q', "Quebec"}, {'R', "Romeo"}, {'S', "Sierra"}, {'T', "Tango"},
        {'U', "Uniform"}, {'V', "Victor"}, {'W', "Whiskey"}, {'X', "X-ray"},
        {'Y', "Yankee"}, {'Z', "Zulu"}
    };
}

std::string ReadbackGenerator::generateCommandConfirmation(
    const std::string& command) {
    
    return "Roger, " + command + " acknowledged.";
}

std::string ReadbackGenerator::generateActionReadback(
    const std::string& action_description) {
    
    return "Roger, " + action_description + ".";
}

std::string ReadbackGenerator::generateParameterConfirmation(
    const std::string& param_name,
    const std::string& param_value) {
    
    std::stringstream ss;
    ss << "Confirming " << param_name << ": " << param_value << ".";
    return ss.str();
}

std::string ReadbackGenerator::generateAltitudeReadback(double altitude_feet) {
    std::stringstream ss;
    
    if (altitude_feet >= 10000) {
        int flight_level = static_cast<int>(altitude_feet) / 100;
        ss << "Flight level " << flight_level << ".";
    } else {
        ss << std::fixed << std::setprecision(0) << altitude_feet << " feet.";
    }
    
    return ss.str();
}

std::string ReadbackGenerator::generateHeadingReadback(double heading_degrees) {
    std::stringstream ss;
    
    ss << "Heading " << std::fixed << std::setprecision(0) << heading_degrees << " degrees.";
    return ss.str();
}

std::string ReadbackGenerator::generateSpeedReadback(double speed_knots) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(0) << speed_knots << " knots.";
    return ss.str();
}

std::string ReadbackGenerator::generateWaypointReadback(const std::string& waypoint_id) {
    std::stringstream ss;
    ss << "Direct to ";
    
    for (char c : waypoint_id) {
        auto it = phonetic_alphabet_.find(std::toupper(c));
        if (it != phonetic_alphabet_.end()) {
            ss << it->second << " ";
        }
    }
    
    ss << ".";
    return ss.str();
}

std::string ReadbackGenerator::formatStandardReadback(
    const std::map<std::string, std::string>& parameters) {
    
    std::stringstream ss;
    ss << "Roger, ";
    
    bool first = true;
    for (const auto& [key, value] : parameters) {
        if (!first) ss << ", ";
        
        if (key == "altitude") {
            ss << generateAltitudeReadback(std::stod(value));
        } else if (key == "heading") {
            ss << generateHeadingReadback(std::stod(value));
        } else if (key == "speed") {
            ss << generateSpeedReadback(std::stod(value));
        } else if (key == "waypoint") {
            ss << generateWaypointReadback(value);
        } else {
            ss << key << " " << value;
        }
        
        first = false;
    }
    
    ss << ".";
    return ss.str();
}

std::string ReadbackGenerator::numberToWords(double number) const {
    static const std::string ones[] = {
        "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"
    };
    
    int int_num = static_cast<int>(number);
    if (int_num >= 0 && int_num < 10) {
        return ones[int_num];
    }
    
    return "";
}

std::string ReadbackGenerator::degreeToHeading(double degrees) const {
    static const std::map<int, std::string> headings = {
        {0, "North"}, {45, "Northeast"}, {90, "East"},
        {135, "Southeast"}, {180, "South"}, {225, "Southwest"},
        {270, "West"}, {315, "Northwest"}
    };
    
    int rounded = static_cast<int>(degrees + 22.5) / 45 * 45;
    rounded = rounded % 360;
    
    auto it = headings.find(rounded);
    if (it != headings.end()) {
        return it->second;
    }
    
    return "";
}

std::string ReadbackGenerator::formatAltitude(double feet) const {
    std::stringstream ss;
    
    if (feet >= 10000) {
        int flight_level = static_cast<int>(feet) / 100;
        ss << "Flight level " << flight_level;
    } else {
        ss << std::fixed << std::setprecision(0) << feet << " feet";
    }
    
    return ss.str();
}

// ============================================================================
// AnnouncementGenerator Implementation
// ============================================================================

AnnouncementGenerator::AnnouncementGenerator() {
}

std::string AnnouncementGenerator::generateStatusAnnouncement(
    const std::string& status_type,
    const std::map<std::string, std::string>& status_data) {
    
    std::stringstream ss;
    ss << status_type << " status: ";
    
    for (const auto& [key, value] : status_data) {
        ss << key << " " << value << ". ";
    }
    
    return ss.str();
}

std::string AnnouncementGenerator::generateWeatherAnnouncement(
    const std::string& airport,
    const std::string& conditions) {
    
    std::stringstream ss;
    ss << "Weather for " << airport << ": " << formatWeatherData(conditions) << ".";
    return ss.str();
}

std::string AnnouncementGenerator::generateTerrainWarning(
    double distance_nm,
    double altitude_diff) {
    
    std::stringstream ss;
    ss << "Terrain ahead, ";
    ss << std::fixed << std::setprecision(1) << distance_nm << " nautical miles, ";
    ss << std::fixed << std::setprecision(0) << altitude_diff << " feet below.";
    return ss.str();
}

std::string AnnouncementGenerator::generateSystemWarning(
    const std::string& system,
    const std::string& issue) {
    
    std::stringstream ss;
    ss << "Warning: " << system << " " << issue << ".";
    return ss.str();
}

std::string AnnouncementGenerator::generateFlightPhaseAnnouncement(
    const std::string& phase) {
    
    return "Flight phase: " + phase + ".";
}

std::string AnnouncementGenerator::generateChecklistAnnouncement(
    const std::string& checklist_name) {
    
    return "Starting " + checklist_name + " checklist.";
}

std::string AnnouncementGenerator::generateNavigationStatus(
    const std::string& current_waypoint,
    double distance_remaining) {
    
    std::stringstream ss;
    ss << "Current waypoint " << current_waypoint << ", ";
    ss << std::fixed << std::setprecision(1) << distance_remaining << " nautical miles remaining.";
    return ss.str();
}

std::string AnnouncementGenerator::formatWeatherData(const std::string& conditions) const {
    // Parse and format weather data
    return conditions;
}

std::string AnnouncementGenerator::formatSystemData(const std::string& system_info) const {
    return system_info;
}

// ============================================================================
// VoiceOutput Implementation
// ============================================================================

VoiceOutput::VoiceOutput()
    : is_playing_(false), is_enabled_(true) {
    
    synthesizer_ = std::make_unique<SpeechSynthesizer>();
    readback_generator_ = std::make_unique<ReadbackGenerator>();
    announcement_generator_ = std::make_unique<AnnouncementGenerator>();
}

VoiceOutput::~VoiceOutput() {
    shutdown();
}

bool VoiceOutput::initialize() {
    TTSConfig config;
    config.speaker = SpeakerProfile::CLEAR_ENUNCIATION;
    config.speech_rate = 0.9f;
    config.volume = 0.8f;
    
    return synthesizer_->initialize(config);
}

void VoiceOutput::shutdown() {
    stopPlayback();
}

void VoiceOutput::playCommandConfirmation(const std::string& command) {
    if (!is_enabled_) return;
    
    std::string readback = readback_generator_->generateCommandConfirmation(command);
    SynthesizedSpeech speech = synthesizer_->synthesize(
        readback, VoiceOutputType::COMMAND_CONFIRMATION);
    
    playSynthesizedSpeech(speech);
}

void VoiceOutput::playActionReadback(const std::string& action_description) {
    if (!is_enabled_) return;
    
    std::string readback = readback_generator_->generateActionReadback(action_description);
    SynthesizedSpeech speech = synthesizer_->synthesize(
        readback, VoiceOutputType::ACTION_READBACK);
    
    playSynthesizedSpeech(speech);
}

void VoiceOutput::playStatusAnnouncement(const std::string& status) {
    if (!is_enabled_) return;
    
    SynthesizedSpeech speech = synthesizer_->synthesize(
        status, VoiceOutputType::STATUS_ANNOUNCEMENT);
    
    playSynthesizedSpeech(speech);
}

void VoiceOutput::playWarningAlert(const std::string& warning_text) {
    // Always play warnings, regardless of enable state
    SynthesizedSpeech speech = synthesizer_->synthesize(
        warning_text, VoiceOutputType::WARNING_ALERT);
    speech.is_critical = true;
    
    playSynthesizedSpeech(speech);
}

void VoiceOutput::playInformation(const std::string& info_text) {
    if (!is_enabled_) return;
    
    SynthesizedSpeech speech = synthesizer_->synthesize(
        info_text, VoiceOutputType::INFORMATION);
    
    playSynthesizedSpeech(speech);
}

void VoiceOutput::playQueryResponse(const std::string& response) {
    if (!is_enabled_) return;
    
    SynthesizedSpeech speech = synthesizer_->synthesize(
        response, VoiceOutputType::QUERY_RESPONSE);
    
    playSynthesizedSpeech(speech);
}

void VoiceOutput::playSynthesizedSpeech(const SynthesizedSpeech& speech) {
    is_playing_ = true;
    
    if (audio_callback_) {
        audio_callback_(speech);
    }
    
    processAnnouncement(speech);
}

void VoiceOutput::stopPlayback() {
    is_playing_ = false;
}

void VoiceOutput::setTTSConfig(const TTSConfig& config) {
    if (synthesizer_) {
        synthesizer_->configure(config);
    }
}

void VoiceOutput::registerAudioCallback(AudioCallback callback) {
    audio_callback_ = callback;
}

std::vector<SpeakerProfile> VoiceOutput::getAvailableSpeakers() const {
    return {
        SpeakerProfile::MALE_PROFESSIONAL,
        SpeakerProfile::FEMALE_PROFESSIONAL,
        SpeakerProfile::STANDARD,
        SpeakerProfile::CLEAR_ENUNCIATION,
        SpeakerProfile::NEUTRAL
    };
}

void VoiceOutput::setActiveSpeaker(SpeakerProfile profile) {
    if (synthesizer_) {
        synthesizer_->setSpeakerProfile(profile);
    }
}

void VoiceOutput::processAnnouncement(const SynthesizedSpeech& speech) {
    announcement_queue_.push_back(speech);
    
    // Keep queue size reasonable
    if (announcement_queue_.size() > 10) {
        announcement_queue_.erase(announcement_queue_.begin());
    }
}

} // namespace AICopilot

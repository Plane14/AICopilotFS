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

#ifndef SPEECH_RECOGNIZER_HPP
#define SPEECH_RECOGNIZER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace AICopilot {

/**
 * Speech Recognition System
 * 
 * Recognizes pilot voice commands using phonetic matching,
 * confidence scoring, and context-aware recognition.
 * 50+ commands supported covering navigation, runway,
 * flight systems, status queries, and emergency operations.
 */

// Command recognition result
struct RecognitionResult {
    std::string command;        // Recognized command
    std::string raw_text;       // Raw spoken text
    double confidence;          // 0.0 to 1.0
    std::map<std::string, std::string> parameters;
    std::string interpretation;
};

// Command definition
struct CommandDefinition {
    std::string command_id;
    std::string display_name;
    std::vector<std::string> variations;  // Alternative phrasings
    std::map<std::string, std::string> parameters;
    bool requires_confirmation;
};

// Voice command categories
enum class CommandCategory {
    NAVIGATION,
    RUNWAY_APPROACH,
    FLIGHT_SYSTEMS,
    STATUS_QUERY,
    EMERGENCY,
    UNKNOWN
};

/**
 * PhoneticMatcher - Matches spoken text to known commands
 */
class PhoneticMatcher {
public:
    PhoneticMatcher();
    
    /**
     * Calculate phonetic similarity (0.0-1.0)
     * Uses Levenshtein distance with phonetic weighting
     */
    double calculateSimilarity(const std::string& spoken, 
                             const std::string& expected) const;
    
    /**
     * Normalize spoken text for matching
     */
    std::string normalizeText(const std::string& text) const;
    
    /**
     * Find best matching command from candidates
     */
    std::string findBestMatch(const std::string& spoken,
                            const std::vector<std::string>& candidates) const;
    
    /**
     * Apply phonetic corrections
     */
    std::string applyPhoneticCorrections(const std::string& text) const;
    
private:
    // Phonetic mappings for common speech variations
    std::map<std::string, std::string> phonetic_replacements_;
    
    int levenshteinDistance(const std::string& s1, const std::string& s2) const;
    std::string toLower(const std::string& text) const;
};

/**
 * CommandRecognizer - Recognizes and validates spoken commands
 */
class CommandRecognizer {
public:
    CommandRecognizer();
    
    /**
     * Initialize command database
     */
    bool initialize();
    
    /**
     * Register a command definition
     */
    void registerCommand(const CommandDefinition& command);
    
    /**
     * Recognize spoken text as command
     */
    RecognitionResult recognize(const std::string& spoken_text);
    
    /**
     * Get command by ID
     */
    CommandDefinition getCommand(const std::string& command_id) const;
    
    /**
     * Get all commands in category
     */
    std::vector<CommandDefinition> getCommandsInCategory(CommandCategory category) const;
    
    /**
     * Get all registered commands
     */
    std::vector<CommandDefinition> getAllCommands() const;
    
    /**
     * Set confidence threshold for command acceptance
     */
    void setConfidenceThreshold(double threshold) { confidence_threshold_ = threshold; }
    
    /**
     * Get confidence threshold
     */
    double getConfidenceThreshold() const { return confidence_threshold_; }
    
    /**
     * Get command recognition statistics
     */
    struct RecognitionStats {
        uint32_t total_attempts = 0;
        uint32_t successful_recognitions = 0;
        double average_confidence = 0.0;
        uint32_t most_common_command_count = 0;
    };
    RecognitionStats getStatistics() const;
    
private:
    std::map<std::string, CommandDefinition> commands_;
    std::unique_ptr<PhoneticMatcher> phonetic_matcher_;
    double confidence_threshold_;
    
    RecognitionStats statistics_;
    std::map<std::string, uint32_t> command_usage_count_;
    
    // Command database initialization
    void initializeNavigationCommands();
    void initializeRunwayCommands();
    void initializeFlightSystemCommands();
    void initializeStatusQueryCommands();
    void initializeEmergencyCommands();
    
    // Helper methods
    CommandCategory determineCategory(const std::string& command_id) const;
    double calculateFinalConfidence(double base_confidence, 
                                   const std::string& context) const;
};

/**
 * ParameterExtractor - Extracts parameters from recognized commands
 */
class ParameterExtractor {
public:
    ParameterExtractor();
    
    /**
     * Extract numeric parameter (altitude, heading, speed)
     */
    double extractNumericParameter(const std::string& text, 
                                  const std::string& param_type);
    
    /**
     * Extract waypoint/airport identifier
     */
    std::string extractIdentifier(const std::string& text);
    
    /**
     * Extract direction/bearing
     */
    std::string extractDirection(const std::string& text);
    
    /**
     * Extract altitude in various formats (feet, FL, meters)
     */
    double extractAltitude(const std::string& text);
    
    /**
     * Extract speed in various formats (knots, mph, kph)
     */
    double extractSpeed(const std::string& text);
    
    /**
     * Extract heading (0-360 degrees)
     */
    double extractHeading(const std::string& text);
    
    /**
     * Extract time duration
     */
    double extractDuration(const std::string& text);  // Returns seconds
    
    /**
     * Extract comma-separated list of parameters
     */
    std::vector<std::string> extractList(const std::string& text);
    
private:
    // Number word mappings
    std::map<std::string, double> number_words_;
    
    // Direction mappings
    std::map<std::string, double> direction_headings_;
    
    // Helper methods
    bool isNumericWord(const std::string& word) const;
    double wordToNumber(const std::string& word) const;
    std::vector<std::string> tokenize(const std::string& text) const;
};

/**
 * SpeechRecognizer - Main speech recognition system
 */
class SpeechRecognizer {
public:
    SpeechRecognizer();
    ~SpeechRecognizer();
    
    /**
     * Initialize speech recognizer
     */
    bool initialize();
    
    /**
     * Shutdown speech recognizer
     */
    void shutdown();
    
    /**
     * Recognize speech from audio buffer
     */
    RecognitionResult recognizeSpeech(const std::vector<int16_t>& audio_buffer);
    
    /**
     * Recognize from text (for testing or STT integration)
     */
    RecognitionResult recognizeText(const std::string& text);
    
    /**
     * Get last recognition result
     */
    RecognitionResult getLastResult() const { return last_result_; }
    
    /**
     * Check if command requires confirmation
     */
    bool requiresConfirmation(const std::string& command_id) const;
    
    /**
     * Get list of all available commands
     */
    std::vector<std::string> getAvailableCommands() const;
    
    /**
     * Get detailed command help
     */
    std::string getCommandHelp(const std::string& command_id) const;
    
    /**
     * Set acoustic environment (quiet, moderate, loud cockpit)
     */
    void setEnvironment(const std::string& environment);
    
    /**
     * Get recognition statistics
     */
    CommandRecognizer::RecognitionStats getStatistics() const;
    
    /**
     * Set global confidence threshold
     */
    void setConfidenceThreshold(double threshold);
    
private:
    std::unique_ptr<CommandRecognizer> command_recognizer_;
    std::unique_ptr<ParameterExtractor> parameter_extractor_;
    
    RecognitionResult last_result_;
    std::string current_environment_;
};

} // namespace AICopilot

#endif // SPEECH_RECOGNIZER_HPP

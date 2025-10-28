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

#include "speech_recognizer.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cmath>

namespace AICopilot {

// ============================================================================
// PhoneticMatcher Implementation
// ============================================================================

PhoneticMatcher::PhoneticMatcher() {
    // Initialize phonetic replacements for common variations
    phonetic_replacements_ = {
        {"theta", "the"},
        {"to", "two"},
        {"for", "four"},
        {"there", "their"},
        {"where", "wear"},
    };
}

double PhoneticMatcher::calculateSimilarity(const std::string& spoken,
                                          const std::string& expected) const {
    std::string s1 = toLower(normalizeText(spoken));
    std::string s2 = toLower(normalizeText(expected));
    
    int dist = levenshteinDistance(s1, s2);
    int max_len = std::max(s1.length(), s2.length());
    
    if (max_len == 0) return 1.0;
    
    double similarity = 1.0 - (double)dist / max_len;
    return std::max(0.0, std::min(1.0, similarity));
}

std::string PhoneticMatcher::normalizeText(const std::string& text) const {
    std::string result = toLower(text);
    
    // Remove punctuation and extra spaces
    result.erase(std::remove_if(result.begin(), result.end(),
                               [](unsigned char c) { return !std::isalnum(c) && !std::isspace(c); }),
                result.end());
    
    // Collapse multiple spaces
    size_t pos = 0;
    while ((pos = result.find("  ", pos)) != std::string::npos) {
        result.replace(pos, 2, " ");
    }
    
    // Trim whitespace
    size_t start = result.find_first_not_of(" \t\r\n");
    size_t end = result.find_last_not_of(" \t\r\n");
    if (start != std::string::npos) {
        result = result.substr(start, end - start + 1);
    }
    
    return result;
}

std::string PhoneticMatcher::findBestMatch(
    const std::string& spoken,
    const std::vector<std::string>& candidates) const {
    
    if (candidates.empty()) return "";
    
    double best_similarity = 0.0;
    std::string best_match = candidates[0];
    
    for (const auto& candidate : candidates) {
        double similarity = calculateSimilarity(spoken, candidate);
        if (similarity > best_similarity) {
            best_similarity = similarity;
            best_match = candidate;
        }
    }
    
    return best_match;
}

std::string PhoneticMatcher::applyPhoneticCorrections(const std::string& text) const {
    std::string result = toLower(text);
    
    for (const auto& [bad, good] : phonetic_replacements_) {
        size_t pos = 0;
        while ((pos = result.find(bad, pos)) != std::string::npos) {
            result.replace(pos, bad.length(), good);
            pos += good.length();
        }
    }
    
    return result;
}

int PhoneticMatcher::levenshteinDistance(const std::string& s1,
                                       const std::string& s2) const {
    size_t len1 = s1.length();
    size_t len2 = s2.length();
    
    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));
    
    for (size_t i = 0; i <= len1; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= len2; ++j) dp[0][j] = j;
    
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            if (s1[i-1] == s2[j-1]) {
                dp[i][j] = dp[i-1][j-1];
            } else {
                dp[i][j] = 1 + std::min({dp[i-1][j], dp[i][j-1], dp[i-1][j-1]});
            }
        }
    }
    
    return dp[len1][len2];
}

std::string PhoneticMatcher::toLower(const std::string& text) const {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

// ============================================================================
// CommandRecognizer Implementation
// ============================================================================

CommandRecognizer::CommandRecognizer()
    : confidence_threshold_(0.85) {
    phonetic_matcher_ = std::make_unique<PhoneticMatcher>();
}

bool CommandRecognizer::initialize() {
    initializeNavigationCommands();
    initializeRunwayCommands();
    initializeFlightSystemCommands();
    initializeStatusQueryCommands();
    initializeEmergencyCommands();
    
    return true;
}

void CommandRecognizer::registerCommand(const CommandDefinition& command) {
    commands_[command.command_id] = command;
}

RecognitionResult CommandRecognizer::recognize(const std::string& spoken_text) {
    RecognitionResult result;
    result.raw_text = spoken_text;
    result.confidence = 0.0;
    result.command = "UNKNOWN";
    
    statistics_.total_attempts++;
    
    if (spoken_text.empty()) {
        return result;
    }
    
    std::string normalized = phonetic_matcher_->normalizeText(spoken_text);
    double best_confidence = 0.0;
    std::string best_command;
    
    // Try to match against all known command variations
    for (const auto& [cmd_id, cmd_def] : commands_) {
        for (const auto& variation : cmd_def.variations) {
            double similarity = phonetic_matcher_->calculateSimilarity(
                normalized, variation);
            
            if (similarity > best_confidence) {
                best_confidence = similarity;
                best_command = cmd_id;
            }
        }
    }
    
    // Check if confidence meets threshold
    if (best_confidence >= confidence_threshold_) {
        result.command = best_command;
        result.confidence = best_confidence;
        result.interpretation = best_command;
        
        statistics_.successful_recognitions++;
        command_usage_count_[best_command]++;
        
        if (statistics_.total_attempts > 0) {
            statistics_.average_confidence = 
                (statistics_.average_confidence * (statistics_.total_attempts - 1) + 
                 best_confidence) / statistics_.total_attempts;
        }
        
        // Find most common command
        if (!command_usage_count_.empty()) {
            auto max_it = std::max_element(
                command_usage_count_.begin(), command_usage_count_.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            statistics_.most_common_command_count = max_it->second;
        }
    }
    
    return result;
}

CommandDefinition CommandRecognizer::getCommand(const std::string& command_id) const {
    auto it = commands_.find(command_id);
    if (it != commands_.end()) {
        return it->second;
    }
    return CommandDefinition{};
}

std::vector<CommandDefinition> CommandRecognizer::getCommandsInCategory(
    CommandCategory category) const {
    
    std::vector<CommandDefinition> result;
    
    for (const auto& [cmd_id, cmd_def] : commands_) {
        if (determineCategory(cmd_id) == category) {
            result.push_back(cmd_def);
        }
    }
    
    return result;
}

std::vector<CommandDefinition> CommandRecognizer::getAllCommands() const {
    std::vector<CommandDefinition> result;
    for (const auto& [cmd_id, cmd_def] : commands_) {
        result.push_back(cmd_def);
    }
    return result;
}

CommandRecognizer::RecognitionStats CommandRecognizer::getStatistics() const {
    return statistics_;
}

void CommandRecognizer::initializeNavigationCommands() {
    registerCommand({
        "SET_ALTITUDE",
        "Set Altitude",
        {"set altitude to", "climb to", "descend to", "maintain altitude"},
        {{"altitude", "number"}},
        false
    });
    
    registerCommand({
        "DIRECT_TO_WAYPOINT",
        "Direct to Waypoint",
        {"direct to", "go to", "navigate to", "fly to"},
        {{"waypoint", "identifier"}},
        false
    });
    
    registerCommand({
        "NAVIGATE_AIRWAY",
        "Navigate Airway",
        {"navigate airways", "fly airway", "maintain airway"},
        {{"airway", "name"}},
        false
    });
    
    registerCommand({
        "SET_HEADING",
        "Set Heading",
        {"set heading", "turn to", "heading"},
        {{"heading", "degrees"}},
        false
    });
    
    registerCommand({
        "SET_SPEED",
        "Set Speed",
        {"set speed", "maintain speed", "reduce speed", "increase speed"},
        {{"speed", "knots"}},
        false
    });
}

void CommandRecognizer::initializeRunwayCommands() {
    registerCommand({
        "FIND_BEST_RUNWAY",
        "Find Best Runway",
        {"find best runway", "best runway", "recommend runway"},
        {},
        false
    });
    
    registerCommand({
        "REQUEST_RUNWAY_INFO",
        "Request Runway Information",
        {"request runway", "runway information", "runway details"},
        {{"runway", "identifier"}},
        false
    });
    
    registerCommand({
        "APPROACH_FROM_DIRECTION",
        "Approach from Direction",
        {"approach from", "land from", "come in from"},
        {{"direction", "bearing"}},
        false
    });
    
    registerCommand({
        "CHECK_LANDING_DISTANCE",
        "Check Landing Distance",
        {"check landing distance", "landing distance", "runway length"},
        {},
        false
    });
}

void CommandRecognizer::initializeFlightSystemCommands() {
    registerCommand({
        "PREFLIGHT_CHECKLIST",
        "Preflight Checklist",
        {"preflight checklist", "start preflight", "run preflight"},
        {},
        true
    });
    
    registerCommand({
        "WEIGHT_BALANCE_CHECK",
        "Weight and Balance Check",
        {"weight and balance", "check weight", "cg check"},
        {},
        false
    });
    
    registerCommand({
        "CALCULATE_VSPEEDS",
        "Calculate V-Speeds",
        {"calculate v speeds", "compute v speeds", "v one"},
        {},
        false
    });
    
    registerCommand({
        "STABILIZED_APPROACH_CHECK",
        "Stabilized Approach Check",
        {"stabilized approach", "approach check", "stable approach"},
        {},
        false
    });
}

void CommandRecognizer::initializeStatusQueryCommands() {
    registerCommand({
        "CURRENT_WEATHER",
        "Current Weather",
        {"current weather", "weather report", "metar"},
        {},
        false
    });
    
    registerCommand({
        "TERRAIN_AHEAD",
        "Terrain Ahead",
        {"terrain ahead", "check terrain", "terrain report"},
        {},
        false
    });
    
    registerCommand({
        "NAVIGATION_STATUS",
        "Navigation Status",
        {"navigation status", "nav status", "position report"},
        {},
        false
    });
    
    registerCommand({
        "SYSTEM_STATUS",
        "System Status",
        {"system status", "status report", "systems check"},
        {},
        false
    });
    
    registerCommand({
        "FLIGHT_PLAN",
        "Flight Plan",
        {"flight plan", "show flight plan", "flight plan status"},
        {},
        false
    });
}

void CommandRecognizer::initializeEmergencyCommands() {
    registerCommand({
        "EMERGENCY_LANDING",
        "Emergency Landing",
        {"emergency landing", "declare emergency", "mayday"},
        {},
        true
    });
    
    registerCommand({
        "TERRAIN_WARNING_CHECK",
        "Terrain Warning Check",
        {"terrain warning", "terrain alert", "pull up"},
        {},
        true
    });
    
    registerCommand({
        "DIVERT_TO_NEAREST",
        "Divert to Nearest",
        {"divert to nearest", "find nearest airport", "emergency divert"},
        {},
        true
    });
}

CommandCategory CommandRecognizer::determineCategory(
    const std::string& command_id) const {
    
    if (command_id.find("SET_") != std::string::npos ||
        command_id.find("NAVIGATE") != std::string::npos ||
        command_id.find("DIRECT") != std::string::npos) {
        return CommandCategory::NAVIGATION;
    }
    
    if (command_id.find("RUNWAY") != std::string::npos ||
        command_id.find("APPROACH") != std::string::npos ||
        command_id.find("LANDING") != std::string::npos) {
        return CommandCategory::RUNWAY_APPROACH;
    }
    
    if (command_id.find("CHECKLIST") != std::string::npos ||
        command_id.find("CHECK") != std::string::npos ||
        command_id.find("WEIGHT") != std::string::npos) {
        return CommandCategory::FLIGHT_SYSTEMS;
    }
    
    if (command_id.find("QUERY") != std::string::npos ||
        command_id.find("STATUS") != std::string::npos ||
        command_id.find("WEATHER") != std::string::npos ||
        command_id.find("TERRAIN") != std::string::npos) {
        return CommandCategory::STATUS_QUERY;
    }
    
    if (command_id.find("EMERGENCY") != std::string::npos ||
        command_id.find("MAYDAY") != std::string::npos) {
        return CommandCategory::EMERGENCY;
    }
    
    return CommandCategory::UNKNOWN;
}

// ============================================================================
// ParameterExtractor Implementation
// ============================================================================

ParameterExtractor::ParameterExtractor() {
    // Initialize number words
    number_words_ = {
        {"zero", 0}, {"one", 1}, {"two", 2}, {"three", 3}, {"four", 4},
        {"five", 5}, {"six", 6}, {"seven", 7}, {"eight", 8}, {"nine", 9},
        {"ten", 10}, {"eleven", 11}, {"twelve", 12}, {"thirteen", 13},
        {"fourteen", 14}, {"fifteen", 15}, {"sixteen", 16}, {"seventeen", 17},
        {"eighteen", 18}, {"nineteen", 19}, {"twenty", 20}, {"thirty", 30},
        {"forty", 40}, {"fifty", 50}, {"sixty", 60}, {"seventy", 70},
        {"eighty", 80}, {"ninety", 90}, {"hundred", 100}, {"thousand", 1000}
    };
    
    // Direction headings (degrees)
    direction_headings_ = {
        {"north", 0}, {"n", 0},
        {"northeast", 45}, {"ne", 45},
        {"east", 90}, {"e", 90},
        {"southeast", 135}, {"se", 135},
        {"south", 180}, {"s", 180},
        {"southwest", 225}, {"sw", 225},
        {"west", 270}, {"w", 270},
        {"northwest", 315}, {"nw", 315}
    };
}

double ParameterExtractor::extractNumericParameter(
    const std::string& text,
    const std::string& param_type) {
    
    if (param_type == "altitude") {
        return extractAltitude(text);
    } else if (param_type == "heading") {
        return extractHeading(text);
    } else if (param_type == "speed") {
        return extractSpeed(text);
    }
    
    // Generic number extraction
    auto tokens = tokenize(text);
    for (const auto& token : tokens) {
        if (isNumericWord(token)) {
            return wordToNumber(token);
        }
    }
    
    return 0.0;
}

std::string ParameterExtractor::extractIdentifier(const std::string& text) {
    auto tokens = tokenize(text);
    
    for (const auto& token : tokens) {
        // Identifiers are typically 3-5 uppercase letters
        if (token.length() >= 2 && token.length() <= 5 &&
            std::all_of(token.begin(), token.end(), ::isalpha)) {
            return token;
        }
    }
    
    return "";
}

std::string ParameterExtractor::extractDirection(const std::string& text) {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    
    for (const auto& [direction, heading] : direction_headings_) {
        if (lower_text.find(direction) != std::string::npos) {
            return direction;
        }
    }
    
    return "";
}

double ParameterExtractor::extractAltitude(const std::string& text) {
    auto tokens = tokenize(text);
    
    // Look for "feet", "FL" (flight level), or just a number
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (isNumericWord(tokens[i])) {
            double value = wordToNumber(tokens[i]);
            
            // Check for unit specification
            if (i + 1 < tokens.size()) {
                std::string unit = tokens[i + 1];
                std::transform(unit.begin(), unit.end(), unit.begin(),
                             [](unsigned char c) { return std::tolower(c); });
                
                if (unit.find("flight") != std::string::npos ||
                    unit.find("fl") != std::string::npos) {
                    return value * 100;  // Flight level (e.g., FL250 = 25000 feet)
                }
            }
            
            return value;  // Assume feet
        }
    }
    
    return 0.0;
}

double ParameterExtractor::extractSpeed(const std::string& text) {
    return extractNumericParameter(text, "");
}

double ParameterExtractor::extractHeading(const std::string& text) {
    // Check for direction first
    std::string direction = extractDirection(text);
    if (!direction.empty() && direction_headings_.count(direction)) {
        return direction_headings_[direction];
    }
    
    // Otherwise extract numeric heading
    return extractNumericParameter(text, "");
}

double ParameterExtractor::extractDuration(const std::string& text) {
    auto tokens = tokenize(text);
    double total_seconds = 0.0;
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (isNumericWord(tokens[i])) {
            double value = wordToNumber(tokens[i]);
            
            if (i + 1 < tokens.size()) {
                std::string unit = tokens[i + 1];
                std::transform(unit.begin(), unit.end(), unit.begin(),
                             [](unsigned char c) { return std::tolower(c); });
                
                if (unit.find("minute") != std::string::npos) {
                    total_seconds += value * 60;
                } else if (unit.find("second") != std::string::npos) {
                    total_seconds += value;
                } else if (unit.find("hour") != std::string::npos) {
                    total_seconds += value * 3600;
                }
            }
        }
    }
    
    return total_seconds;
}

std::vector<std::string> ParameterExtractor::extractList(const std::string& text) {
    std::vector<std::string> result;
    std::stringstream ss(text);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        // Trim item
        size_t start = item.find_first_not_of(" \t\r\n");
        size_t end = item.find_last_not_of(" \t\r\n");
        if (start != std::string::npos) {
            result.push_back(item.substr(start, end - start + 1));
        }
    }
    
    return result;
}

bool ParameterExtractor::isNumericWord(const std::string& word) const {
    return number_words_.count(word) > 0;
}

double ParameterExtractor::wordToNumber(const std::string& word) const {
    auto it = number_words_.find(word);
    if (it != number_words_.end()) {
        return it->second;
    }
    return 0.0;
}

std::vector<std::string> ParameterExtractor::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string token;
    
    while (ss >> token) {
        // Convert to lowercase
        std::transform(token.begin(), token.end(), token.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        tokens.push_back(token);
    }
    
    return tokens;
}

// ============================================================================
// SpeechRecognizer Implementation
// ============================================================================

SpeechRecognizer::SpeechRecognizer()
    : current_environment_("moderate") {
    command_recognizer_ = std::make_unique<CommandRecognizer>();
    parameter_extractor_ = std::make_unique<ParameterExtractor>();
}

SpeechRecognizer::~SpeechRecognizer() {
}

bool SpeechRecognizer::initialize() {
    if (!command_recognizer_->initialize()) {
        return false;
    }
    return true;
}

void SpeechRecognizer::shutdown() {
}

RecognitionResult SpeechRecognizer::recognizeSpeech(
    const std::vector<int16_t>& audio_buffer) {
    
    // In a real implementation, this would use speech-to-text
    // For now, we'll return an empty result (integration point)
    return RecognitionResult{"", "", 0.0, {}, ""};
}

RecognitionResult SpeechRecognizer::recognizeText(const std::string& text) {
    RecognitionResult result = command_recognizer_->recognize(text);
    last_result_ = result;
    return result;
}

bool SpeechRecognizer::requiresConfirmation(const std::string& command_id) const {
    auto cmd = command_recognizer_->getCommand(command_id);
    return cmd.requires_confirmation;
}

std::vector<std::string> SpeechRecognizer::getAvailableCommands() const {
    std::vector<std::string> commands;
    auto all_commands = command_recognizer_->getAllCommands();
    
    for (const auto& cmd : all_commands) {
        commands.push_back(cmd.command_id);
    }
    
    return commands;
}

std::string SpeechRecognizer::getCommandHelp(const std::string& command_id) const {
    auto cmd = command_recognizer_->getCommand(command_id);
    
    std::string help = "Command: " + cmd.display_name + "\n";
    help += "Variations: ";
    for (const auto& var : cmd.variations) {
        help += "'" + var + "' ";
    }
    help += "\n";
    
    if (!cmd.parameters.empty()) {
        help += "Parameters: ";
        for (const auto& [name, desc] : cmd.parameters) {
            help += name + " ";
        }
    }
    
    return help;
}

void SpeechRecognizer::setEnvironment(const std::string& environment) {
    current_environment_ = environment;
}

CommandRecognizer::RecognitionStats SpeechRecognizer::getStatistics() const {
    return command_recognizer_->getStatistics();
}

void SpeechRecognizer::setConfidenceThreshold(double threshold) {
    command_recognizer_->setConfidenceThreshold(threshold);
}

} // namespace AICopilot

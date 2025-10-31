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

#include "voice_interpreter.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include "navdata_provider.h"

namespace AICopilot {

// ============================================================================
// CommandMapper Implementation
// ============================================================================

CommandMapper::CommandMapper() {
    initializeMappings();
}

SystemAction CommandMapper::mapCommandToAction(const std::string& command) {
    std::map<std::string, std::string> empty_params;
    return mapCommandWithParameters(command, empty_params);
}

SystemAction CommandMapper::mapCommandWithParameters(
    const std::string& command,
    const std::map<std::string, std::string>& parameters) {
    
    SystemAction action;
    action.parameters = parameters;
    action.priority = 0.5;
    
    auto it = command_map_.find(command);
    if (it != command_map_.end()) {
        action.type = it->second;
        action.requires_confirmation = confirmation_required_[it->second];
        
        // Generate description
        std::stringstream ss;
        ss << command;
        for (const auto& [key, value] : parameters) {
            ss << " (" << key << ": " << value << ")";
        }
        action.description = ss.str();
    } else {
        action.type = SystemActionType::UNKNOWN;
        action.description = "Unknown command";
        action.requires_confirmation = false;
    }
    
    // Set priority based on command type
    if (action.type == SystemActionType::EMERGENCY_LANDING ||
        action.type == SystemActionType::TERRAIN_WARNING_CHECK ||
        action.type == SystemActionType::DIVERT_TO_NEAREST) {
        action.priority = 1.0;  // Critical priority
    } else if (action.type == SystemActionType::SET_ALTITUDE ||
               action.type == SystemActionType::SET_SPEED ||
               action.type == SystemActionType::SET_HEADING) {
        action.priority = 0.8;  // High priority
    } else if (action.type == SystemActionType::QUERY_WEATHER ||
               action.type == SystemActionType::QUERY_SYSTEM_STATUS) {
        action.priority = 0.3;  // Low priority
    }
    
    return action;
}

void CommandMapper::registerMapping(const std::string& command,
                                   SystemActionType action_type,
                                   bool requires_confirmation) {
    command_map_[command] = action_type;
    confirmation_required_[action_type] = requires_confirmation;
}

std::map<std::string, SystemActionType> CommandMapper::getAllMappings() const {
    return command_map_;
}

void CommandMapper::initializeMappings() {
    // Navigation commands
    registerMapping("SET_ALTITUDE", SystemActionType::SET_ALTITUDE, false);
    registerMapping("SET_HEADING", SystemActionType::SET_HEADING, false);
    registerMapping("SET_SPEED", SystemActionType::SET_SPEED, false);
    registerMapping("DIRECT_TO_WAYPOINT", SystemActionType::DIRECT_TO_WAYPOINT, false);
    registerMapping("NAVIGATE_AIRWAY", SystemActionType::NAVIGATE_AIRWAY, false);
    
    // Runway/Approach commands
    registerMapping("FIND_BEST_RUNWAY", SystemActionType::SELECT_RUNWAY, false);
    registerMapping("REQUEST_RUNWAY_INFO", SystemActionType::SELECT_RUNWAY, false);
    registerMapping("APPROACH_FROM_DIRECTION", SystemActionType::REQUEST_APPROACH, false);
    registerMapping("CHECK_LANDING_DISTANCE", SystemActionType::CHECK_LANDING_DISTANCE, false);
    
    // Flight system commands
    registerMapping("PREFLIGHT_CHECKLIST", SystemActionType::EXECUTE_CHECKLIST, true);
    registerMapping("WEIGHT_BALANCE_CHECK", SystemActionType::WEIGHT_BALANCE_CHECK, false);
    registerMapping("CALCULATE_VSPEEDS", SystemActionType::VSPEED_CALCULATION, false);
    registerMapping("STABILIZED_APPROACH_CHECK", SystemActionType::STABILIZED_APPROACH_CHECK, false);
    
    // Status query commands
    registerMapping("CURRENT_WEATHER", SystemActionType::CHECK_WEATHER, false);
    registerMapping("TERRAIN_AHEAD", SystemActionType::CHECK_TERRAIN, false);
    registerMapping("NAVIGATION_STATUS", SystemActionType::QUERY_NAVIGATION_STATUS, false);
    registerMapping("SYSTEM_STATUS", SystemActionType::QUERY_SYSTEM_STATUS, false);
    registerMapping("FLIGHT_PLAN", SystemActionType::QUERY_FLIGHT_PLAN, false);
    
    // Emergency commands
    registerMapping("EMERGENCY_LANDING", SystemActionType::EMERGENCY_LANDING, true);
    registerMapping("TERRAIN_WARNING_CHECK", SystemActionType::TERRAIN_WARNING_CHECK, true);
    registerMapping("DIVERT_TO_NEAREST", SystemActionType::DIVERT_TO_NEAREST, true);
}

// ============================================================================
// ParameterValidator Implementation
// ============================================================================

ParameterValidator::ParameterValidator()
    : aircraft_type_("generic") {
    
    // Initialize aircraft type limits
    aircraft_limits_["generic"] = {30, 600, 50000};
    aircraft_limits_["single_engine"] = {40, 200, 20000};
    aircraft_limits_["twin_engine"] = {50, 350, 35000};
    aircraft_limits_["jet"] = {100, 500, 50000};
    aircraft_limits_["helicopter"] = {0, 150, 25000};
}

bool ParameterValidator::validateAltitude(double altitude) const {
    return altitude >= 0 && altitude <= 50000;
}

bool ParameterValidator::validateHeading(double heading) const {
    return heading >= 0 && heading <= 360;
}

bool ParameterValidator::validateSpeed(double speed) const {
    auto it = aircraft_limits_.find(aircraft_type_);
    if (it != aircraft_limits_.end()) {
        return speed >= it->second.min_speed && speed <= it->second.max_speed;
    }
    return speed >= 30 && speed <= 600;  // Default limits
}

bool ParameterValidator::validateWaypoint(const std::string& waypoint_id) const {
    // Waypoint IDs are typically 2-5 uppercase letters
    if (waypoint_id.length() < 2 || waypoint_id.length() > 5) {
        return false;
    }
    
    return std::all_of(waypoint_id.begin(), waypoint_id.end(),
                      [](unsigned char c) { return std::isalpha(c); });
}

bool ParameterValidator::validateRunway(const std::string& runway_id) const {
    // Runway format: two digits optionally followed by L/C/R
    // Examples: "01", "18L", "09R"
    if (runway_id.length() < 2 || runway_id.length() > 3) {
        return false;
    }
    
    // First two chars must be digits
    if (!std::isdigit(runway_id[0]) || !std::isdigit(runway_id[1])) {
        return false;
    }
    
    // Optional third char must be L, C, or R
    if (runway_id.length() == 3) {
        char c = std::tolower(runway_id[2]);
        if (c != 'l' && c != 'c' && c != 'r') {
            return false;
        }
    }
    
    return true;
}

bool ParameterValidator::validateAirway(const std::string& airway_name) const {
    // Airways are typically one or two letters followed by digits
    // Examples: "J110", "V1", "A590"
    if (airway_name.empty() || airway_name.length() > 5) {
        return false;
    }
    
    // First char(s) should be letters
    size_t i = 0;
    while (i < airway_name.length() && std::isalpha(airway_name[i])) {
        i++;
    }
    
    if (i == 0) return false;  // Must have at least one letter
    
    // Rest should be digits
    while (i < airway_name.length() && std::isdigit(airway_name[i])) {
        i++;
    }
    return i == airway_name.length();
}

double ParameterValidator::clampAltitude(double altitude) const {
    return std::max(0.0, std::min(50000.0, altitude));
}

double ParameterValidator::clampSpeed(double speed) const {
    auto it = aircraft_limits_.find(aircraft_type_);
    if (it != aircraft_limits_.end()) {
        return std::max(it->second.min_speed,
                       std::min(it->second.max_speed, speed));
    }
    return std::max(30.0, std::min(600.0, speed));
}

double ParameterValidator::clampHeading(double heading) const {
    while (heading < 0) heading += 360;
    while (heading >= 360) heading -= 360;
    return heading;
}

void ParameterValidator::setAircraftType(const std::string& aircraft_type) {
    aircraft_type_ = aircraft_type;
}

// ============================================================================
// AmbiguityResolver Implementation
// ============================================================================

AmbiguityResolver::AmbiguityResolver()
    : current_airport_(""), current_airway_("") {
}

void AmbiguityResolver::setNavdataProvider(std::shared_ptr<INavdataProvider> provider) {
    navdata_provider_ = provider;
}

bool AmbiguityResolver::hasAmbiguity(const SystemAction& action) const {
    // Actions with missing or ambiguous parameters have ambiguity
    
    if (action.type == SystemActionType::DIRECT_TO_WAYPOINT) {
        return action.parameters.find("waypoint") == action.parameters.end() ||
               action.parameters.at("waypoint").empty();
    }
    
    if (action.type == SystemActionType::SELECT_RUNWAY) {
        return action.parameters.find("runway") == action.parameters.end() ||
               action.parameters.at("runway").empty();
    }
    
    return false;
}

AmbiguityResolution AmbiguityResolver::resolve(const SystemAction& action) {
    AmbiguityResolution resolution;
    resolution.preferred_action_index = 0;
    
    if (action.type == SystemActionType::DIRECT_TO_WAYPOINT) {
        resolution.clarification_questions.push_back(
            "Which waypoint? Please specify the waypoint identifier.");
    }
    
    if (action.type == SystemActionType::SELECT_RUNWAY) {
        resolution.clarification_questions.push_back(
            "Which runway? Please specify the runway number.");
    }
    
    // Default to original action if no ambiguity
    if (resolution.clarification_questions.empty()) {
        resolution.possible_actions.push_back(action);
        resolution.preferred_action_index = 0;
    }
    
    return resolution;
}

SystemAction AmbiguityResolver::selectAction(
    const AmbiguityResolution& resolution,
    int selected_index) {
    
    if (selected_index >= 0 && 
        selected_index < (int)resolution.possible_actions.size()) {
        return resolution.possible_actions[selected_index];
    }
    
    return SystemAction{SystemActionType::UNKNOWN, {}, "No action selected", false, 0.0};
}

std::vector<std::string> AmbiguityResolver::resolveRunwayAmbiguity(
    const std::string& airport_code) {
    std::vector<std::string> runways;
    if (airport_code.empty()) {
        return runways;
    }

    std::string normalized = airport_code;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    if (auto provider = navdata_provider_.lock()) {
        AirportInfo airportInfo;
        if (provider->getAirportByICAO(normalized, airportInfo)) {
            runways = airportInfo.runways;
            if (runways.empty()) {
                AirportLayout layout;
                if (provider->getAirportLayout(normalized, layout)) {
                    for (const auto& runway : layout.runways) {
                        runways.push_back(runway.runway_ident);
                    }
                }
            }
        }
    }

    if (runways.empty() && normalized == "KJFK") {
        runways = {"04L", "04R", "13L", "13R", "22L", "22R", "31L", "31R"};
    }

    std::sort(runways.begin(), runways.end());
    runways.erase(std::unique(runways.begin(), runways.end()), runways.end());
    return runways;
}

std::vector<std::string> AmbiguityResolver::resolveWaypointAmbiguity(
    const std::string& waypoint_name) {
    
    std::vector<std::string> candidates;
    if (waypoint_name.empty()) {
        return candidates;
    }

    const std::string normalized = [&waypoint_name]() {
        std::string upper = waypoint_name;
        std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });
        return upper;
    }();

    if (auto provider = navdata_provider_.lock()) {
        AirportInfo airportInfo;
        if (provider->getAirportByICAO(normalized, airportInfo)) {
            candidates.push_back(normalized);
        } else if (normalized.size() == 3) {
            std::string prefixed = "K" + normalized;
            if (provider->getAirportByICAO(prefixed, airportInfo)) {
                candidates.push_back(prefixed);
            }
        }

        NavaidInfo navaidInfo;
        if (provider->getNavaidByID(normalized, navaidInfo)) {
            candidates.push_back(normalized);
        }

        std::sort(candidates.begin(), candidates.end());
        candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    }

    return candidates;
}

VoiceInterpreter::VoiceInterpreter() {
    command_mapper_ = std::make_unique<CommandMapper>();
    parameter_validator_ = std::make_unique<ParameterValidator>();
    ambiguity_resolver_ = std::make_unique<AmbiguityResolver>();
}

VoiceInterpreter::~VoiceInterpreter() {
}

bool VoiceInterpreter::initialize() {
    return true;
}

void VoiceInterpreter::shutdown() {
}

SystemAction VoiceInterpreter::interpretCommand(const std::string& command) {
    std::map<std::string, std::string> empty_params;
    return interpretCommandWithParameters(command, empty_params);
}

SystemAction VoiceInterpreter::interpretCommandWithParameters(
    const std::string& command,
    const std::map<std::string, std::string>& parameters) {
    
    SystemAction action = command_mapper_->mapCommandWithParameters(command, parameters);
    
    // Validate all parameters
    for (const auto& [key, value] : parameters) {
        if (key == "altitude") {
            try {
                double alt = std::stod(value);
                if (!parameter_validator_->validateAltitude(alt)) {
                    action.parameters[key] = std::to_string(
                        parameter_validator_->clampAltitude(alt));
                }
            } catch (...) {}
        }
        
        if (key == "heading") {
            try {
                double hdg = std::stod(value);
                if (!parameter_validator_->validateHeading(hdg)) {
                    action.parameters[key] = std::to_string(
                        parameter_validator_->clampHeading(hdg));
                }
            } catch (...) {}
        }
        
        if (key == "speed") {
            try {
                double spd = std::stod(value);
                if (!parameter_validator_->validateSpeed(spd)) {
                    action.parameters[key] = std::to_string(
                        parameter_validator_->clampSpeed(spd));
                }
            } catch (...) {}
        }
    }
    
    // Check for custom handler
    auto it = custom_handlers_.find(command);
    if (it != custom_handlers_.end()) {
        action = it->second(parameters);
    }
    
    last_action_ = action;
    return action;
}

bool VoiceInterpreter::isCommandAmbiguous(const SystemAction& action) const {
    return ambiguity_resolver_->hasAmbiguity(action);
}

AmbiguityResolution VoiceInterpreter::resolveAmbiguity(
    const SystemAction& action) {
    return ambiguity_resolver_->resolve(action);
}

void VoiceInterpreter::registerCustomHandler(
    const std::string& command_name,
    CommandHandler handler) {
    custom_handlers_[command_name] = handler;
}

bool VoiceInterpreter::validateAction(const SystemAction& action) const {
    if (action.type == SystemActionType::UNKNOWN) {
        return false;
    }
    
    // Validate critical parameters based on action type
    switch (action.type) {
        case SystemActionType::SET_ALTITUDE:
            if (action.parameters.find("altitude") == action.parameters.end()) {
                return false;
            }
            break;
        
        case SystemActionType::SET_HEADING:
            if (action.parameters.find("heading") == action.parameters.end()) {
                return false;
            }
            break;
        
        case SystemActionType::SET_SPEED:
            if (action.parameters.find("speed") == action.parameters.end()) {
                return false;
            }
            break;
        
        default:
            break;
    }
    
    return true;
}

std::string VoiceInterpreter::getActionDescription(const SystemAction& action) const {
    return action.description;
}

void VoiceInterpreter::setFlightContext(const std::string& context_info) {
    flight_context_ = context_info;
}

void VoiceInterpreter::setNavdataProvider(std::shared_ptr<INavdataProvider> provider) {
    if (ambiguity_resolver_) {
        ambiguity_resolver_->setNavdataProvider(std::move(provider));
    }
}

} // namespace AICopilot

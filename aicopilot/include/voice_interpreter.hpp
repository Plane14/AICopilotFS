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

#ifndef VOICE_INTERPRETER_HPP
#define VOICE_INTERPRETER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace AICopilot {

class INavdataProvider;

/**
 * Voice Command Interpreter
 * 
 * Parses recognized voice commands and maps them to flight system actions.
 * Handles parameter extraction, ambiguity resolution, and system integration.
 */

// System action types
enum class SystemActionType {
    SET_ALTITUDE,
    SET_HEADING,
    SET_SPEED,
    DIRECT_TO_WAYPOINT,
    NAVIGATE_AIRWAY,
    SELECT_RUNWAY,
    REQUEST_APPROACH,
    CHECK_WEATHER,
    CHECK_TERRAIN,
    TERRAIN_WARNING_CHECK,
    QUERY_NAVIGATION_STATUS,
    QUERY_FLIGHT_PLAN,
    QUERY_SYSTEM_STATUS,
    QUERY_WEATHER,
    PREFLIGHT_CHECK,
    WEIGHT_BALANCE_CHECK,
    VSPEED_CALCULATION,
    STABILIZED_APPROACH_CHECK,
    CHECK_LANDING_DISTANCE,
    EMERGENCY_LANDING,
    DIVERT_TO_NEAREST,
    EXECUTE_CHECKLIST,
    CUSTOM_ACTION,
    UNKNOWN
};

// System action with parameters
struct SystemAction {
    SystemActionType type;
    std::map<std::string, std::string> parameters;
    std::string description;
    bool requires_confirmation;
    double priority;  // 0.0 (lowest) to 1.0 (highest)
};

// Ambiguity resolution result
struct AmbiguityResolution {
    std::vector<SystemAction> possible_actions;
    std::vector<std::string> clarification_questions;
    int preferred_action_index;
};

/**
 * CommandMapper - Maps recognized commands to system actions
 */
class CommandMapper {
public:
    CommandMapper();
    
    /**
     * Map command string to system action
     */
    SystemAction mapCommandToAction(const std::string& command);
    
    /**
     * Map command with parameters to action
     */
    SystemAction mapCommandWithParameters(
        const std::string& command,
        const std::map<std::string, std::string>& parameters);
    
    /**
     * Register custom command mapping
     */
    void registerMapping(const std::string& command, 
                        SystemActionType action_type,
                        bool requires_confirmation = false);
    
    /**
     * Get all registered mappings
     */
    std::map<std::string, SystemActionType> getAllMappings() const;
    
private:
    std::map<std::string, SystemActionType> command_map_;
    std::map<SystemActionType, bool> confirmation_required_;
    
    void initializeMappings();
};

/**
 * ParameterValidator - Validates and sanitizes action parameters
 */
class ParameterValidator {
public:
    ParameterValidator();
    
    /**
     * Validate altitude parameter
     * Valid range: 0 to 50,000 feet
     */
    bool validateAltitude(double altitude) const;
    
    /**
     * Validate heading parameter
     * Valid range: 0 to 360 degrees
     */
    bool validateHeading(double heading) const;
    
    /**
     * Validate speed parameter
     * Valid range: 30 to 600+ knots depending on aircraft
     */
    bool validateSpeed(double speed) const;
    
    /**
     * Validate waypoint identifier
     */
    bool validateWaypoint(const std::string& waypoint_id) const;
    
    /**
     * Validate runway identifier (e.g., "01L", "18R")
     */
    bool validateRunway(const std::string& runway_id) const;
    
    /**
     * Validate airway name (e.g., "J110", "V1")
     */
    bool validateAirway(const std::string& airway_name) const;
    
    /**
     * Clamp value to valid range
     */
    double clampAltitude(double altitude) const;
    double clampSpeed(double speed) const;
    double clampHeading(double heading) const;
    
    /**
     * Set aircraft type for context-specific validation
     */
    void setAircraftType(const std::string& aircraft_type);
    
private:
    std::string aircraft_type_;
    
    // Aircraft-specific limits
    struct AircraftLimits {
        double min_speed;
        double max_speed;
        double max_altitude;
    };
    std::map<std::string, AircraftLimits> aircraft_limits_;
};

/**
 * AmbiguityResolver - Resolves ambiguous voice commands
 */
class AmbiguityResolver {
public:
    AmbiguityResolver();
    
    /**
     * Detect ambiguity in command
     */
    bool hasAmbiguity(const SystemAction& action) const;
    
    /**
     * Resolve ambiguity and generate clarification questions
     */
    AmbiguityResolution resolve(const SystemAction& action);
    
    /**
     * Select action from ambiguous options (user confirmation)
     */
    SystemAction selectAction(const AmbiguityResolution& resolution, 
                             int selected_index);
    
    /**
     * Resolve runway ambiguity (multiple runways at airport)
     */
    std::vector<std::string> resolveRunwayAmbiguity(
        const std::string& airport_code);
    
    /**
     * Resolve waypoint ambiguity (similar names)
     */
    std::vector<std::string> resolveWaypointAmbiguity(
        const std::string& waypoint_name);

    /**
     * Provide navdata access for resolving ambiguities
     */
    void setNavdataProvider(std::shared_ptr<INavdataProvider> provider);
    
private:
    // Context for ambiguity resolution
    std::string current_airport_;
    std::string current_airway_;
    std::weak_ptr<INavdataProvider> navdata_provider_;
};

/**
 * VoiceInterpreter - Main command interpretation system
 */
class VoiceInterpreter {
public:
    VoiceInterpreter();
    ~VoiceInterpreter();
    
    /**
     * Initialize interpreter
     */
    bool initialize();
    
    /**
     * Shutdown interpreter
     */
    void shutdown();
    
    /**
     * Interpret recognized command
     */
    SystemAction interpretCommand(const std::string& command);
    
    /**
     * Interpret command with extracted parameters
     */
    SystemAction interpretCommandWithParameters(
        const std::string& command,
        const std::map<std::string, std::string>& parameters);
    
    /**
     * Check if command is ambiguous
     */
    bool isCommandAmbiguous(const SystemAction& action) const;
    
    /**
     * Resolve command ambiguity
     */
    AmbiguityResolution resolveAmbiguity(const SystemAction& action);
    
    /**
     * Register custom command handler
     */
    using CommandHandler = std::function<SystemAction(
        const std::map<std::string, std::string>&)>;
    void registerCustomHandler(const std::string& command_name, 
                              CommandHandler handler);
    
    /**
     * Validate action before execution
     */
    bool validateAction(const SystemAction& action) const;
    
    /**
     * Get action description for pilot readback
     */
    std::string getActionDescription(const SystemAction& action) const;
    
    /**
     * Set current flight context (helps with interpretation)
     */
    void setFlightContext(const std::string& context_info);

    /**
     * Provide navdata access to the interpreter for better ambiguity handling
     */
    void setNavdataProvider(std::shared_ptr<INavdataProvider> provider);
    
    /**
     * Get last interpreted action
     */
    SystemAction getLastAction() const { return last_action_; }
    
private:
    std::unique_ptr<CommandMapper> command_mapper_;
    std::unique_ptr<ParameterValidator> parameter_validator_;
    std::unique_ptr<AmbiguityResolver> ambiguity_resolver_;
    
    std::map<std::string, CommandHandler> custom_handlers_;
    
    SystemAction last_action_;
    std::string flight_context_;
    
    // Helper methods
    SystemAction handleNavigationCommand(
        const std::map<std::string, std::string>& params);
    SystemAction handleRunwayCommand(
        const std::map<std::string, std::string>& params);
    SystemAction handleFlightSystemCommand(
        const std::map<std::string, std::string>& params);
    SystemAction handleStatusQuery(
        const std::map<std::string, std::string>& params);
    SystemAction handleEmergency(
        const std::map<std::string, std::string>& params);
};

} // namespace AICopilot

#endif // VOICE_INTERPRETER_HPP

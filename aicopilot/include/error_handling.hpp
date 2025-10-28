/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PHASE 1: ERROR HANDLING AND INPUT VALIDATION
* Module: error_handling.hpp
* 
* PURPOSE:
* - Define exception hierarchy for all error types
* - Provide error codes and messages
* - Support recovery strategies
* - Enable comprehensive error logging
*
* FEATURES:
* - Standard exception hierarchy (base exceptions)
* - Domain-specific exceptions (SimConnect, Navigation, Aircraft, etc)
* - Error codes with detailed message mappings
* - Severity levels (INFO, WARNING, ERROR, CRITICAL)
* - Recovery action recommendations
* - Stack trace support for debugging
*
* USAGE:
*   try {
*       // SimConnect or validation code
*   } catch (const SimConnectException& e) {
*       logger.error(e.getErrorCode(), e.what());
*       // Attempt recovery
*   } catch (const ValidationException& e) {
*       logger.warn(e.getErrorCode(), e.what());
*       // Handle invalid input
*   }
*****************************************************************************/

#ifndef ERROR_HANDLING_HPP
#define ERROR_HANDLING_HPP

#include <exception>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <memory>
#include <vector>
#include <map>
#include <mutex>

namespace AICopilot {

// ============================================================================
// ERROR CODES AND SEVERITY
// ============================================================================

enum class ErrorCode {
    // SimConnect errors (100-199)
    SIMCONNECT_NOT_CONNECTED = 100,
    SIMCONNECT_CONNECTION_FAILED = 101,
    SIMCONNECT_CONNECTION_LOST = 102,
    SIMCONNECT_RECONNECTION_IN_PROGRESS = 103,
    SIMCONNECT_DATA_REQUEST_FAILED = 104,
    SIMCONNECT_EVENT_TRANSMISSION_FAILED = 105,
    SIMCONNECT_INVALID_DATA_RECEIVED = 106,
    SIMCONNECT_TIMEOUT = 107,
    SIMCONNECT_ALREADY_CONNECTED = 108,

    // Validation errors (200-299)
    VALIDATION_INVALID_LATITUDE = 200,
    VALIDATION_INVALID_LONGITUDE = 201,
    VALIDATION_INVALID_ALTITUDE = 202,
    VALIDATION_INVALID_AIRSPEED = 203,
    VALIDATION_INVALID_HEADING = 204,
    VALIDATION_INVALID_WAYPOINT = 205,
    VALIDATION_INVALID_WEIGHT = 206,
    VALIDATION_INVALID_WEIGHT_BALANCE = 207,
    VALIDATION_ANTIMERIDIAN_CROSSING = 208,
    VALIDATION_POLE_PROXIMITY = 209,
    VALIDATION_OUT_OF_RANGE = 210,
    VALIDATION_NULL_POINTER = 211,
    VALIDATION_EMPTY_STRING = 212,
    VALIDATION_INVALID_FORMAT = 213,
    VALIDATION_INVALID_FLIGHT_PLAN = 214,
    VALIDATION_INVALID_WAYPOINT_SEQUENCE = 215,

    // Navigation errors (300-399)
    NAVIGATION_INVALID_FLIGHT_PLAN = 300,
    NAVIGATION_INVALID_WAYPOINT_SEQUENCE = 301,
    NAVIGATION_WAYPOINT_UNREACHABLE = 302,
    NAVIGATION_INSUFFICIENT_FUEL = 303,
    NAVIGATION_ALTITUDE_CONFLICT = 304,
    NAVIGATION_AIRSPACE_VIOLATION = 305,
    NAVIGATION_COORDINATE_CALCULATION_FAILED = 306,

    // Aircraft systems errors (400-499)
    AIRCRAFT_INVALID_STATE = 400,
    AIRCRAFT_ELECTRICAL_FAILURE = 401,
    AIRCRAFT_ENGINE_FAILURE = 402,
    AIRCRAFT_STRUCTURAL_LIMIT_EXCEEDED = 403,
    AIRCRAFT_WEIGHT_BALANCE_OUT_OF_ENVELOPE = 404,
    AIRCRAFT_FUEL_MISMATCH = 405,
    AIRCRAFT_SYSTEMS_UNAVAILABLE = 406,

    // I/O and File errors (500-599)
    FILE_NOT_FOUND = 500,
    FILE_READ_ERROR = 501,
    FILE_WRITE_ERROR = 502,
    FILE_INVALID_FORMAT = 503,
    FILE_CORRUPTED = 504,
    DATABASE_ERROR = 505,
    NETWORK_ERROR = 506,

    // Configuration errors (600-699)
    CONFIG_INVALID_PARAMETER = 600,
    CONFIG_MISSING_PARAMETER = 601,
    CONFIG_INVALID_VALUE = 602,
    CONFIG_FILE_NOT_FOUND = 603,

    // System errors (700-799)
    SYSTEM_OUT_OF_MEMORY = 700,
    SYSTEM_RESOURCE_UNAVAILABLE = 701,
    SYSTEM_TIMEOUT = 702,
    SYSTEM_THREAD_ERROR = 703,

    // Unknown/Other
    UNKNOWN_ERROR = 999
};

enum class ErrorSeverity {
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
    CRITICAL = 3
};

// ============================================================================
// BASE EXCEPTION CLASS
// ============================================================================

class AICopilotException : public std::exception {
protected:
    ErrorCode errorCode_;
    ErrorSeverity severity_;
    std::string message_;
    std::string context_;
    std::time_t timestamp_;
    std::string functionName_;
    int lineNumber_;

public:
    AICopilotException(ErrorCode code, const std::string& msg, 
                      const std::string& func = "", int line = 0)
        : errorCode_(code), message_(msg), functionName_(func), 
          lineNumber_(line), severity_(ErrorSeverity::ERROR) {
        timestamp_ = std::time(nullptr);
    }

    virtual ~AICopilotException() noexcept = default;

    const char* what() const noexcept override {
        return message_.c_str();
    }

    ErrorCode getErrorCode() const noexcept {
        return errorCode_;
    }

    ErrorSeverity getSeverity() const noexcept {
        return severity_;
    }

    const std::string& getContext() const noexcept {
        return context_;
    }

    void setContext(const std::string& ctx) noexcept {
        context_ = ctx;
    }

    void setSeverity(ErrorSeverity sev) noexcept {
        severity_ = sev;
    }

    std::string getFormattedMessage() const noexcept {
        std::ostringstream oss;
        
        // Format: [TIMESTAMP] [SEVERITY] [CODE] [FUNCTION:LINE] MESSAGE
        auto timeStruct = std::localtime(&timestamp_);
        oss << "[" << std::put_time(timeStruct, "%Y-%m-%d %H:%M:%S") << "] ";
        
        oss << "[";
        switch (severity_) {
            case ErrorSeverity::INFO:     oss << "INFO"; break;
            case ErrorSeverity::WARNING:  oss << "WARN"; break;
            case ErrorSeverity::ERROR:    oss << "ERR "; break;
            case ErrorSeverity::CRITICAL: oss << "CRIT"; break;
        }
        oss << "] ";
        
        oss << "[" << static_cast<int>(errorCode_) << "] ";
        
        if (!functionName_.empty()) {
            oss << functionName_ << ":" << lineNumber_ << " ";
        }
        
        oss << message_;
        
        if (!context_.empty()) {
            oss << " (Context: " << context_ << ")";
        }
        
        return oss.str();
    }

    std::time_t getTimestamp() const noexcept {
        return timestamp_;
    }
};

// ============================================================================
// DOMAIN-SPECIFIC EXCEPTIONS
// ============================================================================

/**
 * SimConnect communication errors
 */
class SimConnectException : public AICopilotException {
private:
    int hresult_;

public:
    SimConnectException(ErrorCode code, const std::string& msg, 
                       int hr = 0, const std::string& func = "", int line = 0)
        : AICopilotException(code, msg, func, line), hresult_(hr) {
        severity_ = ErrorSeverity::ERROR;
    }

    int getHResult() const noexcept {
        return hresult_;
    }
};

/**
 * Connection-related errors with recovery support
 */
class ConnectionException : public SimConnectException {
private:
    bool canReconnect_;
    int reconnectAttempts_;
    int maxReconnectAttempts_;

public:
    ConnectionException(ErrorCode code, const std::string& msg, 
                       bool canReconnect = true, int maxAttempts = 5,
                       const std::string& func = "", int line = 0)
        : SimConnectException(code, msg, 0, func, line),
          canReconnect_(canReconnect), reconnectAttempts_(0),
          maxReconnectAttempts_(maxAttempts) {
        severity_ = ErrorSeverity::WARNING;
    }

    bool canAttemptReconnection() const noexcept {
        return canReconnect_ && (reconnectAttempts_ < maxReconnectAttempts_);
    }

    void incrementReconnectAttempt() noexcept {
        reconnectAttempts_++;
    }

    int getReconnectAttempts() const noexcept {
        return reconnectAttempts_;
    }

    int getMaxReconnectAttempts() const noexcept {
        return maxReconnectAttempts_;
    }
};

/**
 * Input validation errors
 */
class ValidationException : public AICopilotException {
private:
    std::string fieldName_;
    std::string fieldValue_;
    std::string expectedConstraint_;

public:
    ValidationException(ErrorCode code, const std::string& msg,
                       const std::string& field = "", 
                       const std::string& value = "",
                       const std::string& constraint = "",
                       const std::string& func = "", int line = 0)
        : AICopilotException(code, msg, func, line),
          fieldName_(field), fieldValue_(value), 
          expectedConstraint_(constraint) {
        severity_ = ErrorSeverity::WARNING;
    }

    const std::string& getFieldName() const noexcept {
        return fieldName_;
    }

    const std::string& getFieldValue() const noexcept {
        return fieldValue_;
    }

    const std::string& getConstraint() const noexcept {
        return expectedConstraint_;
    }
};

/**
 * Navigation errors
 */
class NavigationException : public AICopilotException {
private:
    double latitude_;
    double longitude_;

public:
    NavigationException(ErrorCode code, const std::string& msg,
                       double lat = 0.0, double lon = 0.0,
                       const std::string& func = "", int line = 0)
        : AICopilotException(code, msg, func, line),
          latitude_(lat), longitude_(lon) {
        severity_ = ErrorSeverity::ERROR;
    }

    double getLatitude() const noexcept {
        return latitude_;
    }

    double getLongitude() const noexcept {
        return longitude_;
    }
};

/**
 * Aircraft systems errors
 */
class AircraftSystemException : public AICopilotException {
private:
    std::string systemName_;
    std::string currentState_;

public:
    AircraftSystemException(ErrorCode code, const std::string& msg,
                           const std::string& system = "",
                           const std::string& state = "",
                           const std::string& func = "", int line = 0)
        : AICopilotException(code, msg, func, line),
          systemName_(system), currentState_(state) {
        severity_ = ErrorSeverity::CRITICAL;
    }

    const std::string& getSystemName() const noexcept {
        return systemName_;
    }

    const std::string& getCurrentState() const noexcept {
        return currentState_;
    }
};

// ============================================================================
// ERROR MESSAGE MAPPING
// ============================================================================

class ErrorMessageMap {
private:
    static const std::map<ErrorCode, std::string>& getMessages() {
        static const std::map<ErrorCode, std::string> messages = {
            // SimConnect errors
            {ErrorCode::SIMCONNECT_NOT_CONNECTED, 
             "SimConnect is not connected to simulator"},
            {ErrorCode::SIMCONNECT_CONNECTION_FAILED, 
             "Failed to establish connection with SimConnect"},
            {ErrorCode::SIMCONNECT_CONNECTION_LOST, 
             "Connection to SimConnect was lost unexpectedly"},
            {ErrorCode::SIMCONNECT_RECONNECTION_IN_PROGRESS, 
             "Reconnection to SimConnect is in progress"},
            {ErrorCode::SIMCONNECT_DATA_REQUEST_FAILED, 
             "Failed to request data from SimConnect"},
            {ErrorCode::SIMCONNECT_EVENT_TRANSMISSION_FAILED, 
             "Failed to transmit event through SimConnect"},
            {ErrorCode::SIMCONNECT_INVALID_DATA_RECEIVED, 
             "Received invalid or corrupted data from SimConnect"},
            {ErrorCode::SIMCONNECT_TIMEOUT, 
             "SimConnect operation timed out"},
            {ErrorCode::SIMCONNECT_ALREADY_CONNECTED, 
             "Already connected to SimConnect"},

            // Validation errors
            {ErrorCode::VALIDATION_INVALID_LATITUDE, 
             "Latitude must be between -90 and +90 degrees"},
            {ErrorCode::VALIDATION_INVALID_LONGITUDE, 
             "Longitude must be between -180 and +180 degrees"},
            {ErrorCode::VALIDATION_INVALID_ALTITUDE, 
             "Altitude is outside valid aircraft operating range"},
            {ErrorCode::VALIDATION_INVALID_AIRSPEED, 
             "Airspeed is outside valid operating range"},
            {ErrorCode::VALIDATION_INVALID_HEADING, 
             "Heading must be between 0 and 360 degrees"},
            {ErrorCode::VALIDATION_INVALID_WAYPOINT, 
             "Waypoint coordinates or properties are invalid"},
            {ErrorCode::VALIDATION_INVALID_WEIGHT, 
             "Aircraft weight is outside valid operating envelope"},
            {ErrorCode::VALIDATION_INVALID_WEIGHT_BALANCE, 
             "Weight and balance is outside center of gravity limits"},
            {ErrorCode::VALIDATION_ANTIMERIDIAN_CROSSING, 
             "Route crosses International Date Line - recalculate"},
            {ErrorCode::VALIDATION_POLE_PROXIMITY, 
             "Route comes too close to geographic poles"},
            {ErrorCode::VALIDATION_OUT_OF_RANGE, 
             "Value is outside acceptable range"},
            {ErrorCode::VALIDATION_NULL_POINTER, 
             "Null pointer provided where value required"},
            {ErrorCode::VALIDATION_EMPTY_STRING, 
             "Empty string provided where non-empty string required"},
            {ErrorCode::VALIDATION_INVALID_FORMAT, 
             "Data format is invalid or unrecognized"},

            // Navigation errors
            {ErrorCode::NAVIGATION_INVALID_FLIGHT_PLAN, 
             "Flight plan contains invalid waypoints or properties"},
            {ErrorCode::NAVIGATION_INVALID_WAYPOINT_SEQUENCE, 
             "Waypoint sequence creates impossible navigation path"},
            {ErrorCode::NAVIGATION_WAYPOINT_UNREACHABLE, 
             "Waypoint cannot be reached with current aircraft performance"},
            {ErrorCode::NAVIGATION_INSUFFICIENT_FUEL, 
             "Insufficient fuel to reach next waypoint"},
            {ErrorCode::NAVIGATION_ALTITUDE_CONFLICT, 
             "Altitude assignment conflicts with airspace restrictions"},
            {ErrorCode::NAVIGATION_AIRSPACE_VIOLATION, 
             "Flight plan violates airspace restrictions"},
            {ErrorCode::NAVIGATION_COORDINATE_CALCULATION_FAILED, 
             "Failed to calculate coordinate values"},

            // Aircraft systems errors
            {ErrorCode::AIRCRAFT_INVALID_STATE, 
             "Aircraft is in invalid state for requested operation"},
            {ErrorCode::AIRCRAFT_ELECTRICAL_FAILURE, 
             "Aircraft electrical system has failed"},
            {ErrorCode::AIRCRAFT_ENGINE_FAILURE, 
             "Aircraft engine has failed or is inoperative"},
            {ErrorCode::AIRCRAFT_STRUCTURAL_LIMIT_EXCEEDED, 
             "Aircraft structural limits have been exceeded"},
            {ErrorCode::AIRCRAFT_WEIGHT_BALANCE_OUT_OF_ENVELOPE, 
             "Weight and balance is outside aircraft envelope"},
            {ErrorCode::AIRCRAFT_FUEL_MISMATCH, 
             "Fuel quantity mismatch detected"},
            {ErrorCode::AIRCRAFT_SYSTEMS_UNAVAILABLE, 
             "Required aircraft systems are unavailable"},

            // I/O errors
            {ErrorCode::FILE_NOT_FOUND, 
             "Required file not found"},
            {ErrorCode::FILE_READ_ERROR, 
             "Error reading file - check file permissions"},
            {ErrorCode::FILE_WRITE_ERROR, 
             "Error writing file - check disk space"},
            {ErrorCode::FILE_INVALID_FORMAT, 
             "File format is not recognized"},
            {ErrorCode::FILE_CORRUPTED, 
             "File is corrupted or cannot be parsed"},
            {ErrorCode::DATABASE_ERROR, 
             "Database operation failed"},
            {ErrorCode::NETWORK_ERROR, 
             "Network communication error"},

            // Configuration errors
            {ErrorCode::CONFIG_INVALID_PARAMETER, 
             "Configuration parameter is invalid"},
            {ErrorCode::CONFIG_MISSING_PARAMETER, 
             "Required configuration parameter is missing"},
            {ErrorCode::CONFIG_INVALID_VALUE, 
             "Configuration value is not valid"},
            {ErrorCode::CONFIG_FILE_NOT_FOUND, 
             "Configuration file not found"},

            // System errors
            {ErrorCode::SYSTEM_OUT_OF_MEMORY, 
             "System is out of available memory"},
            {ErrorCode::SYSTEM_RESOURCE_UNAVAILABLE, 
             "Required system resource is unavailable"},
            {ErrorCode::SYSTEM_TIMEOUT, 
             "System operation timed out"},
            {ErrorCode::SYSTEM_THREAD_ERROR, 
             "Thread operation failed"},

            {ErrorCode::UNKNOWN_ERROR, 
             "An unknown error occurred"}
        };
        return messages;
    }

public:
    static std::string getMessage(ErrorCode code) {
        auto& msgs = getMessages();
        auto it = msgs.find(code);
        if (it != msgs.end()) {
            return it->second;
        }
        return "Unknown error code: " + std::to_string(static_cast<int>(code));
    }
};

// ============================================================================
// ERROR RECOVERY STRATEGIES
// ============================================================================

enum class RecoveryAction {
    NONE,                          // No recovery possible
    RETRY,                         // Retry the operation
    RETRY_WITH_BACKOFF,           // Retry with exponential backoff
    RECONNECT,                     // Attempt reconnection
    GRACEFUL_DEGRADATION,         // Continue with reduced functionality
    FAILOVER,                      // Switch to alternative system
    ABORT_OPERATION,               // Stop current operation cleanly
    RESTART_SYSTEM,                // Restart affected subsystem
    NOTIFY_USER,                   // Notify user of error
    LOG_AND_CONTINUE               // Log error and continue
};

class RecoveryStrategy {
public:
    virtual ~RecoveryStrategy() = default;
    virtual RecoveryAction getRecommendedAction() const = 0;
    virtual std::string getRecoveryInstructions() const = 0;
    virtual int getRetryDelay() const { return 1000; }  // milliseconds
    virtual int getMaxRetries() const { return 3; }
};

class SimConnectRecoveryStrategy : public RecoveryStrategy {
public:
    RecoveryAction getRecommendedAction() const override {
        return RecoveryAction::RETRY_WITH_BACKOFF;
    }

    std::string getRecoveryInstructions() const override {
        return "Attempting to reconnect to SimConnect with exponential backoff...";
    }

    int getRetryDelay() const override {
        return 2000;  // 2 second initial delay
    }

    int getMaxRetries() const override {
        return 5;
    }
};

class ValidationRecoveryStrategy : public RecoveryStrategy {
public:
    RecoveryAction getRecommendedAction() const override {
        return RecoveryAction::NOTIFY_USER;
    }

    std::string getRecoveryInstructions() const override {
        return "Fix input validation error and resubmit request";
    }
};

// ============================================================================
// ERROR LOGGER
// ============================================================================

class ErrorLogger {
private:
    std::string logFilePath_;
    bool logToConsole_;
    bool logToFile_;
    ErrorSeverity minimumSeverity_;

public:
    ErrorLogger(const std::string& logFile = "", bool console = true, bool file = true)
        : logFilePath_(logFile), logToConsole_(console), logToFile_(file),
          minimumSeverity_(ErrorSeverity::INFO) {}

    void setMinimumSeverity(ErrorSeverity sev) {
        minimumSeverity_ = sev;
    }

    void logException(const AICopilotException& ex) {
        if (static_cast<int>(ex.getSeverity()) < static_cast<int>(minimumSeverity_)) {
            return;
        }

        std::string formatted = ex.getFormattedMessage();

        if (logToConsole_) {
            std::cerr << formatted << std::endl;
        }

        if (logToFile_ && !logFilePath_.empty()) {
            try {
                std::ofstream file(logFilePath_, std::ios::app);
                if (file.is_open()) {
                    file << formatted << std::endl;
                    file.close();
                }
            } catch (...) {
                // Silently fail if we can't write to log file
            }
        }
    }

    void log(ErrorCode code, const std::string& additionalContext = "") {
        std::string msg = ErrorMessageMap::getMessage(code);
        if (!additionalContext.empty()) {
            msg += " - " + additionalContext;
        }

        AICopilotException ex(code, msg);
        logException(ex);
    }

    void logWarning(ErrorCode code, const std::string& msg = "") {
        std::string fullMsg = msg.empty() ? ErrorMessageMap::getMessage(code) : msg;
        AICopilotException ex(code, fullMsg);
        ex.setSeverity(ErrorSeverity::WARNING);
        logException(ex);
    }

    void logError(ErrorCode code, const std::string& msg = "") {
        std::string fullMsg = msg.empty() ? ErrorMessageMap::getMessage(code) : msg;
        AICopilotException ex(code, fullMsg);
        ex.setSeverity(ErrorSeverity::ERROR);
        logException(ex);
    }

    void logCritical(ErrorCode code, const std::string& msg = "") {
        std::string fullMsg = msg.empty() ? ErrorMessageMap::getMessage(code) : msg;
        AICopilotException ex(code, fullMsg);
        ex.setSeverity(ErrorSeverity::CRITICAL);
        logException(ex);
    }
};

} // namespace AICopilot

#endif // ERROR_HANDLING_HPP

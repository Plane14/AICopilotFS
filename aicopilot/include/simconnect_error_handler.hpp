/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PHASE 1: ERROR HANDLING AND INPUT VALIDATION
* Module: simconnect_error_handler.hpp
* 
* PURPOSE:
* - Add comprehensive error handling to SimConnect communication
* - Implement connection loss detection and recovery
* - Add retry logic with exponential backoff
* - Validate data received from SimConnect
* - Provide graceful degradation on connection failure
* - Add detailed error logging and reporting
*
* FEATURES:
* - Connection state monitoring
* - Automatic reconnection with backoff
* - Data validation on receipt
* - Error-specific recovery actions
* - Connection timeout detection
* - Graceful fallback modes
* - Comprehensive error reporting
*
* USAGE:
*   SimConnectErrorHandler handler;
*   handler.initialize();
*   
*   try {
*       if (!handler.connect(SimulatorType::MSFS2024)) {
*           handler.handleConnectionError(ErrorCode::SIMCONNECT_CONNECTION_FAILED);
*       }
*   } catch (const ConnectionException& e) {
*       handler.attemptRecovery(e);
*   }
*****************************************************************************/

#ifndef SIMCONNECT_ERROR_HANDLER_HPP
#define SIMCONNECT_ERROR_HANDLER_HPP

#include "error_handling.hpp"
#include "validation_framework.hpp"
#include "aicopilot_types.h"
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <queue>
#include <functional>
#ifdef AICOPILOT_HAVE_SIMCONNECT
#include <Windows.h>
#include "SimConnect.h"
#endif

namespace AICopilot {

// ============================================================================
// CONNECTION STATE MANAGEMENT
// ============================================================================

enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    CONNECTION_LOST,
    FAILED,
    GRACEFUL_DEGRADATION
};

struct ConnectionMetrics {
    std::chrono::system_clock::time_point lastConnectAttempt;
    std::chrono::system_clock::time_point lastSuccessfulConnection;
    std::chrono::system_clock::time_point lastDataReceived;
    int failureCount = 0;
    int reconnectAttempts = 0;
    int successfulConnections = 0;
    int totalDataErrors = 0;
    int consecutiveTimeouts = 0;
};

// ============================================================================
// SIMCONNECT ERROR HANDLER
// ============================================================================

class SimConnectErrorHandler {
private:
    ConnectionState state_;
    ConnectionMetrics metrics_;
    ErrorLogger logger_;
    mutable std::mutex stateMutex_;
    std::atomic<bool> isMonitoring_{false};
    
    // Configuration
    int maxReconnectAttempts_ = 5;
    int connectionTimeout_ = 5000;  // milliseconds
    int dataReceiveTimeout_ = 10000;  // milliseconds
    int baseRetryDelay_ = 1000;  // milliseconds
    double backoffMultiplier_ = 1.5;

public:
    SimConnectErrorHandler(const std::string& logFile = "")
        : state_(ConnectionState::DISCONNECTED),
          logger_(logFile) {
        metrics_.lastConnectAttempt = std::chrono::system_clock::now();
    }

    ~SimConnectErrorHandler() = default;

    /**
     * Initialize error handler with configuration
     */
    void initialize() {
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            state_ = ConnectionState::DISCONNECTED;
        }
        logger_.setMinimumSeverity(ErrorSeverity::WARNING);
    }

    /**
     * Set configuration parameters
     */
    void setConfiguration(int maxAttempts, int connTimeout, int dataTimeout, 
                         int baseDelay, double backoff) {
        maxReconnectAttempts_ = maxAttempts;
        connectionTimeout_ = connTimeout;
        dataReceiveTimeout_ = dataTimeout;
        baseRetryDelay_ = baseDelay;
        backoffMultiplier_ = backoff;
    }

    /**
     * Get current connection state
     */
    ConnectionState getConnectionState() const {
        std::lock_guard<std::mutex> lock(stateMutex_);
        return state_;
    }

    /**
     * Check if connection is active
     */
    bool isConnected() const {
        std::lock_guard<std::mutex> lock(stateMutex_);
        return state_ == ConnectionState::CONNECTED;
    }

    /**
     * Validate aircraft state data received from SimConnect
     */
    ValidationResult validateAircraftStateData(const AircraftState& state) {
        // Validate coordinates
        ValidationResult coordResult = CoordinateValidator::validateCoordinatePair(
            state.position.latitude, state.position.longitude);
        if (!coordResult.isValid) {
            return coordResult;
        }

        // Validate altitude
        ValidationResult altResult = AltitudeValidator::validateAltitude(
            state.position.altitude);
        if (!altResult.isValid) {
            return altResult;
        }

        // Validate heading
        ValidationResult headResult = HeadingValidator::validateHeading(state.heading);
        if (!headResult.isValid) {
            return headResult;
        }

        // Validate airspeeds
        ValidationResult iasResult = AirspeedValidator::validateIndicatedAirspeed(
            state.indicatedAirspeed);
        if (!iasResult.isValid) {
            return iasResult;
        }

        ValidationResult tasResult = AirspeedValidator::validateTrueAirspeed(
            state.trueAirspeed);
        if (!tasResult.isValid) {
            return tasResult;
        }

        // Validate vertical speed
        ValidationResult vsResult = AltitudeValidator::validateVerticalSpeed(
            state.verticalSpeed);
        if (!vsResult.isValid) {
            return vsResult;
        }

        // Validate fuel quantity
        if (state.fuelQuantity < 0.0) {
            return ValidationResult(false, ErrorCode::AIRCRAFT_FUEL_MISMATCH,
                "Fuel quantity cannot be negative");
        }

        // Validate pitch and bank (should not exceed realistic values)
        if (std::abs(state.pitch) > 90.0) {
            return ValidationResult(false, ErrorCode::AIRCRAFT_INVALID_STATE,
                "Pitch angle exceeds physical limits");
        }

        if (std::abs(state.bank) > 180.0) {
            return ValidationResult(false, ErrorCode::AIRCRAFT_INVALID_STATE,
                "Bank angle exceeds physical limits");
        }

        // Validate electrical system parameters
        if (state.batteryVoltage < 0.0 || state.batteryVoltage > 50.0) {
            return ValidationResult(false, ErrorCode::AIRCRAFT_ELECTRICAL_FAILURE,
                "Invalid battery voltage");
        }

        if (state.batteryLoad < 0.0 || state.batteryLoad > 500.0) {
            return ValidationResult(false, ErrorCode::AIRCRAFT_ELECTRICAL_FAILURE,
                "Invalid battery load");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid aircraft state");
    }

    /**
     * Handle connection attempt with retry logic
     */
    bool attemptConnection(SimulatorType simType, int timeout = 0) {
        if (timeout == 0) timeout = connectionTimeout_;

        std::lock_guard<std::mutex> lock(stateMutex_);

        if (state_ == ConnectionState::CONNECTED) {
            throw SimConnectException(ErrorCode::SIMCONNECT_ALREADY_CONNECTED,
                "Already connected to simulator");
        }

        try {
            state_ = ConnectionState::CONNECTING;
            metrics_.lastConnectAttempt = std::chrono::system_clock::now();

#ifdef AICOPILOT_HAVE_SIMCONNECT
            // Try to open a lightweight SimConnect session to validate connectivity.
            HANDLE hSim = nullptr;
            HRESULT hr = SimConnect_Open(&hSim, "AICopilotErrorHandler", NULL, 0, 0, 0);
            if (hr == S_OK && hSim != nullptr) {
                // Close immediately; this module does not retain the handle.
                SimConnect_Close(hSim);
                state_ = ConnectionState::CONNECTED;
                metrics_.lastSuccessfulConnection = std::chrono::system_clock::now();
                metrics_.successfulConnections++;
                metrics_.reconnectAttempts = 0;
                logger_.log(ErrorCode::UNKNOWN_ERROR, "Connected to simulator successfully (SimConnect)");
                return true;
            } else {
                state_ = ConnectionState::FAILED;
                metrics_.failureCount++;
                throw SimConnectException(ErrorCode::SIMCONNECT_CONNECTION_FAILED,
                    "SimConnect_Open failed (HRESULT=" + std::to_string(hr) + ")");
            }
#else
            // No SimConnect SDK available in this build -- keep simulated behavior for stub mode
            state_ = ConnectionState::CONNECTED;
            metrics_.lastSuccessfulConnection = std::chrono::system_clock::now();
            metrics_.successfulConnections++;
            metrics_.reconnectAttempts = 0;
            logger_.log(ErrorCode::UNKNOWN_ERROR, "Connected to simulator successfully (stub mode)");
            return true;
#endif

        } catch (const std::exception& e) {
            state_ = ConnectionState::FAILED;
            metrics_.failureCount++;

            throw SimConnectException(
                ErrorCode::SIMCONNECT_CONNECTION_FAILED,
                std::string("Connection attempt failed: ") + e.what()
            );
        }
    }

    /**
     * Attempt connection with exponential backoff
     */
    bool attemptConnectionWithBackoff(SimulatorType simType) {
        for (int attempt = 0; attempt < maxReconnectAttempts_; attempt++) {
            try {
                if (attemptConnection(simType)) {
                    return true;
                }
            } catch (const SimConnectException& e) {
                logger_.logWarning(e.getErrorCode(), 
                    "Connection attempt " + std::to_string(attempt + 1) + 
                    " failed, retrying...");

                metrics_.reconnectAttempts++;

                if (attempt < maxReconnectAttempts_ - 1) {
                    // Calculate backoff delay: delay * (multiplier ^ attempt)
                    int delayMs = static_cast<int>(
                        baseRetryDelay_ * std::pow(backoffMultiplier_, attempt)
                    );

                    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                }
            }
        }

        // All reconnection attempts failed
        throw ConnectionException(
            ErrorCode::SIMCONNECT_CONNECTION_FAILED,
            "Failed to connect to simulator after " + 
            std::to_string(maxReconnectAttempts_) + " attempts",
            true,  // Can attempt reconnection later
            maxReconnectAttempts_
        );
    }

    /**
     * Handle data reception with timeout detection
     */
    bool processDataWithTimeout(const AircraftState& state, int timeout = 0) {
        if (timeout == 0) timeout = dataReceiveTimeout_;

        // Check if we've received data recently
        auto now = std::chrono::system_clock::now();
        auto timeSinceLastData = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - metrics_.lastDataReceived
        ).count();

        if (timeSinceLastData > timeout && metrics_.lastDataReceived != 
            std::chrono::system_clock::time_point::min()) {
            metrics_.consecutiveTimeouts++;

            if (metrics_.consecutiveTimeouts > 3) {
                throw SimConnectException(
                    ErrorCode::SIMCONNECT_TIMEOUT,
                    "SimConnect data reception timeout - no data for " + 
                    std::to_string(timeSinceLastData) + " ms"
                );
            }
        } else {
            metrics_.consecutiveTimeouts = 0;
        }

        // Validate received data
        ValidationResult result = validateAircraftStateData(state);
        if (!result.isValid) {
            metrics_.totalDataErrors++;

            logger_.logWarning(result.errorCode, 
                "Invalid aircraft state data received: " + result.message);

            // Don't throw - log and continue with data
            // This is graceful degradation
        }

        metrics_.lastDataReceived = now;
        return result.isValid;
    }

    /**
     * Handle connection loss
     */
    void handleConnectionLoss() {
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            state_ = ConnectionState::CONNECTION_LOST;
        }

        logger_.logError(ErrorCode::SIMCONNECT_CONNECTION_LOST,
            "Connection to SimConnect has been lost");

        metrics_.failureCount++;
    }

    /**
     * Enter graceful degradation mode
     */
    void enterGracefulDegradation() {
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            state_ = ConnectionState::GRACEFUL_DEGRADATION;
        }

        logger_.logWarning(ErrorCode::SIMCONNECT_CONNECTION_LOST,
            "Entering graceful degradation mode - limited functionality");
    }

    /**
     * Attempt recovery from error
     */
    RecoveryAction attemptRecovery(const AICopilotException& ex) {
        RecoveryAction action = RecoveryAction::NONE;

        switch (ex.getErrorCode()) {
            case ErrorCode::SIMCONNECT_CONNECTION_LOST:
            case ErrorCode::SIMCONNECT_CONNECTION_FAILED:
                action = RecoveryAction::RETRY_WITH_BACKOFF;
                break;

            case ErrorCode::SIMCONNECT_TIMEOUT:
                action = RecoveryAction::RECONNECT;
                break;

            case ErrorCode::SIMCONNECT_INVALID_DATA_RECEIVED:
                action = RecoveryAction::LOG_AND_CONTINUE;
                break;

            case ErrorCode::VALIDATION_INVALID_LATITUDE:
            case ErrorCode::VALIDATION_INVALID_LONGITUDE:
            case ErrorCode::VALIDATION_INVALID_ALTITUDE:
                action = RecoveryAction::NOTIFY_USER;
                break;

            default:
                action = RecoveryAction::LOG_AND_CONTINUE;
                break;
        }

        logger_.logWarning(ex.getErrorCode(), 
            "Recovery action: " + recoverActionToString(action));

        return action;
    }

    /**
     * Get connection metrics
     */
    const ConnectionMetrics& getMetrics() const {
        return metrics_;
    }

    /**
     * Get formatted status report
     */
    std::string getStatusReport() const {
        std::ostringstream oss;
        oss << "SimConnect Status Report\n";
        oss << "========================\n";
        oss << "State: " << connectionStateToString(getConnectionState()) << "\n";
        oss << "Successful Connections: " << metrics_.successfulConnections << "\n";
        oss << "Failed Connections: " << metrics_.failureCount << "\n";
        oss << "Reconnect Attempts: " << metrics_.reconnectAttempts << "\n";
        oss << "Total Data Errors: " << metrics_.totalDataErrors << "\n";
        oss << "Consecutive Timeouts: " << metrics_.consecutiveTimeouts << "\n";
        return oss.str();
    }

private:
    static std::string connectionStateToString(ConnectionState state) {
        switch (state) {
            case ConnectionState::DISCONNECTED: return "DISCONNECTED";
            case ConnectionState::CONNECTING: return "CONNECTING";
            case ConnectionState::CONNECTED: return "CONNECTED";
            case ConnectionState::RECONNECTING: return "RECONNECTING";
            case ConnectionState::CONNECTION_LOST: return "CONNECTION_LOST";
            case ConnectionState::FAILED: return "FAILED";
            case ConnectionState::GRACEFUL_DEGRADATION: return "GRACEFUL_DEGRADATION";
            default: return "UNKNOWN";
        }
    }

    static std::string recoverActionToString(RecoveryAction action) {
        switch (action) {
            case RecoveryAction::NONE: return "NONE";
            case RecoveryAction::RETRY: return "RETRY";
            case RecoveryAction::RETRY_WITH_BACKOFF: return "RETRY_WITH_BACKOFF";
            case RecoveryAction::RECONNECT: return "RECONNECT";
            case RecoveryAction::GRACEFUL_DEGRADATION: return "GRACEFUL_DEGRADATION";
            case RecoveryAction::FAILOVER: return "FAILOVER";
            case RecoveryAction::ABORT_OPERATION: return "ABORT_OPERATION";
            case RecoveryAction::RESTART_SYSTEM: return "RESTART_SYSTEM";
            case RecoveryAction::NOTIFY_USER: return "NOTIFY_USER";
            case RecoveryAction::LOG_AND_CONTINUE: return "LOG_AND_CONTINUE";
            default: return "UNKNOWN";
        }
    }
};

} // namespace AICopilot

#endif // SIMCONNECT_ERROR_HANDLER_HPP

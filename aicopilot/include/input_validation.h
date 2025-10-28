#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <sstream>

namespace AICopilot {

enum class ValidationErrorCode {
    NO_ERROR = 0,
    INVALID_LATITUDE = 1,
    INVALID_LONGITUDE = 2,
    INVALID_ALTITUDE = 3,
    INVALID_SPEED = 4,
    INVALID_WAYPOINT = 5,
    EMPTY_FLIGHTPLAN = 6,
    DUPLICATE_WAYPOINT = 7,
    INVALID_HEADING = 8,
};

struct ValidationResult {
    bool is_valid;
    ValidationErrorCode error_code;
    std::string error_message;
    std::string field_name;

    ValidationResult() 
        : is_valid(true), error_code(ValidationErrorCode::NO_ERROR), 
          error_message(""), field_name("") {}

    ValidationResult(bool valid, ValidationErrorCode code, 
                     const std::string& msg, const std::string& field)
        : is_valid(valid), error_code(code), error_message(msg), field_name(field) {}
};

class InputValidation {
public:
    static ValidationResult ValidateCoordinates(double latitude, double longitude) {
        if (std::isnan(latitude) || std::isinf(latitude)) {
            return ValidationResult(false, ValidationErrorCode::INVALID_LATITUDE,
                "Latitude is NaN or infinity", "latitude");
        }

        if (latitude < -90.0 || latitude > 90.0) {
            std::ostringstream oss;
            oss << "Latitude out of range: " << latitude << " (valid: -90 to 90)";
            return ValidationResult(false, ValidationErrorCode::INVALID_LATITUDE,
                oss.str(), "latitude");
        }

        if (std::isnan(longitude) || std::isinf(longitude)) {
            return ValidationResult(false, ValidationErrorCode::INVALID_LONGITUDE,
                "Longitude is NaN or infinity", "longitude");
        }

        if (longitude < -180.0 || longitude > 180.0) {
            std::ostringstream oss;
            oss << "Longitude out of range: " << longitude << " (valid: -180 to 180)";
            return ValidationResult(false, ValidationErrorCode::INVALID_LONGITUDE,
                oss.str(), "longitude");
        }

        return ValidationResult(true, ValidationErrorCode::NO_ERROR, "", "");
    }

    static ValidationResult ValidateAltitude(double feet) {
        if (std::isnan(feet) || std::isinf(feet)) {
            return ValidationResult(false, ValidationErrorCode::INVALID_ALTITUDE,
                "Altitude is NaN or infinity", "altitude");
        }

        constexpr double MIN_ALT = -1000.0;
        constexpr double MAX_ALT = 60000.0;

        if (feet < MIN_ALT) {
            std::ostringstream oss;
            oss << "Altitude below minimum: " << feet << " feet (minimum: " 
                << MIN_ALT << " feet)";
            return ValidationResult(false, ValidationErrorCode::INVALID_ALTITUDE,
                oss.str(), "altitude");
        }

        if (feet > MAX_ALT) {
            std::ostringstream oss;
            oss << "Altitude above maximum: " << feet << " feet (maximum: " 
                << MAX_ALT << " feet)";
            return ValidationResult(false, ValidationErrorCode::INVALID_ALTITUDE,
                oss.str(), "altitude");
        }

        return ValidationResult(true, ValidationErrorCode::NO_ERROR, "", "");
    }

    static ValidationResult ValidateSpeed(double knots) {
        if (std::isnan(knots) || std::isinf(knots)) {
            return ValidationResult(false, ValidationErrorCode::INVALID_SPEED,
                "Speed is NaN or infinity", "speed");
        }

        constexpr double MIN_SPEED = 0.0;
        constexpr double MAX_SPEED = 650.0;

        if (knots < MIN_SPEED) {
            std::ostringstream oss;
            oss << "Speed is negative: " << knots << " knots";
            return ValidationResult(false, ValidationErrorCode::INVALID_SPEED,
                oss.str(), "speed");
        }

        if (knots > MAX_SPEED) {
            std::ostringstream oss;
            oss << "Speed exceeds maximum: " << knots << " knots (maximum: " 
                << MAX_SPEED << " knots)";
            return ValidationResult(false, ValidationErrorCode::INVALID_SPEED,
                oss.str(), "speed");
        }

        return ValidationResult(true, ValidationErrorCode::NO_ERROR, "", "");
    }

    static ValidationResult ValidateHeading(int degrees) {
        if (degrees < 0 || degrees >= 360) {
            std::ostringstream oss;
            oss << "Heading out of range: " << degrees << " degrees (valid: 0-359)";
            return ValidationResult(false, ValidationErrorCode::INVALID_HEADING,
                oss.str(), "heading");
        }

        return ValidationResult(true, ValidationErrorCode::NO_ERROR, "", "");
    }

    static ValidationResult ValidateFlightPlan(const std::vector<std::string>& waypoints) {
        if (waypoints.empty()) {
            return ValidationResult(false, ValidationErrorCode::EMPTY_FLIGHTPLAN,
                "Flight plan has no waypoints", "waypoints");
        }

        if (waypoints.size() < 2) {
            return ValidationResult(false, ValidationErrorCode::EMPTY_FLIGHTPLAN,
                "Flight plan must have at least 2 waypoints (departure and arrival)",
                "waypoints");
        }

        std::vector<std::string> seen_waypoints;
        for (size_t i = 0; i < waypoints.size(); ++i) {
            const auto& wp = waypoints[i];

            if (wp.empty()) {
                std::ostringstream oss;
                oss << "Waypoint " << i << " has empty identifier";
                return ValidationResult(false, ValidationErrorCode::INVALID_WAYPOINT,
                    oss.str(), "waypoints[" + std::to_string(i) + "]");
            }

            for (const auto& seen : seen_waypoints) {
                if (seen == wp) {
                    std::ostringstream oss;
                    oss << "Duplicate waypoint at index " << i << ": " << wp;
                    return ValidationResult(false, ValidationErrorCode::DUPLICATE_WAYPOINT,
                        oss.str(), "waypoints[" + std::to_string(i) + "]");
                }
            }

            seen_waypoints.push_back(wp);
        }

        return ValidationResult(true, ValidationErrorCode::NO_ERROR, "", "");
    }

    static ValidationResult ValidateLongitude(double longitude) {
        if (std::isnan(longitude) || std::isinf(longitude)) {
            return ValidationResult(false, ValidationErrorCode::INVALID_LONGITUDE,
                "Longitude is NaN or infinity", "longitude");
        }

        if (longitude < -180.0 || longitude > 180.0) {
            std::ostringstream oss;
            oss << "Longitude out of range: " << longitude << " (valid: -180 to 180)";
            return ValidationResult(false, ValidationErrorCode::INVALID_LONGITUDE,
                oss.str(), "longitude");
        }

        return ValidationResult(true, ValidationErrorCode::NO_ERROR, "", "");
    }

    static ValidationResult ValidateLatitude(double latitude) {
        if (std::isnan(latitude) || std::isinf(latitude)) {
            return ValidationResult(false, ValidationErrorCode::INVALID_LATITUDE,
                "Latitude is NaN or infinity", "latitude");
        }

        if (latitude < -90.0 || latitude > 90.0) {
            std::ostringstream oss;
            oss << "Latitude out of range: " << latitude << " (valid: -90 to 90)";
            return ValidationResult(false, ValidationErrorCode::INVALID_LATITUDE,
                oss.str(), "latitude");
        }

        return ValidationResult(true, ValidationErrorCode::NO_ERROR, "", "");
    }

    static bool IsCoordinateValid(double latitude, double longitude) {
        auto result = ValidateCoordinates(latitude, longitude);
        return result.is_valid;
    }

    static bool IsAltitudeValid(double feet) {
        auto result = ValidateAltitude(feet);
        return result.is_valid;
    }

    static bool IsSpeedValid(double knots) {
        auto result = ValidateSpeed(knots);
        return result.is_valid;
    }

    static bool IsHeadingValid(int degrees) {
        auto result = ValidateHeading(degrees);
        return result.is_valid;
    }

    static bool IsFlightPlanValid(const std::vector<std::string>& waypoints) {
        auto result = ValidateFlightPlan(waypoints);
        return result.is_valid;
    }

    static std::string GetErrorCodeName(ValidationErrorCode code) {
        switch (code) {
            case ValidationErrorCode::NO_ERROR:
                return "NO_ERROR";
            case ValidationErrorCode::INVALID_LATITUDE:
                return "INVALID_LATITUDE";
            case ValidationErrorCode::INVALID_LONGITUDE:
                return "INVALID_LONGITUDE";
            case ValidationErrorCode::INVALID_ALTITUDE:
                return "INVALID_ALTITUDE";
            case ValidationErrorCode::INVALID_SPEED:
                return "INVALID_SPEED";
            case ValidationErrorCode::INVALID_WAYPOINT:
                return "INVALID_WAYPOINT";
            case ValidationErrorCode::EMPTY_FLIGHTPLAN:
                return "EMPTY_FLIGHTPLAN";
            case ValidationErrorCode::DUPLICATE_WAYPOINT:
                return "DUPLICATE_WAYPOINT";
            case ValidationErrorCode::INVALID_HEADING:
                return "INVALID_HEADING";
            default:
                return "UNKNOWN_ERROR";
        }
    }
};

}

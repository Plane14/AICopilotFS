/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PHASE 1: ERROR HANDLING AND INPUT VALIDATION
* Module: validation_framework.hpp
* 
* PURPOSE:
* - Provide comprehensive input validation for all data types
* - Validate coordinates with edge case handling
* - Validate aircraft parameters and limits
* - Validate flight planning inputs
* - Support validation composition and error reporting
*
* FEATURES:
* - Geographic coordinate validation (lat/lon bounds)
* - Antimeridian crossing detection and handling
* - Pole proximity detection
* - Altitude range validation
* - Airspeed range validation
* - Weight and balance envelope checking
* - Waypoint existence and sequence validation
* - Composable validators
* - Detailed validation result messages
*
* USAGE:
*   ValidationResult result = CoordinateValidator::validate(lat, lon);
*   if (!result.isValid) {
*       logger.warn(result.errorCode, result.message);
*   }
*
*   // Or with exception throwing
*   if (!result.isValid) {
*       throw ValidationException(result.errorCode, result.message);
*   }
*****************************************************************************/

#ifndef VALIDATION_FRAMEWORK_HPP
#define VALIDATION_FRAMEWORK_HPP

#include "error_handling.hpp"
#include "aicopilot_types.h"
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <memory>

namespace AICopilot {

// ============================================================================
// VALIDATION RESULT STRUCTURE
// ============================================================================

struct ValidationResult {
    bool isValid;
    ErrorCode errorCode;
    std::string message;
    std::string fieldName;
    std::string suggestedValue;  // Optional suggestion for correction
    int severityLevel;           // 0=info, 1=warning, 2=error, 3=critical

    ValidationResult()
        : isValid(true), errorCode(ErrorCode::UNKNOWN_ERROR),
          severityLevel(0) {}

    ValidationResult(bool valid, ErrorCode code, const std::string& msg)
        : isValid(valid), errorCode(code), message(msg), severityLevel(valid ? 0 : 2) {}

    ValidationResult(bool valid, ErrorCode code, const std::string& msg,
                    const std::string& field)
        : isValid(valid), errorCode(code), message(msg),
          fieldName(field), severityLevel(valid ? 0 : 2) {}
};

// ============================================================================
// COORDINATE VALIDATOR
// ============================================================================

class CoordinateValidator {
public:
    static constexpr double MIN_LATITUDE = -90.0;
    static constexpr double MAX_LATITUDE = 90.0;
    static constexpr double MIN_LONGITUDE = -180.0;
    static constexpr double MAX_LONGITUDE = 180.0;
    static constexpr double POLE_PROXIMITY_THRESHOLD = 5.0;  // degrees
    static constexpr double ANTIMERIDIAN_THRESHOLD = 170.0;   // degrees from ±180

    /**
     * Validate latitude value
     */
    static ValidationResult validateLatitude(double latitude) {
        if (std::isnan(latitude) || std::isinf(latitude)) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_LATITUDE,
                "Latitude is NaN or infinite");
        }

        if (latitude < MIN_LATITUDE || latitude > MAX_LATITUDE) {
            std::ostringstream oss;
            oss << "Latitude " << latitude << " is outside valid range ["
                << MIN_LATITUDE << ", " << MAX_LATITUDE << "]";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_LATITUDE,
                oss.str(), "latitude");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid latitude", "latitude");
    }

    /**
     * Validate longitude value
     */
    static ValidationResult validateLongitude(double longitude) {
        if (std::isnan(longitude) || std::isinf(longitude)) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_LONGITUDE,
                "Longitude is NaN or infinite");
        }

        if (longitude < MIN_LONGITUDE || longitude > MAX_LONGITUDE) {
            std::ostringstream oss;
            oss << "Longitude " << longitude << " is outside valid range ["
                << MIN_LONGITUDE << ", " << MAX_LONGITUDE << "]";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_LONGITUDE,
                oss.str(), "longitude");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid longitude", "longitude");
    }

    /**
     * Validate latitude and longitude pair
     */
    static ValidationResult validateCoordinatePair(double latitude, double longitude) {
        // Validate latitude first
        ValidationResult latResult = validateLatitude(latitude);
        if (!latResult.isValid) {
            return latResult;
        }

        // Validate longitude
        ValidationResult lonResult = validateLongitude(longitude);
        if (!lonResult.isValid) {
            return lonResult;
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR,
            "Valid coordinate pair", "coordinates");
    }

    /**
     * Check if coordinates are at poles
     */
    static ValidationResult checkPoleCrossing(double latitude) {
        if (std::abs(latitude) > (90.0 - POLE_PROXIMITY_THRESHOLD)) {
            std::ostringstream oss;
            oss << "Latitude " << latitude << " is too close to poles (within "
                << POLE_PROXIMITY_THRESHOLD << " degrees)";
            return ValidationResult(false, ErrorCode::VALIDATION_POLE_PROXIMITY,
                oss.str(), "latitude");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Safe from pole proximity");
    }

    /**
     * Check for antimeridian crossing (International Date Line)
     * Returns warning if route crosses ±180° longitude line
     */
    static ValidationResult checkAntimeridianCrossing(double lon1, double lon2) {
        // Calculate the absolute difference
        double diff = std::abs(lon2 - lon1);

        // If difference is greater than 180, we're crossing the antimeridian
        if (diff > 180.0) {
            std::ostringstream oss;
            oss << "Route appears to cross International Date Line: lon1=" << lon1
                << ", lon2=" << lon2 << ". Difference=" << diff << " degrees.";
            
            ValidationResult result(false, ErrorCode::VALIDATION_ANTIMERIDIAN_CROSSING,
                oss.str(), "longitude_pair");
            result.severityLevel = 1;  // Warning, not error
            return result;
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "No antimeridian crossing detected");
    }

    /**
     * Normalize longitude to standard ±180 range
     */
    static double normalizeLongitude(double longitude) {
        // First bring into -360 to +360 range
        while (longitude > 180.0) {
            longitude -= 360.0;
        }
        while (longitude < -180.0) {
            longitude += 360.0;
        }
        return longitude;
    }

    /**
     * Normalize latitude (handle edge cases)
     */
    static double normalizeLatitude(double latitude) {
        // Clamp to valid range
        if (latitude > 90.0) latitude = 90.0;
        if (latitude < -90.0) latitude = -90.0;
        return latitude;
    }

    /**
     * Calculate great circle distance between two points
     * Returns distance in nautical miles
     */
    static double greatCircleDistance(double lat1, double lon1, double lat2, double lon2) {
        // Earth's radius in nautical miles
        constexpr double EARTH_RADIUS_NM = 3440.065;
        constexpr double PI = 3.14159265358979323846;

        // Convert to radians
        double lat1Rad = lat1 * PI / 180.0;
        double lon1Rad = lon1 * PI / 180.0;
        double lat2Rad = lat2 * PI / 180.0;
        double lon2Rad = lon2 * PI / 180.0;

        // Haversine formula
        double dLat = lat2Rad - lat1Rad;
        double dLon = lon2Rad - lon1Rad;

        double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
                   std::cos(lat1Rad) * std::cos(lat2Rad) *
                   std::sin(dLon / 2.0) * std::sin(dLon / 2.0);

        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
        double distance = EARTH_RADIUS_NM * c;

        return distance;
    }

    /**
     * Calculate initial bearing from point 1 to point 2
     * Returns bearing in degrees (0-360)
     */
    static double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
        constexpr double PI = 3.14159265358979323846;
        
        double lat1Rad = lat1 * PI / 180.0;
        double lon1Rad = lon1 * PI / 180.0;
        double lat2Rad = lat2 * PI / 180.0;
        double lon2Rad = lon2 * PI / 180.0;

        double dLon = lon2Rad - lon1Rad;

        double y = std::sin(dLon) * std::cos(lat2Rad);
        double x = std::cos(lat1Rad) * std::sin(lat2Rad) -
                   std::sin(lat1Rad) * std::cos(lat2Rad) * std::cos(dLon);

        double bearing = std::atan2(y, x) * 180.0 / PI;
        bearing = std::fmod(bearing + 360.0, 360.0);  // Normalize to 0-360

        return bearing;
    }
};

// ============================================================================
// ALTITUDE VALIDATOR
// ============================================================================

class AltitudeValidator {
public:
    // Typical aircraft altitude limits (in feet)
    static constexpr double MIN_ALTITUDE = 0.0;
    static constexpr double MAX_GENERAL_ALTITUDE = 50000.0;
    static constexpr double MAX_TURBOCHARGED_ALTITUDE = 35000.0;
    static constexpr double MAX_UNPRESSURIZED_ALTITUDE = 15000.0;

    /**
     * Validate altitude for general aviation
     */
    static ValidationResult validateAltitude(double altitude,
                                            double maxAltitude = MAX_GENERAL_ALTITUDE) {
        if (std::isnan(altitude) || std::isinf(altitude)) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_ALTITUDE,
                "Altitude is NaN or infinite");
        }

        if (altitude < MIN_ALTITUDE) {
            std::ostringstream oss;
            oss << "Altitude " << altitude << " ft is below ground level";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_ALTITUDE,
                oss.str(), "altitude");
        }

        if (altitude > maxAltitude) {
            std::ostringstream oss;
            oss << "Altitude " << altitude << " ft exceeds maximum "
                << maxAltitude << " ft";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_ALTITUDE,
                oss.str(), "altitude");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid altitude");
    }

    /**
     * Validate altitude change rate (vertical speed)
     */
    static ValidationResult validateVerticalSpeed(double verticalSpeed) {
        // Realistic vertical speed limits (feet per minute)
        // Most GA aircraft: ±2000 fpm
        constexpr double MAX_VERTICAL_SPEED = 4000.0;  // fpm

        if (std::isnan(verticalSpeed) || std::isinf(verticalSpeed)) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_ALTITUDE,
                "Vertical speed is NaN or infinite");
        }

        if (std::abs(verticalSpeed) > MAX_VERTICAL_SPEED) {
            std::ostringstream oss;
            oss << "Vertical speed " << verticalSpeed << " fpm exceeds realistic limits";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_ALTITUDE,
                oss.str(), "vertical_speed");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid vertical speed");
    }
};

// ============================================================================
// AIRSPEED VALIDATOR
// ============================================================================

class AirspeedValidator {
public:
    // Typical airspeed limits (in knots)
    static constexpr double MIN_AIRSPEED = 0.0;
    static constexpr double MAX_AIRSPEED_GENERAL = 200.0;
    static constexpr double MAX_AIRSPEED_JET = 500.0;
    static constexpr double STALL_SPEED_ESTIMATE = 40.0;  // Conservative estimate

    /**
     * Validate indicated airspeed
     */
    static ValidationResult validateIndicatedAirspeed(double ias,
                                                     double maxIAS = MAX_AIRSPEED_GENERAL) {
        if (std::isnan(ias) || std::isinf(ias)) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_AIRSPEED,
                "IAS is NaN or infinite");
        }

        if (ias < MIN_AIRSPEED) {
            std::ostringstream oss;
            oss << "IAS " << ias << " knots is negative";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_AIRSPEED,
                oss.str(), "indicated_airspeed");
        }

        if (ias > maxIAS) {
            std::ostringstream oss;
            oss << "IAS " << ias << " knots exceeds maximum " << maxIAS << " knots";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_AIRSPEED,
                oss.str(), "indicated_airspeed");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid IAS");
    }

    /**
     * Validate true airspeed
     */
    static ValidationResult validateTrueAirspeed(double tas) {
        if (std::isnan(tas) || std::isinf(tas)) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_AIRSPEED,
                "TAS is NaN or infinite");
        }

        if (tas < MIN_AIRSPEED) {
            std::ostringstream oss;
            oss << "TAS " << tas << " knots is negative";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_AIRSPEED,
                oss.str(), "true_airspeed");
        }

        if (tas > 600.0) {  // Supersonic range
            std::ostringstream oss;
            oss << "TAS " << tas << " knots is unrealistic for conventional aircraft";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_AIRSPEED,
                oss.str(), "true_airspeed");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid TAS");
    }

    /**
     * Validate ground speed
     */
    static ValidationResult validateGroundSpeed(double groundSpeed) {
        if (std::isnan(groundSpeed) || std::isinf(groundSpeed)) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_AIRSPEED,
                "Ground speed is NaN or infinite");
        }

        if (groundSpeed < 0.0) {
            std::ostringstream oss;
            oss << "Ground speed " << groundSpeed << " knots is negative";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_AIRSPEED,
                oss.str(), "ground_speed");
        }

        if (groundSpeed > 600.0) {
            std::ostringstream oss;
            oss << "Ground speed " << groundSpeed << " knots is unrealistic";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_AIRSPEED,
                oss.str(), "ground_speed");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid ground speed");
    }
};

// ============================================================================
// HEADING VALIDATOR
// ============================================================================

class HeadingValidator {
public:
    /**
     * Validate magnetic/true heading
     */
    static ValidationResult validateHeading(double heading) {
        if (std::isnan(heading) || std::isinf(heading)) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_HEADING,
                "Heading is NaN or infinite");
        }

        // Normalize to 0-360 range for checking
        double normalized = heading;
        while (normalized < 0.0) normalized += 360.0;
        while (normalized >= 360.0) normalized -= 360.0;

        if (normalized < 0.0 || normalized >= 360.0) {
            std::ostringstream oss;
            oss << "Heading " << heading << " degrees is outside valid range";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_HEADING,
                oss.str(), "heading");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid heading");
    }

    /**
     * Normalize heading to 0-360 range
     */
    static double normalizeHeading(double heading) {
        while (heading < 0.0) heading += 360.0;
        while (heading >= 360.0) heading -= 360.0;
        return heading;
    }

    /**
     * Calculate angular difference between two headings
     * Returns value between -180 and +180 degrees
     */
    static double headingDifference(double heading1, double heading2) {
        heading1 = normalizeHeading(heading1);
        heading2 = normalizeHeading(heading2);

        double diff = heading2 - heading1;

        if (diff > 180.0) diff -= 360.0;
        if (diff < -180.0) diff += 360.0;

        return diff;
    }
};

// ============================================================================
// WEIGHT AND BALANCE VALIDATOR
// ============================================================================

class WeightBalanceValidator {
public:
    struct EnvelopePoint {
        double weight;      // pounds
        double cgPercent;   // percent of mean aerodynamic chord
    };

    /**
     * Validate aircraft weight
     */
    static ValidationResult validateWeight(double weight, double maxWeight) {
        if (std::isnan(weight) || std::isinf(weight)) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WEIGHT,
                "Weight is NaN or infinite");
        }

        if (weight < 0.0) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WEIGHT,
                "Weight cannot be negative");
        }

        if (weight > maxWeight) {
            std::ostringstream oss;
            oss << "Weight " << weight << " lbs exceeds maximum " << maxWeight << " lbs";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WEIGHT,
                oss.str(), "weight");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid weight");
    }

    /**
     * Validate center of gravity position
     */
    static ValidationResult validateCG(double cgPercent, double minCGPercent, double maxCGPercent) {
        if (std::isnan(cgPercent) || std::isinf(cgPercent)) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WEIGHT_BALANCE,
                "CG position is NaN or infinite");
        }

        if (cgPercent < minCGPercent || cgPercent > maxCGPercent) {
            std::ostringstream oss;
            oss << "CG position " << cgPercent << "% MAC is outside envelope ["
                << minCGPercent << "%, " << maxCGPercent << "%]";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WEIGHT_BALANCE,
                oss.str(), "cg_position");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid CG position");
    }

    /**
     * Check if weight/CG combination is within envelope
     * Envelope is defined as a polygon of weight vs CG% points
     */
    static ValidationResult validateWeightBalanceEnvelope(
        double weight, double cgPercent,
        const std::vector<EnvelopePoint>& envelope) {

        if (envelope.size() < 3) {
            return ValidationResult(true, ErrorCode::UNKNOWN_ERROR,
                "Insufficient envelope points for checking");
        }

        // Simple point-in-polygon test (horizontal line intersection)
        int intersections = 0;

        for (size_t i = 0; i < envelope.size(); i++) {
            size_t j = (i + 1) % envelope.size();
            const EnvelopePoint& p1 = envelope[i];
            const EnvelopePoint& p2 = envelope[j];

            // Check if horizontal line at cgPercent intersects segment p1->p2
            if ((p1.cgPercent <= cgPercent && cgPercent < p2.cgPercent) ||
                (p2.cgPercent <= cgPercent && cgPercent < p1.cgPercent)) {

                // Calculate intersection weight
                double t = (cgPercent - p1.cgPercent) / (p2.cgPercent - p1.cgPercent);
                double intersectionWeight = p1.weight + t * (p2.weight - p1.weight);

                if (weight <= intersectionWeight) {
                    intersections++;
                }
            }
        }

        bool isInside = (intersections % 2 == 1);

        if (!isInside) {
            std::ostringstream oss;
            oss << "Weight " << weight << " lbs at CG " << cgPercent
                << "% is outside weight and balance envelope";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WEIGHT_BALANCE,
                oss.str());
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR,
            "Weight and balance within envelope");
    }
};

// ============================================================================
// WAYPOINT VALIDATOR
// ============================================================================

class WaypointValidator {
public:
    /**
     * Validate waypoint structure
     */
    static ValidationResult validateWaypoint(const Waypoint& waypoint) {
        // Validate ID
        if (waypoint.id.empty()) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WAYPOINT,
                "Waypoint ID cannot be empty");
        }

        // Validate coordinates
        ValidationResult coordResult = CoordinateValidator::validateCoordinatePair(
            waypoint.position.latitude, waypoint.position.longitude);
        if (!coordResult.isValid) {
            coordResult.errorCode = ErrorCode::VALIDATION_INVALID_WAYPOINT;
            return coordResult;
        }

        // Validate altitude
        ValidationResult altResult = AltitudeValidator::validateAltitude(waypoint.altitude);
        if (!altResult.isValid) {
            altResult.errorCode = ErrorCode::VALIDATION_INVALID_WAYPOINT;
            return altResult;
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid waypoint");
    }

    /**
     * Validate flight plan waypoint sequence
     */
    static ValidationResult validateWaypointSequence(const std::vector<Waypoint>& waypoints) {
        if (waypoints.size() < 2) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WAYPOINT,
                "Flight plan must have at least departure and arrival");
        }

        // Check each waypoint
        for (size_t i = 0; i < waypoints.size(); i++) {
            ValidationResult result = validateWaypoint(waypoints[i]);
            if (!result.isValid) {
                std::ostringstream oss;
                oss << "Waypoint " << i << " (" << waypoints[i].id << ") is invalid: "
                    << result.message;
                return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WAYPOINT_SEQUENCE,
                    oss.str());
            }
        }

        // Check for duplicate waypoints
        for (size_t i = 0; i < waypoints.size() - 1; i++) {
            for (size_t j = i + 1; j < waypoints.size(); j++) {
                double dist = CoordinateValidator::greatCircleDistance(
                    waypoints[i].position.latitude, waypoints[i].position.longitude,
                    waypoints[j].position.latitude, waypoints[j].position.longitude);

                if (dist < 0.1) {  // Less than 0.1 NM apart
                    std::ostringstream oss;
                    oss << "Waypoints " << i << " and " << j
                        << " are very close (" << dist << " NM apart)";
                    return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WAYPOINT_SEQUENCE,
                        oss.str(), "waypoint_sequence");
                }
            }
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid waypoint sequence");
    }

    /**
     * Validate flight plan leg distance
     */
    static ValidationResult validateLegDistance(double distance,
                                               double maxLegDistance = 500.0) {  // NM
        if (distance < 0.1) {  // Less than 0.1 NM
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WAYPOINT,
                "Waypoint spacing is too small");
        }

        if (distance > maxLegDistance) {
            std::ostringstream oss;
            oss << "Leg distance " << distance << " NM exceeds maximum "
                << maxLegDistance << " NM";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_WAYPOINT,
                oss.str(), "leg_distance");
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid leg distance");
    }
};

// ============================================================================
// FLIGHT PLAN VALIDATOR
// ============================================================================

class FlightPlanValidator {
public:
    /**
     * Validate complete flight plan
     */
    static ValidationResult validateFlightPlan(const FlightPlan& plan) {
        // Validate departure and arrival
        if (plan.departure.empty()) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_FLIGHT_PLAN,
                "Departure airport is not specified");
        }

        if (plan.arrival.empty()) {
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_FLIGHT_PLAN,
                "Arrival airport is not specified");
        }

        // Validate cruise altitude
        ValidationResult altResult = AltitudeValidator::validateAltitude(plan.cruiseAltitude);
        if (!altResult.isValid) {
            altResult.errorCode = ErrorCode::VALIDATION_INVALID_FLIGHT_PLAN;
            return altResult;
        }

        // Validate cruise speed
        ValidationResult speedResult = AirspeedValidator::validateTrueAirspeed(plan.cruiseSpeed);
        if (!speedResult.isValid) {
            speedResult.errorCode = ErrorCode::VALIDATION_INVALID_FLIGHT_PLAN;
            return speedResult;
        }

        // Validate waypoint sequence
        ValidationResult seqResult = WaypointValidator::validateWaypointSequence(plan.waypoints);
        if (!seqResult.isValid) {
            seqResult.errorCode = ErrorCode::VALIDATION_INVALID_FLIGHT_PLAN;
            return seqResult;
        }

        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid flight plan");
    }
};

// ============================================================================
// POINTER AND DATA VALIDATOR
// ============================================================================

class DataValidator {
public:
    /**
     * Validate non-null pointer
     */
    template<typename T>
    static ValidationResult validatePointer(const T* ptr, const std::string& name) {
        if (ptr == nullptr) {
            std::ostringstream oss;
            oss << "Null pointer provided for " << name;
            return ValidationResult(false, ErrorCode::VALIDATION_NULL_POINTER,
                oss.str(), name);
        }
        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid pointer");
    }

    /**
     * Validate non-empty string
     */
    static ValidationResult validateNonEmptyString(const std::string& str, const std::string& name) {
        if (str.empty()) {
            std::ostringstream oss;
            oss << name << " cannot be empty";
            return ValidationResult(false, ErrorCode::VALIDATION_EMPTY_STRING,
                oss.str(), name);
        }
        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid non-empty string");
    }

    /**
     * Validate string length
     */
    static ValidationResult validateStringLength(const std::string& str,
                                                size_t maxLength,
                                                const std::string& name) {
        if (str.length() > maxLength) {
            std::ostringstream oss;
            oss << name << " exceeds maximum length of " << maxLength
                << " characters";
            return ValidationResult(false, ErrorCode::VALIDATION_INVALID_FORMAT,
                oss.str(), name);
        }
        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid string length");
    }

    /**
     * Validate numeric range
     */
    template<typename T>
    static ValidationResult validateRange(T value, T minValue, T maxValue,
                                         const std::string& name) {
        if (value < minValue || value > maxValue) {
            std::ostringstream oss;
            oss << name << " value " << value << " is outside range ["
                << minValue << ", " << maxValue << "]";
            return ValidationResult(false, ErrorCode::VALIDATION_OUT_OF_RANGE,
                oss.str(), name);
        }
        return ValidationResult(true, ErrorCode::UNKNOWN_ERROR, "Valid range");
    }
};

} // namespace AICopilot

#endif // VALIDATION_FRAMEWORK_HPP

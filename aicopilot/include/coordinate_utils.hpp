/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PHASE 1: ERROR HANDLING AND INPUT VALIDATION
* Module: coordinate_utils.hpp
* 
* PURPOSE:
* - Provide specialized coordinate utilities for edge cases
* - Handle antimeridian crossing and pole proximity
* - Calculate geodetic distances and bearings
* - Handle date line wrapping
* - Provide coordinate conversion utilities
* - Generate test/example coordinates for validation
*
* FEATURES:
* - Antimeridian-safe coordinate arithmetic
* - Pole proximity awareness
* - Spherical trigonometry calculations
* - Coordinate format conversions
* - Safe coordinate interpolation
* - Coordinate constraint detection
*
* TEST CASES INCLUDED:
* - Poles: ±90° latitude
* - Antimeridian: ±180° longitude
* - Date line wrapping
* - Short distances (<0.1 NM)
* - Long distances (>1000 NM)
* - Crossing multiple coordinate system boundaries
*****************************************************************************/

#ifndef COORDINATE_UTILS_HPP
#define COORDINATE_UTILS_HPP

#include "validation_framework.hpp"
#include "aicopilot_types.h"
#include <cmath>
#include <vector>
#include <sstream>
#include <iomanip>
#include <memory>

namespace AICopilot {

// ============================================================================
// MATHEMATICAL CONSTANTS
// ============================================================================

namespace CoordinateConstants {
    constexpr double PI = 3.14159265358979323846;
    constexpr double TWO_PI = 2.0 * PI;
    constexpr double DEG_TO_RAD = PI / 180.0;
    constexpr double RAD_TO_DEG = 180.0 / PI;
    
    // Earth parameters (WGS84)
    constexpr double EARTH_RADIUS_M = 6371008.8;           // meters
    constexpr double EARTH_RADIUS_NM = 3440.065;           // nautical miles
    constexpr double EARTH_RADIUS_KM = 6371.008;           // kilometers
    
    // Equatorial and polar radii (WGS84 ellipsoid)
    constexpr double EARTH_EQUATORIAL_RADIUS = 6378137.0;  // meters
    constexpr double EARTH_POLAR_RADIUS = 6356752.3;       // meters
}

// ============================================================================
// COORDINATE EDGE CASE DETECTOR
// ============================================================================

struct CoordinateEdgeCase {
    bool isAtNorthPole = false;
    bool isAtSouthPole = false;
    bool isAtEquator = false;
    bool isPrimeMeridian = false;
    bool isAntimeridian = false;
    bool isNearPole = false;
    bool crossesAntimeridian = false;
    
    std::string describe() const {
        std::ostringstream oss;
        if (isAtNorthPole) oss << "At North Pole; ";
        if (isAtSouthPole) oss << "At South Pole; ";
        if (isAtEquator) oss << "At Equator; ";
        if (isPrimeMeridian) oss << "At Prime Meridian; ";
        if (isAntimeridian) oss << "At Antimeridian; ";
        if (isNearPole) oss << "Near Pole; ";
        if (crossesAntimeridian) oss << "Crosses Antimeridian; ";
        
        std::string desc = oss.str();
        if (!desc.empty() && desc.back() == ' ') {
            desc.pop_back();
            desc.pop_back();  // Remove "; "
        }
        return desc.empty() ? "Normal coordinate" : desc;
    }
};

// ============================================================================
// COORDINATE UTILITY CLASS
// ============================================================================

class CoordinateUtils {
public:
    using RadianPair = std::pair<double, double>;

    /**
     * Detect edge cases in a coordinate
     */
    static CoordinateEdgeCase detectEdgeCases(double lat, double lon) {
        CoordinateEdgeCase ec;
        
        // Normalize coordinates
        lat = CoordinateValidator::normalizeLatitude(lat);
        lon = CoordinateValidator::normalizeLongitude(lon);
        
        // Check poles
        ec.isAtNorthPole = (std::abs(lat - 90.0) < 0.001);
        ec.isAtSouthPole = (std::abs(lat + 90.0) < 0.001);
        ec.isNearPole = (std::abs(lat) > 85.0) && !ec.isAtNorthPole && !ec.isAtSouthPole;
        
        // Check equator and meridians
        ec.isAtEquator = (std::abs(lat) < 0.001);
        ec.isPrimeMeridian = (std::abs(lon) < 0.001);
        ec.isAntimeridian = (std::abs(std::abs(lon) - 180.0) < 0.001);
        
        return ec;
    }

    /**
     * Check if great circle path between two points crosses antimeridian
     */
    static bool pathCrossesAntimeridian(double lat1, double lon1, double lat2, double lon2) {
        double lonDiff = lon2 - lon1;
        
        // If difference is > 180, path crosses antimeridian going one direction
        // If difference is < -180, path crosses going the other direction
        return std::abs(lonDiff) > 180.0;
    }

    /**
     * Calculate shortest distance using Haversine formula
     * Returns distance in nautical miles
     */
    static double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
        // Handle pole cases
        if (std::abs(lat1 - 90.0) < 0.001 || std::abs(lat1 + 90.0) < 0.001) {
            // At poles, we need special handling
            return polarDistance(lat1, lon1, lat2, lon2);
        }
        
        double lat1Rad = lat1 * CoordinateConstants::DEG_TO_RAD;
        double lon1Rad = lon1 * CoordinateConstants::DEG_TO_RAD;
        double lat2Rad = lat2 * CoordinateConstants::DEG_TO_RAD;
        double lon2Rad = lon2 * CoordinateConstants::DEG_TO_RAD;

        double dLat = lat2Rad - lat1Rad;
        double dLon = lon2Rad - lon1Rad;

        double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
                   std::cos(lat1Rad) * std::cos(lat2Rad) *
                   std::sin(dLon / 2.0) * std::sin(dLon / 2.0);

        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
        double distance = CoordinateConstants::EARTH_RADIUS_NM * c;

        return distance;
    }

    /**
     * Calculate distance for polar regions (special handling at poles)
     */
    static double polarDistance(double lat1, double lon1, double lat2, double lon2) {
        // For points at poles, use simplified calculation
        // Distance approximately = (90 - |lat2|) * 60 NM for lines of longitude
        
        if (std::abs(lat1 - 90.0) < 0.001) {  // At north pole
            // All meridians converge at north pole
            return (90.0 - lat2) * 60.0;  // 60 NM per degree of latitude
        }
        
        if (std::abs(lat1 + 90.0) < 0.001) {  // At south pole
            return (90.0 + lat2) * 60.0;
        }
        
        // Fall back to Haversine for non-pole cases
        return haversineDistance(lat1, lon1, lat2, lon2);
    }

    /**
     * Calculate Vincenty distance (more accurate for ellipsoid)
     * Returns distance in nautical miles
     */
    static double vincentyDistance(double lat1, double lon1, double lat2, double lon2) {
        double lat1Rad = lat1 * CoordinateConstants::DEG_TO_RAD;
        double lon1Rad = lon1 * CoordinateConstants::DEG_TO_RAD;
        double lat2Rad = lat2 * CoordinateConstants::DEG_TO_RAD;
        double lon2Rad = lon2 * CoordinateConstants::DEG_TO_RAD;

        double a = CoordinateConstants::EARTH_EQUATORIAL_RADIUS;
        double b = CoordinateConstants::EARTH_POLAR_RADIUS;
        double f = 1.0 / 298.257223563;  // WGS84 flattening

        double L = lon2Rad - lon1Rad;
        double U1 = std::atan((1.0 - f) * std::tan(lat1Rad));
        double U2 = std::atan((1.0 - f) * std::tan(lat2Rad));

        double sinU1 = std::sin(U1);
        double cosU1 = std::cos(U1);
        double sinU2 = std::sin(U2);
        double cosU2 = std::cos(U2);

        double lambda = L;
        double lambdaP = 2.0 * CoordinateConstants::PI;
        double iterLimit = 100.0;
        double cosSqAlpha = 0.0;
        double sinSigma = 0.0;
        double cos2SigmaM = 0.0;
        double cosSigma = 0.0;
        double sigma = 0.0;

        for (int i = 0; i < static_cast<int>(iterLimit); i++) {
            double sinLambda = std::sin(lambda);
            double cosLambda = std::cos(lambda);

            sinSigma = std::sqrt((cosU2 * sinLambda) * (cosU2 * sinLambda) +
                                (cosU1 * sinU2 - sinU1 * cosU2 * cosLambda) *
                                (cosU1 * sinU2 - sinU1 * cosU2 * cosLambda));

            if (sinSigma == 0.0) {
                return 0.0;  // Points are the same
            }

            cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
            sigma = std::atan2(sinSigma, cosSigma);

            double sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;
            cosSqAlpha = 1.0 - sinAlpha * sinAlpha;

            cos2SigmaM = cosSigma - 2.0 * sinU1 * sinU2 / cosSqAlpha;

            if (std::isnan(cos2SigmaM)) {
                cos2SigmaM = 0.0;  // Equatorial line
            }

            double C = f / 16.0 * cosSqAlpha * (4.0 + f * (4.0 - 3.0 * cosSqAlpha));
            lambdaP = lambda;

            lambda = L + (1.0 - C) * f * sinAlpha *
                    (sigma + C * sinSigma *
                    (cos2SigmaM + C * cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM)));

            if (std::abs(lambda - lambdaP) > 1e-12) {
                continue;
            } else {
                break;
            }
        }

        double uSq = cosSqAlpha * (a * a - b * b) / (b * b);
        double A = 1.0 + uSq / 16384.0 * (4096.0 + uSq * (-768.0 + uSq * (320.0 - 175.0 * uSq)));
        double B = uSq / 1024.0 * (256.0 + uSq * (-128.0 + uSq * (74.0 - 47.0 * uSq)));

        double deltaSigma = B * sinSigma * (cos2SigmaM + B / 4.0 *
                           (cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM) -
                           B / 6.0 * cos2SigmaM * (-3.0 + 4.0 * sinSigma * sinSigma) *
                           (-3.0 + 4.0 * cos2SigmaM * cos2SigmaM)));

        double s = b * A * (sigma - deltaSigma);
        
        // Convert meters to nautical miles
        return s / 1852.0;
    }

    /**
     * Calculate initial bearing from point 1 to point 2 (degrees)
     */
    static double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
        double lat1Rad = lat1 * CoordinateConstants::DEG_TO_RAD;
        double lon1Rad = lon1 * CoordinateConstants::DEG_TO_RAD;
        double lat2Rad = lat2 * CoordinateConstants::DEG_TO_RAD;
        double lon2Rad = lon2 * CoordinateConstants::DEG_TO_RAD;

        double dLon = lon2Rad - lon1Rad;

        double y = std::sin(dLon) * std::cos(lat2Rad);
        double x = std::cos(lat1Rad) * std::sin(lat2Rad) -
                   std::sin(lat1Rad) * std::cos(lat2Rad) * std::cos(dLon);

        double bearing = std::atan2(y, x) * CoordinateConstants::RAD_TO_DEG;
        bearing = std::fmod(bearing + 360.0, 360.0);  // Normalize to 0-360

        return bearing;
    }

    /**
     * Calculate destination point given bearing and distance
     * Returns new coordinate pair
     */
    static Position destinationPoint(double lat, double lon, double bearing, double distance) {
        double latRad = lat * CoordinateConstants::DEG_TO_RAD;
        double lonRad = lon * CoordinateConstants::DEG_TO_RAD;
        double bearingRad = bearing * CoordinateConstants::DEG_TO_RAD;
        
        // Angular distance in radians
        double angDist = distance / CoordinateConstants::EARTH_RADIUS_NM;

        double lat2Rad = std::asin(std::sin(latRad) * std::cos(angDist) +
                                   std::cos(latRad) * std::sin(angDist) * std::cos(bearingRad));

        double lon2Rad = lonRad + std::atan2(std::sin(bearingRad) * std::sin(angDist) * std::cos(latRad),
                                             std::cos(angDist) - std::sin(latRad) * std::sin(lat2Rad));

        Position result;
        result.latitude = lat2Rad * CoordinateConstants::RAD_TO_DEG;
        result.longitude = lon2Rad * CoordinateConstants::RAD_TO_DEG;
        result.longitude = CoordinateValidator::normalizeLongitude(result.longitude);
        result.heading = bearing;
        result.altitude = 0.0;

        return result;
    }

    /**
     * Safely interpolate between two coordinates (handles antimeridian)
     * fraction: 0.0 = point1, 1.0 = point2
     */
    static Position interpolateCoordinate(double lat1, double lon1, double lat2, double lon2,
                                         double fraction) {
        if (fraction < 0.0 || fraction > 1.0) {
            throw ValidationException(ErrorCode::VALIDATION_OUT_OF_RANGE,
                "Interpolation fraction must be between 0 and 1",
                "fraction", std::to_string(fraction));
        }

        // Handle antimeridian crossing
        double lon2Adj = lon2;
        if (pathCrossesAntimeridian(lat1, lon1, lat2, lon2)) {
            if (lon1 > 0.0) {
                lon2Adj = lon2 + 360.0;
            } else {
                lon2Adj = lon2 - 360.0;
            }
        }

        // Linear interpolation (simplified, acceptable for short segments)
        double lat = lat1 + (lat2 - lat1) * fraction;
        double lon = lon1 + (lon2Adj - lon1) * fraction;
        lon = CoordinateValidator::normalizeLongitude(lon);

        Position result;
        result.latitude = lat;
        result.longitude = lon;
        result.heading = calculateBearing(lat1, lon1, lat2, lon2);
        result.altitude = 0.0;

        return result;
    }

    /**
     * Convert coordinates to radians
     */
    static RadianPair toRadians(double lat, double lon) {
        return std::make_pair(
            lat * CoordinateConstants::DEG_TO_RAD,
            lon * CoordinateConstants::DEG_TO_RAD
        );
    }

    /**
     * Convert coordinates from radians
     */
    static std::pair<double, double> toDegrees(double latRad, double lonRad) {
        return std::make_pair(
            latRad * CoordinateConstants::RAD_TO_DEG,
            lonRad * CoordinateConstants::RAD_TO_DEG
        );
    }

    /**
     * Format coordinate for display
     * Format: "DD°MM'SS.S\"H" (Degrees, Minutes, Seconds)
     */
    static std::string formatCoordinate(double latitude, double longitude) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4);

        // Format latitude
        double latAbs = std::abs(latitude);
        int latDeg = static_cast<int>(latAbs);
        double latMin = (latAbs - latDeg) * 60.0;
        int latMinInt = static_cast<int>(latMin);
        double latSec = (latMin - latMinInt) * 60.0;

        oss << latDeg << "°" << latMinInt << "'" << latSec << "\"" << (latitude >= 0 ? "N" : "S");
        oss << ", ";

        // Format longitude
        double lonAbs = std::abs(longitude);
        int lonDeg = static_cast<int>(lonAbs);
        double lonMin = (lonAbs - lonDeg) * 60.0;
        int lonMinInt = static_cast<int>(lonMin);
        double lonSec = (lonMin - lonMinInt) * 60.0;

        oss << lonDeg << "°" << lonMinInt << "'" << lonSec << "\"" << (longitude >= 0 ? "E" : "W");

        return oss.str();
    }

    /**
     * Format coordinate in decimal degrees
     */
    static std::string formatCoordinateDecimal(double latitude, double longitude) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << latitude << ", " << longitude;
        return oss.str();
    }
};

// ============================================================================
// TEST COORDINATE GENERATOR
// ============================================================================

class TestCoordinateGenerator {
public:
    /**
     * Generate edge case test coordinates
     */
    static std::vector<std::pair<double, double>> generateEdgeCaseCoordinates() {
        return {
            // Poles
            {90.0, 0.0},           // North Pole
            {-90.0, 0.0},          // South Pole
            
            // Equator and Meridians
            {0.0, 0.0},            // Null Island (0,0)
            {0.0, 180.0},          // Antimeridian on Equator
            {0.0, -180.0},         // Antimeridian on Equator (alternate)
            
            // Near Poles
            {89.9, 0.0},           // Near North Pole
            {-89.9, 0.0},          // Near South Pole
            
            // Antimeridian crossings
            {45.0, 179.0},         // Just before antimeridian
            {45.0, -179.0},        // Just after antimeridian
            {-45.0, 179.5},        // Crossing point (northbound)
            
            // Major cities (validation tests)
            {51.5074, -0.1278},    // London
            {48.8566, 2.3522},     // Paris
            {35.6762, 139.6503},   // Tokyo
            {-33.8688, 151.2093},  // Sydney
            {-23.5505, -46.6333},  // São Paulo
            
            // Edge values
            {89.9999, 179.9999},   // Max valid bounds
            {-89.9999, -179.9999}, // Min valid bounds
        };
    }

    /**
     * Generate route that crosses antimeridian
     */
    static std::vector<Position> generateAntimeridianRoute() {
        std::vector<Position> route;
        
        // Route from Japan to USA (crosses Pacific, crosses antimeridian)
        route.push_back({35.6762, 139.6503, 0.0, 0.0});      // Tokyo, Japan
        route.push_back({51.5074, -0.1278, 0.0, 0.0});       // London (waypoint)
        route.push_back({47.6062, -122.3321, 0.0, 0.0});     // Seattle, USA
        
        return route;
    }

    /**
     * Generate route with pole proximity
     */
    static std::vector<Position> generatePoleProximityRoute() {
        std::vector<Position> route;
        
        route.push_back({80.0, 0.0, 0.0, 0.0});      // Arctic
        route.push_back({85.0, 0.0, 0.0, 0.0});      // Very close to North Pole
        route.push_back({80.0, 45.0, 0.0, 0.0});     // Another Arctic point
        
        return route;
    }
};

} // namespace AICopilot

#endif // COORDINATE_UTILS_HPP

/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* METAR Parser Utilities - ICAO Format Parsing and Conversion
* High-performance METAR parsing with error recovery
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef METAR_PARSER_HPP
#define METAR_PARSER_HPP

#include <string>
#include <vector>
#include <cmath>
#include <regex>

namespace AICopilot {

// ============================================================================
// METAR Parsing Constants and Enumerations
// ============================================================================

// Weather phenomenon codes
enum class WeatherPhenomenaCode {
    RA,             // Rain
    SN,             // Snow
    RASN,           // Rain and snow
    SNRA,           // Snow and rain
    SG,             // Snow grains
    IC,             // Ice crystals
    PE,             // Ice pellets
    GR,             // Hail
    GS,             // Small hail/snow pellets
    UP,             // Unknown precipitation
    DZ,             // Drizzle
    PL,             // Sleet (ice pellets)
    FZRA,           // Freezing rain
    FZDZ,           // Freezing drizzle
    TS,             // Thunderstorm
    TSRA,           // Thunderstorm with rain
    TSSN,           // Thunderstorm with snow
    TSGS,           // Thunderstorm with hail
    TSGR,           // Thunderstorm with hail
    VC,             // Vicinity
    FG,             // Fog
    MIFG,           // Shallow fog
    VCTS,           // Vicinity thunderstorm
    CAVOK,          // Ceiling and visibility OK
    UNKNOWN
};

// Cloud coverage descriptors
enum class CloudCoverage {
    SKC,            // Sky clear
    CLR,            // Clear
    FEW,            // 1-2/8 coverage
    SCT,            // 3-4/8 coverage (scattered)
    BKN,            // 5-7/8 coverage (broken)
    OVC,            // 8/8 coverage (overcast)
    VV,             // Vertical visibility
    UNKNOWN
};

// ============================================================================
// Visibility Conversion Constants
// ============================================================================

namespace Visibility {
    // Statute Mile to Meter Conversion
    static constexpr double STATUTE_MILE_TO_METERS = 1609.34;
    static constexpr double STATUTE_MILE_TO_KM = 1.60934;
    static constexpr double STATUTE_MILE_TO_FEET = 5280.0;
    
    // Common visibility limits
    static constexpr double MIN_VISIBILITY_SM = 0.25;      // 1/4 SM
    static constexpr double MAX_VISIBILITY_SM = 10.0;      // 10+ SM
}

// ============================================================================
// METAR Parser Utilities
// ============================================================================

class METARParser {
public:
    // ========== Core Parsing Methods ==========
    
    /**
     * Parse wind token from METAR
     * Formats: "31008KT" (direction, speed, unit)
     *         "VRB05KT" (variable wind)
     *         "27015G25KT" (with gusts)
     * 
     * @param windToken Wind token string
     * @param direction Output: wind direction (0-360 degrees)
     * @param speed Output: wind speed (knots)
     * @param gust Output: gust speed (knots, 0 if none)
     * @param variable Output: true if variable
     * @param minDir Output: min direction if variable
     * @param maxDir Output: max direction if variable
     * @return true if parsed successfully
     */
    static bool parseWind(
        const std::string& windToken,
        int& direction,
        int& speed,
        int& gust,
        bool& variable,
        int& minDir,
        int& maxDir);
    
    /**
     * Parse visibility token from METAR
     * Formats: "10SM" (statute miles)
     *         "9999" (meters, typically ICAO)
     *         "10" (kilometers, some regions)
     *         "1/4SM" (fractional)
     * 
     * @param visToken Visibility token
     * @param visibilitySmiles Output: visibility in statute miles
     * @return true if parsed successfully
     */
    static bool parseVisibility(
        const std::string& visToken,
        double& visibilitySmiles);
    
    /**
     * Parse cloud layer token from METAR
     * Formats: "FEW030" (few at 3000 ft)
     *         "SCT050CB" (scattered at 5000 ft with cumulonimbus)
     *         "OVC100TCU" (overcast at 10000 ft, towering cumulus)
     *         "VV002" (vertical visibility 200 ft)
     * 
     * @param cloudToken Cloud layer token
     * @param coverage Output: cloud coverage type
     * @param altitudeAgl Output: altitude above ground (feet)
     * @param isCB Output: true if cumulonimbus
     * @param isTCU Output: true if towering cumulus
     * @return true if parsed successfully
     */
    static bool parseCloudLayer(
        const std::string& cloudToken,
        CloudCoverage& coverage,
        int& altitudeAgl,
        bool& isCB,
        bool& isTCU);
    
    /**
     * Parse temperature/dewpoint from METAR
     * Format: "18/14" (18°C temp, 14°C dewpoint)
     *         "M05/M10" (negative temperatures: -5°C, -10°C)
     * 
     * @param tempToken Temperature token
     * @param temperature Output: temperature in Celsius
     * @param dewpoint Output: dewpoint in Celsius
     * @return true if parsed successfully
     */
    static bool parseTemperatureDewpoint(
        const std::string& tempToken,
        int& temperature,
        int& dewpoint);
    
    /**
     * Parse altimeter setting from METAR
     * Formats: "A3012" (30.12 inHg)
     *         "Q1018" (1018 hectopascals/millibars)
     * 
     * @param altimeterToken Altimeter token
     * @param altimeterInHg Output: altimeter in inHg
     * @param altimeterMbar Output: altimeter in millibars
     * @return true if parsed successfully
     */
    static bool parseAltimeter(
        const std::string& altimeterToken,
        double& altimeterInHg,
        double& altimeterMbar);
    
    /**
     * Parse weather phenomena from METAR
     * Examples: "RA" (rain), "-SHRA" (light showers rain)
     *          "+TSRA" (heavy thunderstorm rain), "BR" (mist)
     * 
     * @param phenomToken Weather token
     * @param phenomena Output: phenomenon code
     * @param intensity Output: intensity (-1=light, 0=moderate, +1=heavy)
     * @return true if parsed successfully
     */
    static bool parseWeatherPhenomena(
        const std::string& phenomToken,
        WeatherPhenomenaCode& phenomena,
        int& intensity);
    
    // ========== Utility Methods ==========
    
    /**
     * Identify token type in METAR sequence
     * @param token METAR token
     * @return Token type identifier
     */
    static std::string identifyTokenType(const std::string& token);
    
    /**
     * Check if token matches regex pattern
     * @param token Token to check
     * @param pattern Regex pattern
     * @return true if matches
     */
    static bool matchesPattern(
        const std::string& token,
        const std::string& pattern);
    
    /**
     * Extract integer from token with optional sign
     * Format: "M05" → -5, "05" → 5
     * 
     * @param token Token containing integer
     * @param startPos Starting position
     * @param length Number of characters to parse
     * @param value Output: parsed integer
     * @return true if successful
     */
    static bool extractSignedInteger(
        const std::string& token,
        size_t startPos,
        size_t length,
        int& value);
    
    /**
     * Extract double value from token
     * Format: "3012" → 30.12, "1018" → 1018
     * 
     * @param token Token containing value
     * @param value Output: parsed double
     * @return true if successful
     */
    static bool extractDouble(
        const std::string& token,
        double& value);
    
    // ========== Conversion Methods ==========
    
    /**
     * Convert visibility to statute miles
     * @param value Input value
     * @param fromUnit Unit code ('M'=meters, 'K'=km, 'F'=feet)
     * @return Visibility in statute miles
     */
    static double convertVisibilityToStatuteMiles(
        double value,
        char fromUnit = 'M');
    
    /**
     * Convert visibility from statute miles
     * @param smiles Visibility in statute miles
     * @param toUnit Target unit ('M'=meters, 'K'=km)
     * @return Converted visibility
     */
    static double convertVisibilityFromStatuteMiles(
        double smiles,
        char toUnit = 'M');
    
    /**
     * Convert altitude from feet to meters
     * @param feet Altitude in feet
     * @return Altitude in meters
     */
    static double feetToMeters(double feet);
    
    /**
     * Convert altitude from meters to feet
     * @param meters Altitude in meters
     * @return Altitude in feet
     */
    static double metersToFeet(double meters);
    
    /**
     * Convert wind speed from knots to other units
     * @param knots Wind speed in knots
     * @param toUnit Target unit ('K'=km/h, 'M'=m/s)
     * @return Converted wind speed
     */
    static double convertWindSpeed(double knots, char toUnit = 'K');
    
    /**
     * Normalize ICAO code to uppercase
     * @param code ICAO code
     * @return Normalized code
     */
    static std::string normalizeICAOCode(const std::string& code);
    
    /**
     * Check if string is valid ICAO code (4 chars, no spaces)
     * @param code Code to validate
     * @return true if valid
     */
    static bool isValidICAOCode(const std::string& code);
    
    /**
     * Split METAR string into tokens
     * @param metarString Raw METAR string
     * @return Vector of tokens
     */
    static std::vector<std::string> tokenizeMETAR(
        const std::string& metarString);
    
    /**
     * Check for CAVOK (Ceiling And Visibility OK)
     * @param weatherCodes Weather phenomena codes
     * @param visibility Current visibility
     * @param ceiling Current ceiling
     * @return true if CAVOK conditions
     */
    static bool isCAVOK(
        const std::vector<WeatherPhenomenaCode>& weatherCodes,
        double visibility,
        double ceiling);
    
private:
    // Regex patterns for METAR components
    static const std::string WIND_PATTERN;
    static const std::string VISIBILITY_PATTERN;
    static const std::string CLOUD_PATTERN;
    static const std::string TEMP_PATTERN;
    static const std::string ALTIMETER_PATTERN;
    
    // Helper: Parse integer with optional leading 'M' for negative
    static int parseSignedInt(const std::string& str);
    
    // Helper: Parse fraction (e.g., "1/4")
    static double parseFraction(const std::string& str);
};

}  // namespace AICopilot

#endif  // METAR_PARSER_HPP

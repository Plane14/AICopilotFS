/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Weather Data System - METAR/TAF Parsing and Weather Management
* Production-ready weather data structures and parsing algorithms
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <chrono>
#include <mutex>
#include <memory>

namespace AICopilot {

// ============================================================================
// Weather Data Enumerations
// ============================================================================

enum class WeatherConditionType {
    CLEAR,           // SKC or CLR
    FEW,             // FEW (1-2 oktas)
    SCATTERED,       // SCT (3-4 oktas)
    BROKEN,          // BKN (5-7 oktas)
    OVERCAST,        // OVC (8 oktas)
    THUNDERSTORM,    // TS present
    ICE,             // Icing conditions
    UNKNOWN
};

enum class PrecipitationType {
    NONE,
    RAIN,
    SNOW,
    SLEET,
    FREEZING_RAIN,
    ICE_PELLETS,
    MIXED
};

// ============================================================================
// Weather Data Structures
// ============================================================================

/**
 * Cloud Layer Information
 */
struct CloudLayer {
    WeatherConditionType coverage;  // FEW, SCT, BKN, OVC
    int altitudeAgl;                 // feet above ground level
    bool isCumulonimbus;             // CB - thunderstorm clouds
    bool isToweringCumulus;          // TCU
    
    int getCoveragePercentage() const;
};

/**
 * Wind Information
 */
struct WindData {
    int directionDegrees;   // 0-360, from which wind is coming
    int speedKnots;         // Wind speed in knots
    int gustKnots;          // Gust speed (0 if no gusts)
    bool isVariable;        // Variable wind direction
    int variableMinDir;     // Minimum direction if variable
    int variableMaxDir;     // Maximum direction if variable
    
    double getSpeedMPS() const;         // Meters per second
    double getSpeedKMH() const;         // Kilometers per hour
};

/**
 * Primary Weather Data Structure
 */
struct WeatherData {
    // Station information
    std::string icaoCode;
    long timestampUnix;  // Unix timestamp of observation
    
    // Temperature and dewpoint
    double temperatureCelsius;
    double dewpointCelsius;
    double getTemperatureFahrenheit() const;
    double getRelativeHumidity() const;  // Calculated from temp/dewpoint
    
    // Wind conditions
    WindData wind;
    
    // Visibility
    double visibilityStatuteMiles;
    double getVisibilityMeters() const;
    double getVisibilityKilometers() const;
    
    // Cloud layers and ceiling
    std::vector<CloudLayer> cloudLayers;
    double ceilingFeet;                  // Lowest OVC/BKN altitude
    WeatherConditionType skyCondition;
    
    // Pressure
    double altimeterSettingInHg;
    double altimeterSettingMbar;
    
    // Weather phenomena
    PrecipitationType precipitation;
    std::vector<std::string> weatherCodes;  // RA, SN, CAVOK, etc.
    
    // Hazardous conditions
    bool isIcingCondition;
    bool hasThunderstorm;
    bool hasFreezingRain;
    bool hasTurbulence;
    
    // Validity flags
    bool isValid;
    std::string parseError;
    
    // Utility methods
    double getFlightCategory() const;  // Returns 0-4 for MVFR/VFR/LIFR/IFR
    bool isSuitableForVFR() const;
    bool isSuitableForIFR() const;
    bool isSuitableForLanding() const;
    
    // Cache metadata
    std::chrono::system_clock::time_point cacheTime;
};

/**
 * TAF (Terminal Aerodrome Forecast) Entry
 */
struct TAFEntry {
    long validFromUnix;       // Start of validity period
    long validToUnix;         // End of validity period
    std::string changeType;   // BECMG, TEMPO
    WeatherData forecastData;
};

/**
 * TAF Report
 */
struct TAFReport {
    std::string icaoCode;
    long issuedTimeUnix;
    long validFromUnix;
    long validToUnix;
    std::vector<TAFEntry> forecasts;
    bool isValid;
    std::string parseError;
};

// ============================================================================
// Weather Database Class
// ============================================================================

/**
 * Production-Ready Weather Database
 * Manages weather data, METAR/TAF parsing, caching with TTL
 */
class WeatherDatabase {
public:
    // Cache configuration
    static constexpr int MAX_CACHE_ENTRIES = 500;
    static constexpr int CACHE_TTL_SECONDS = 3600;  // 1 hour
    static constexpr int MAX_CACHE_SIZE_MB = 10;
    
    WeatherDatabase();
    ~WeatherDatabase();
    
    // ========== Main API ==========
    
    /**
     * Initialize weather database
     * @return true if successful
     */
    bool initialize();
    
    /**
     * Shutdown weather database
     */
    void shutdown();
    
    /**
     * Get weather at specified airport
     * @param icaoCode Airport ICAO code (e.g., "KJFK")
     * @param timeSeconds Optional time (defaults to current time)
     * @return WeatherData structure
     */
    WeatherData GetWeatherAt(const std::string& icaoCode, long timeSeconds = 0);
    
    /**
     * Parse METAR string into WeatherData
     * @param metarString Raw METAR string
     * @return Parsed WeatherData
     */
    WeatherData ParseMETAR(const std::string& metarString);
    
    /**
     * Parse TAF string into forecast data
     * @param tafString Raw TAF string
     * @return Vector of forecast entries
     */
    std::vector<WeatherData> ParseTAF(const std::string& tafString);
    
    // ========== Accessor Methods ==========
    
    /**
     * Get visibility in statute miles
     * @param icaoCode Airport ICAO code
     * @return Visibility in statute miles
     */
    double GetVisibility(const std::string& icaoCode);
    
    /**
     * Get ceiling in feet
     * @param icaoCode Airport ICAO code
     * @return Ceiling altitude in feet
     */
    double GetCeiling(const std::string& icaoCode);
    
    /**
     * Get wind data
     * @param icaoCode Airport ICAO code
     * @return Wind data (direction, speed, gust)
     */
    WindData GetWindData(const std::string& icaoCode);
    
    /**
     * Get temperature in Celsius
     * @param icaoCode Airport ICAO code
     * @return Temperature in Celsius
     */
    double GetTemperature(const std::string& icaoCode);
    
    /**
     * Get dewpoint in Celsius
     * @param icaoCode Airport ICAO code
     * @return Dewpoint in Celsius
     */
    double GetDewpoint(const std::string& icaoCode);
    
    /**
     * Get altimeter setting
     * @param icaoCode Airport ICAO code
     * @return Altimeter setting in inHg
     */
    double GetAltimeter(const std::string& icaoCode);
    
    /**
     * Check for hazardous conditions
     * @param icaoCode Airport ICAO code
     * @return true if icing, thunderstorm, or other hazards present
     */
    bool HasHazardousConditions(const std::string& icaoCode);
    
    /**
     * Get precipitation type
     * @param icaoCode Airport ICAO code
     * @return Precipitation type
     */
    PrecipitationType GetPrecipitationType(const std::string& icaoCode);
    
    // ========== Cache Management ==========
    
    /**
     * Clear all cached weather data
     */
    void ClearCache();
    
    /**
     * Update weather cache manually
     * @param icaoCode Airport ICAO code
     * @param metarString METAR string
     * @return true if successful
     */
    bool UpdateCache(const std::string& icaoCode, const std::string& metarString);
    
    /**
     * Get cache size in bytes
     * @return Number of bytes used by cache
     */
    size_t GetCacheSize() const;
    
    /**
     * Get number of cached entries
     * @return Cache entry count
     */
    int GetCacheEntryCount() const;
    
    /**
     * Refresh expired entries
     */
    void RefreshExpiredEntries();
    
private:
    // ========== Internal Structures ==========
    
    struct CachedWeather {
        WeatherData data;
        std::chrono::system_clock::time_point cacheTime;
        int accessCount;
        size_t estimatedSize;
    };
    
    // ========== Member Variables ==========
    
    std::unordered_map<std::string, CachedWeather> weatherCache_;
    mutable std::mutex cacheMutex_;
    size_t totalCacheSize_;
    
    // ========== Parsing Helpers ==========
    
    void parseStationId(const std::string& token, WeatherData& wx);
    void parseDateTime(const std::string& token, WeatherData& wx);
    void parseWind(const std::string& token, WeatherData& wx);
    void parseVisibility(const std::string& token, WeatherData& wx);
    void parseWeatherPhenomena(const std::string& token, WeatherData& wx);
    void parseCloudLayers(const std::string& token, WeatherData& wx);
    void parseTemperatureDewpoint(const std::string& token, WeatherData& wx);
    void parseAltimeter(const std::string& token, WeatherData& wx);
    void parseRemark(const std::string& remark, WeatherData& wx);
    
    // ========== Helper Methods ==========
    
    WeatherData GetDefaultWeather(const std::string& icaoCode);
    bool IsExpired(const CachedWeather& cached) const;
    void EvictOldestEntry();
    void DeriveConditions(WeatherData& wx);
    double CalculateCeiling(const std::vector<CloudLayer>& layers) const;
    double CalculateRelativeHumidity(double tempC, double dewpointC) const;
    
    // ========== Sample Data ==========
    
    void InitializeSampleAirports();
};

}  // namespace AICopilot

#endif  // WEATHER_DATA_H

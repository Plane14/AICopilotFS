/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Weather Database - METAR parsing and weather API integration
* Provides real-time weather conditions from aviation weather sources
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef WEATHER_DATABASE_HPP
#define WEATHER_DATABASE_HPP

#include "aicopilot_types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <mutex>

namespace AICopilot {

/**
 * Cloud layer information from METAR
 */
struct CloudLayer {
    std::string coverage;       // SKC, CLR, FEW, SCT, BKN, OVC
    int altitude;               // feet AGL
    std::string cloudType;      // CB, TCU, or empty (from remarks)
    
    int getCoveragePercentage() const {
        if (coverage == "SKC" || coverage == "CLR") return 0;
        if (coverage == "FEW") return 25;
        if (coverage == "SCT") return 50;
        if (coverage == "BKN") return 75;
        if (coverage == "OVC") return 100;
        return 0;
    }
};

/**
 * Complete METAR weather report
 */
struct METARReport {
    std::string stationId;          // ICAO code (e.g., "KJFK")
    time_t observationTime;         // Unix timestamp
    int windDirection;              // 0-360 degrees (from)
    int windSpeed;                  // knots
    int windGust;                   // knots (0 if no gust)
    bool windVariable;              // Variable wind direction
    int variableMinDir, variableMaxDir;  // If variable
    
    int visibility;                 // statute miles
    std::vector<std::string> weatherPhenomena;  // RA, SN, TS, CAVOK, VV###, etc.
    
    std::vector<CloudLayer> clouds;
    
    int temperature;                // Celsius
    int dewpoint;                   // Celsius
    double altimeterSetting;        // inHg (e.g., 30.12)
    
    // Derived weather conditions
    bool thunderstorm;              // Present if "TS" in phenomena
    bool precipitation;             // Present if RA, SN, etc.
    bool icing;                     // Inferred from conditions
    bool lowVisibility;             // < 3 SM
    double ceilingFeet;             // Lowest OVC/BKN altitude
    
    // Remarks (RMK section)
    std::vector<std::string> remarks;
    
    // Validity
    bool isValid;
    std::string parseError;         // If not valid, reason why
};

/**
 * Weather Database
 * Manages METAR reports and weather conditions
 */
class WeatherDatabase {
public:
    static constexpr int METAR_CACHE_SIZE = 100;
    static constexpr int METAR_EXPIRATION_MINUTES = 60;  // METAR reports valid for 1 hour
    
    WeatherDatabase();
    ~WeatherDatabase();
    
    /**
     * Initialize weather database
     * @param cacheFile Optional path to cached METAR reports
     * @return true if initialized
     */
    bool initialize(const std::string& cacheFile = "");
    
    /**
     * Shutdown weather database
     */
    void shutdown();
    
    /**
     * Parse METAR report string
     * @param metarString Raw METAR string (e.g., "KJFK 121851Z 31008KT 10SM...")
     * @return Parsed METARReport structure
     */
    METARReport parseMETAR(const std::string& metarString);
    
    /**
     * Update weather for airport
     * @param icao Airport ICAO code
     * @param metar METAR report string
     * @return true if successfully updated
     */
    bool updateWeather(const std::string& icao, const std::string& metarString);
    
    /**
     * Get current weather for airport
     * @param icao Airport ICAO code
     * @param report Output METAR report
     * @return true if weather available
     */
    bool getWeather(const std::string& icao, METARReport& report);
    
    /**
     * Get weather conditions from report
     * @param report METAR report
     * @return WeatherConditions structure
     */
    WeatherConditions getConditions(const METARReport& report);
    
    /**
     * Check if weather is suitable for flight
     * @param report METAR report
     * @return true if conditions meet minimum VFR
     */
    bool isSuitableForFlight(const METARReport& report);
    
    /**
     * Check if weather is suitable for takeoff
     * @param report METAR report
     * @return true if conditions meet takeoff minimums
     */
    bool isSuitableForTakeoff(const METARReport& report);
    
    /**
     * Check if weather is suitable for landing
     * @param report METAR report
     * @return true if conditions meet landing minimums
     */
    bool isSuitableForLanding(const METARReport& report);
    
    /**
     * Load METAR reports from file
     * @param filepath Path to file with METAR reports (one per line)
     * @return Number of reports loaded
     */
    int loadMETARsFromFile(const std::string& filepath);
    
    /**
     * Save cached METAR reports to file
     * @param filepath Path to output file
     * @return true if successful
     */
    bool saveMETARsToFile(const std::string& filepath);
    
    /**
     * Clear all cached METAR reports
     */
    void clearCache();
    
    /**
     * Get number of cached reports
     */
    int getCacheSize() const;

private:
    struct CachedReport {
        METARReport report;
        time_t cacheTime;
    };
    
    std::map<std::string, CachedReport> reportCache_;
    mutable std::mutex cacheMutex_;
    
    // METAR parsing helper methods
    bool parseStationId(const std::string& token, METARReport& report);
    bool parseDateTime(const std::string& token, METARReport& report);
    bool parseWind(const std::string& token, METARReport& report);
    bool parseVisibility(const std::string& token, METARReport& report);
    bool parseWeatherPhenomena(const std::string& token, METARReport& report);
    bool parseCloudLayer(const std::string& token, METARReport& report);
    bool parseTemperatureDewpoint(const std::string& token, METARReport& report);
    bool parseAltimeter(const std::string& token, METARReport& report);
    bool parseRemarks(const std::string& token, METARReport& report);
    
    // Helper methods
    bool isExpired(const CachedReport& cached) const;
    void deriveConditions(METARReport& report);
    double calculateCeiling(const METARReport& report) const;
    static int parseInteger(const std::string& str, int& value);
};

} // namespace AICopilot

#endif // WEATHER_DATABASE_HPP

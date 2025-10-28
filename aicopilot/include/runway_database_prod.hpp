/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Runway Database - Production-ready airport runway management system
* Provides runway data, ILS information, and runway selection for 50+ airports
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#pragma once

#include "runway_data.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <mutex>

namespace AICopilot {

/**
 * Production-Ready Runway Database
 * Manages runway information for 50+ major airports
 * Features: Thread-safe, <10ms queries, <5MB memory footprint
 */
class RunwayDatabase {
public:
    RunwayDatabase();
    ~RunwayDatabase();
    
    /**
     * Initialize database with built-in runway data
     * @return true if initialized successfully
     */
    bool Initialize();
    
    /**
     * Shutdown database
     */
    void Shutdown();
    
    /**
     * Get runway information by ICAO and runway ID
     * @param icao Airport ICAO code (e.g., "KJFK")
     * @param runwayId Runway identifier (e.g., "04L")
     * @param runway Output runway information
     * @return true if runway found
     */
    bool GetRunwayInfo(const std::string& icao, const std::string& runwayId, 
                       RunwayInfo& runway) const;
    
    /**
     * Get all runways for airport
     * @param icao Airport ICAO code
     * @return Vector of RunwayInfo structures
     */
    std::vector<RunwayInfo> GetAllRunways(const std::string& icao) const;
    
    /**
     * Get best runway for landing based on wind
     * Minimizes crosswind, prefers headwind and ILS
     * @param icao Airport ICAO code
     * @param windDirection Wind direction (degrees 0-360)
     * @param windSpeed Wind speed (knots)
     * @return Selected RunwayInfo or empty if none suitable
     */
    RunwayInfo GetBestRunway(const std::string& icao, int windDirection, int windSpeed) const;
    
    /**
     * Get best runway for landing with criteria
     * @param icao Airport ICAO code
     * @param windDirection Wind direction (degrees)
     * @param windSpeed Wind speed (knots)
     * @param maxCrosswind Maximum acceptable crosswind (knots)
     * @param preferILS Prefer ILS-equipped runways
     * @return Selected RunwayInfo or empty if none suitable
     */
    RunwayInfo GetBestRunwayForLanding(const std::string& icao, int windDirection, 
                                       int windSpeed, double maxCrosswind, 
                                       bool preferILS = true) const;
    
    /**
     * Get best runway for takeoff
     * Maximizes headwind, minimizes crosswind
     * @param icao Airport ICAO code
     * @param windDirection Wind direction (degrees)
     * @param windSpeed Wind speed (knots)
     * @return Selected RunwayInfo or empty if none suitable
     */
    RunwayInfo GetBestRunwayForTakeoff(const std::string& icao, int windDirection, 
                                       int windSpeed) const;
    
    /**
     * Validate runway for aircraft operation
     * Checks length, width, and surface compatibility
     * @param icao Airport ICAO code
     * @param runwayId Runway identifier
     * @param aircraftType Aircraft type string
     * @param requiredDistance Required runway distance (feet)
     * @return true if runway is valid for aircraft
     */
    bool ValidateRunway(const std::string& icao, const std::string& runwayId,
                        const std::string& aircraftType, double requiredDistance) const;
    
    /**
     * Calculate landing distance required (FAA 67% rule)
     * @param runway Runway information
     * @param aircraftConfig Aircraft performance data
     * @return Required landing distance (feet)
     */
    double GetLandingDistance(const RunwayInfo& runway, 
                             const AircraftPerformance& aircraftConfig) const;
    
    /**
     * Calculate takeoff distance required (FAA 15% rule)
     * @param runway Runway information
     * @param aircraftConfig Aircraft performance data
     * @return Required takeoff distance (feet)
     */
    double GetTakeoffDistance(const RunwayInfo& runway,
                             const AircraftPerformance& aircraftConfig) const;
    
    /**
     * Get airport information
     * @param icao Airport ICAO code
     * @param airport Output airport information
     * @return true if airport found
     */
    bool GetAirportInfo(const std::string& icao, AirportInfo& airport) const;
    
    /**
     * Check if airport has ILS
     * @param icao Airport ICAO code
     * @return true if any runway has ILS
     */
    bool HasILS(const std::string& icao) const;
    
    /**
     * Check if runway has ILS
     * @param icao Airport ICAO code
     * @param runwayId Runway identifier
     * @return true if runway has ILS
     */
    bool HasRunwayILS(const std::string& icao, const std::string& runwayId) const;
    
    /**
     * Get ILS data for runway
     * @param icao Airport ICAO code
     * @param runwayId Runway identifier
     * @param ilsData Output ILS data
     * @return true if ILS available
     */
    bool GetILSData(const std::string& icao, const std::string& runwayId, 
                    ILSData& ilsData) const;
    
    /**
     * Get runway count
     * @return Total number of runways in database
     */
    int GetRunwayCount() const;
    
    /**
     * Get airport count
     * @return Total number of airports in database
     */
    int GetAirportCount() const;
    
    /**
     * Get all airport codes
     * @return Vector of ICAO codes
     */
    std::vector<std::string> GetAirportCodes() const;
    
    /**
     * Check if airport exists in database
     * @param icao Airport ICAO code
     * @return true if airport found
     */
    bool AirportExists(const std::string& icao) const;
    
    /**
     * Clear all data
     */
    void Clear();
    
    /**
     * Add runway to database
     * @param runway Runway information
     */
    void AddRunway(const RunwayInfo& runway);
    
    /**
     * Get statistics
     * @return String with database statistics
     */
    std::string GetStatistics() const;

private:
    struct AirportKey {
        std::string icao;
        bool operator<(const AirportKey& other) const { return icao < other.icao; }
    };
    
    struct RunwayKey {
        std::string icao;
        std::string runwayId;
        bool operator<(const RunwayKey& other) const {
            if (icao != other.icao) return icao < other.icao;
            return runwayId < other.runwayId;
        }
    };
    
    std::map<RunwayKey, RunwayInfo> runways_;
    std::map<AirportKey, AirportInfo> airports_;
    mutable std::mutex dbMutex_;
    
    // Initialize with production runway data
    void InitializeRunwayData();
    
    // Helper methods
    void AddAirport(const AirportInfo& airport);
    void UpdateRunwayStats(const std::string& icao);
    
    // Surface friction coefficients
    double GetSurfaceFrictionCoefficient(SurfaceType surface) const;
    
    // FAA landing/takeoff distance calculations
    double CalculateLandingDistanceWithFactors(const RunwayInfo& runway,
                                              const AircraftPerformance& aircraft) const;
    double CalculateTakeoffDistanceWithFactors(const RunwayInfo& runway,
                                              const AircraftPerformance& aircraft) const;
};

} // namespace AICopilot

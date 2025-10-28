/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Runway Database - Airport runway and ILS information
* Provides runway data, ILS frequencies, and approach information
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef RUNWAY_DATABASE_HPP
#define RUNWAY_DATABASE_HPP

#include "aicopilot_types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>

namespace AICopilot {

/**
 * ILS System Information
 */
struct ILSData {
    bool hasILS;                    // Does runway have ILS
    double localizerFrequency;      // 108.1-111.95 MHz
    double glideslopeFrequency;     // 329.15-335.0 MHz
    double localizerCourse;         // Magnetic heading
    double glideslopeAngle;         // Degrees (typically 3.0)
    Position thresholdPosition;     // Runway threshold coordinates
    int decisionHeight;             // Feet AGL (CAT I: 200, CAT II: 100, CAT III: 0)
    std::string category;           // CAT I, CAT II, CAT IIIA, CAT IIIB, CAT IIIC
    int minimumRVR;                 // Runway Visual Range - feet
};

/**
 * Runway Information
 */
struct RunwayInfo {
    std::string icaoCode;           // Airport ICAO (e.g., "KJFK")
    std::string runwayId;           // Runway identifier (e.g., "04L", "22R", "18")
    Position thresholdPosition;     // Runway threshold coordinates
    int headingMagnetic;            // Magnetic heading (40, 90, 180, etc.)
    int headingTrue;                // True heading (calculated from mag var)
    int length;                     // Feet
    int width;                      // Feet
    std::string surfaceType;        // ASPH, CONC, GRS, DIRT, TURF, WATER, etc.
    
    ILSData ilsData;
    bool hasVGSI;                   // PAPI or VASI
    std::string vgsiType;           // PAPI, VASI, or empty
    
    int designCode;                 // ICAO Design Code (1-4 digits)
    bool displaceThreshold;         // Runway has displaced threshold
    int displacedThresholdDistance; // Feet from actual threshold
    
    // Distance available fields (feet)
    int TODA;                       // Takeoff Distance Available
    int TORA;                       // Takeoff Run Available
    int LDA;                        // Landing Distance Available
    int ASDA;                       // Accelerate-Stop Distance Available
    
    // Surface conditions
    bool friction;                  // Low friction hazard
    bool grooving;                  // Grooved surface
    bool porous;                    // Porous friction course
    
    // Lighting
    bool hasALS;                    // Approach Lighting System
    bool hasRWYLights;              // Runway edge lights
    bool hasREIL;                   // Runway End Identifier Lights
};

/**
 * Runway Database
 * Manages runway and ILS information for airports
 */
class RunwayDatabase {
public:
    RunwayDatabase();
    ~RunwayDatabase();
    
    /**
     * Initialize runway database
     * @param csvFile Path to runway database CSV file
     * @return true if initialized successfully
     */
    bool initialize(const std::string& csvFile);
    
    /**
     * Shutdown database
     */
    void shutdown();
    
    /**
     * Get runway information by ICAO and runway ID
     * @param icao Airport ICAO code
     * @param runwayId Runway identifier (e.g., "04L")
     * @param runway Output runway information
     * @return true if found
     */
    bool getRunway(const std::string& icao, const std::string& runwayId, RunwayInfo& runway);
    
    /**
     * Get all runways for airport
     * @param icao Airport ICAO code
     * @return Vector of runways
     */
    std::vector<RunwayInfo> getAirportRunways(const std::string& icao);
    
    /**
     * Get best runway for landing based on wind
     * @param icao Airport ICAO code
     * @param windDirection Wind direction (degrees)
     * @param windSpeed Wind speed (knots)
     * @param maxCrosswind Maximum acceptable crosswind (knots)
     * @param runway Output selected runway
     * @return true if suitable runway found
     */
    bool selectRunwayForLanding(const std::string& icao, 
                               int windDirection, int windSpeed,
                               int maxCrosswind,
                               RunwayInfo& runway);
    
    /**
     * Get best runway for takeoff based on wind
     * @param icao Airport ICAO code
     * @param windDirection Wind direction (degrees)
     * @param windSpeed Wind speed (knots)
     * @param runway Output selected runway
     * @return true if suitable runway found
     */
    bool selectRunwayForTakeoff(const std::string& icao, 
                               int windDirection, int windSpeed,
                               RunwayInfo& runway);
    
    /**
     * Get ILS data for runway
     * @param icao Airport ICAO code
     * @param runwayId Runway identifier
     * @param ils Output ILS data
     * @return true if ILS available
     */
    bool getILSData(const std::string& icao, const std::string& runwayId, ILSData& ils);
    
    /**
     * Check if runway is suitable for landing
     * @param runway Runway information
     * @param aircraftLength Aircraft length in feet
     * @param requiredDistance Required landing distance
     * @return true if runway is suitable
     */
    bool isSuitableForLanding(const RunwayInfo& runway, 
                             int aircraftLength, 
                             int requiredDistance);
    
    /**
     * Check if runway is suitable for takeoff
     * @param runway Runway information
     * @param aircraftLength Aircraft length in feet
     * @param requiredDistance Required takeoff distance
     * @return true if runway is suitable
     */
    bool isSuitableForTakeoff(const RunwayInfo& runway,
                            int aircraftLength,
                            int requiredDistance);
    
    /**
     * Load runways from CSV file
     * @param filePath Path to runway CSV file
     * @return Number of runways loaded
     * 
     * CSV Format:
     * ICAO,RwyID,Lat,Lon,Hdg,Length,Width,Surface,ILS,Freq1,Freq2,CourseGS,DH,Cat,RVR,TODA,TORA,LDA,ASDA
     * Example: KJFK,04L,40.636,N73.778,040,10995,150,ASPH,1,110.9,332.6,040,200,I,1800,11351,11351,10000,11351
     */
    int loadFromCSV(const std::string& filePath);
    
    /**
     * Add runway to database
     * @param runway Runway information
     */
    void addRunway(const RunwayInfo& runway);
    
    /**
     * Get runway count
     */
    int getRunwayCount() const;
    
    /**
     * Clear all runways
     */
    void clear();

private:
    struct RunwayKey {
        std::string icao;
        std::string runwayId;
        
        bool operator<(const RunwayKey& other) const {
            if (icao != other.icao) return icao < other.icao;
            return runwayId < other.runwayId;
        }
    };
    
    std::map<RunwayKey, RunwayInfo> runwayData_;
    mutable std::mutex dbMutex_;
    
    // Helper methods
    double calculateCrosswind(int runwayHeading, int windDirection, int windSpeed) const;
    double calculateHeadwind(int runwayHeading, int windDirection, int windSpeed) const;
    std::string getReciprocal(const std::string& runwayId) const;
    int parseRunwayHeading(const std::string& runwayId) const;
};

} // namespace AICopilot

#endif // RUNWAY_DATABASE_HPP

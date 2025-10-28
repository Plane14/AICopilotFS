/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Navigation Database - Waypoints, airways, and procedures
* Provides navigation data including navaids, airways, and approach procedures
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef NAVDATA_LOADER_HPP
#define NAVDATA_LOADER_HPP

#include "aicopilot_types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>

namespace AICopilot {

/**
 * Navaid types
 */
enum class NavaidType {
    VOR,            // VHF Omnidirectional Range
    NDB,            // Non-Directional Beacon
    DME,            // Distance Measuring Equipment
    TACAN,          // Tactical Air Navigation
    INTERSECTION,   // Intersection point
    USER,           // User-defined waypoint
    AIRPORT,        // Airport reference point
    UNKNOWN
};

/**
 * Navaid (waypoint) information
 */
struct NavaidInfo {
    std::string identifier;         // 1-5 character ident
    NavaidType type;
    Position position;
    double magneticVariation;       // Degrees (East positive)
    double frequency;               // MHz (VOR) or kHz (NDB)
    int range;                      // Nautical miles
    std::string region;             // ICAO region
    std::string name;               // Full name
};

/**
 * Airway segment
 */
struct AirwaySegment {
    std::string identifier;         // Route ID (e.g., "J500", "A590")
    std::string type;               // "LOW" or "HIGH"
    NavaidInfo startPoint;
    NavaidInfo endPoint;
    int minimumAltitude;            // Feet MSL
    int maximumAltitude;            // Feet MSL (0 if no limit)
    std::vector<Position> routePoints;  // Waypoints along segment
};

/**
 * Approach/Departure Procedure
 */
struct ProcedureWaypoint {
    std::string identifier;
    Position position;
    double altitude;                // Feet MSL
    double speed;                   // Knots (0 if no constraint)
    std::string type;               // IAF, IF, FAF, MAP, etc.
    bool altitudeConstraint;
    bool speedConstraint;
};

struct Procedure {
    std::string identifier;         // Procedure name
    std::string type;               // "SID" (departure), "STAR" (arrival), "APP" (approach)
    std::string runway;             // Associated runway
    std::string airportICAO;        // Airport ICAO code
    std::vector<ProcedureWaypoint> waypoints;
    std::vector<std::string> transitions;  // Available transitions
};

/**
 * Navigation Database Loader
 * Loads and manages navigation data from files
 */
class NavdataLoader {
public:
    NavdataLoader();
    ~NavdataLoader();
    
    /**
     * Initialize navigation database
     * @param navdataPath Path to navigation data directory
     * @return true if initialized
     */
    bool initialize(const std::string& navdataPath);
    
    /**
     * Shutdown database
     */
    void shutdown();
    
    /**
     * Load navaids from file
     * @param filePath Path to navaids.dat file
     * @return Number of navaids loaded
     * 
     * File format (comma-separated):
     * TYPE,IDENT,REGION,AIRPORT,LAT,LON,MAG_VAR,FREQ,RANGE
     * VOR,JFK,NY,KJFK,40.628,-73.772,12.8,110.9,150
     */
    int loadNavaids(const std::string& filePath);
    
    /**
     * Load airways from file
     * @param filePath Path to airways.dat file
     * @return Number of airways loaded
     */
    int loadAirways(const std::string& filePath);
    
    /**
     * Load procedures from file
     * @param filePath Path to procedures.dat file
     * @return Number of procedures loaded
     */
    int loadProcedures(const std::string& filePath);
    
    /**
     * Get navaid by identifier
     * @param identifier Navaid identifier (e.g., "JFK", "OZZZI")
     * @param navaid Output navaid information
     * @return true if found
     */
    bool getNavaid(const std::string& identifier, NavaidInfo& navaid);
    
    /**
     * Get navaids near position
     * @param position Center position
     * @param radiusNM Search radius in nautical miles
     * @param type Filter by type (empty for all)
     * @return Vector of nearby navaids
     */
    std::vector<NavaidInfo> getNavaidsNearby(const Position& position, 
                                            double radiusNM,
                                            const std::string& type = "");
    
    /**
     * Get airways starting from navaid
     * @param startIdent Starting navaid identifier
     * @return Vector of airway segments
     */
    std::vector<AirwaySegment> getAirwaysFrom(const std::string& startIdent);
    
    /**
     * Get SID procedures for airport and runway
     * @param icao Airport ICAO code
     * @param runway Runway identifier
     * @return Vector of SID procedures
     */
    std::vector<Procedure> getSIDProcedures(const std::string& icao, 
                                           const std::string& runway = "");
    
    /**
     * Get STAR procedures for airport and runway
     * @param icao Airport ICAO code
     * @param runway Runway identifier
     * @return Vector of STAR procedures
     */
    std::vector<Procedure> getSTARProcedures(const std::string& icao,
                                            const std::string& runway = "");
    
    /**
     * Get approach procedures for airport and runway
     * @param icao Airport ICAO code
     * @param runway Runway identifier
     * @return Vector of approach procedures
     */
    std::vector<Procedure> getApproachProcedures(const std::string& icao,
                                                const std::string& runway = "");
    
    /**
     * Add navaid to database
     * @param navaid Navaid information
     */
    void addNavaid(const NavaidInfo& navaid);
    
    /**
     * Add airway to database
     * @param airway Airway segment
     */
    void addAirway(const AirwaySegment& airway);
    
    /**
     * Add procedure to database
     * @param proc Procedure information
     */
    void addProcedure(const Procedure& proc);
    
    /**
     * Get statistics
     * @return Tuple of (navaids_count, airways_count, procedures_count)
     */
    std::tuple<int, int, int> getStatistics() const;
    
    /**
     * Clear all data
     */
    void clear();

private:
    std::map<std::string, NavaidInfo> navaids_;
    std::vector<AirwaySegment> airways_;
    std::vector<Procedure> procedures_;
    mutable std::mutex dbMutex_;
    
    // Helper methods
    NavaidType parseNavaidType(const std::string& typeStr);
    double calculateDistance(const Position& p1, const Position& p2) const;
    bool isWithinRadius(const Position& p1, const Position& p2, double radiusNM) const;
};

} // namespace AICopilot

#endif // NAVDATA_LOADER_HPP

/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef NAVDATA_PROVIDER_H
#define NAVDATA_PROVIDER_H

#include "aicopilot_types.h"
#include <string>
#include <vector>
#include <memory>

namespace AICopilot {

/**
 * Airport information structure
 */
struct AirportInfo {
    std::string icao;           // ICAO code (e.g., "KJFK")
    std::string iata;           // IATA code (e.g., "JFK")
    std::string name;           // Full airport name
    Position position;          // Airport reference point
    double elevation;           // Field elevation in feet MSL
    int longestRunway;          // Longest runway length in feet
    bool towered;               // Has control tower
    std::vector<std::string> runways; // List of runway identifiers
};

/**
 * Navaid information structure
 */
struct NavaidInfo {
    std::string id;             // Navaid identifier
    std::string name;           // Navaid name
    std::string type;           // VOR, NDB, DME, TACAN, etc.
    Position position;          // Navaid position
    double frequency;           // Frequency in kHz (NDB) or MHz (VOR)
    double range;               // Range in nautical miles
    double magneticVariation;   // Local magnetic variation
};

/**
 * Abstract interface for navigation data providers
 * Can be implemented by SimConnect, database, or external API providers
 */
class INavdataProvider {
public:
    virtual ~INavdataProvider() = default;
    
    /**
     * Initialize the navdata provider
     * @return true if initialization successful
     */
    virtual bool initialize() = 0;
    
    /**
     * Shutdown the navdata provider
     */
    virtual void shutdown() = 0;
    
    /**
     * Check if provider is ready to serve data
     */
    virtual bool isReady() const = 0;
    
    /**
     * Get airport information by ICAO code
     * @param icao ICAO airport code (e.g., "KJFK")
     * @param info Output airport information
     * @return true if airport found
     */
    virtual bool getAirportByICAO(const std::string& icao, AirportInfo& info) = 0;
    
    /**
     * Get airports within a radius
     * @param center Center position
     * @param radiusNM Search radius in nautical miles
     * @return Vector of airports within radius
     */
    virtual std::vector<AirportInfo> getAirportsNearby(const Position& center, double radiusNM) = 0;
    
    /**
     * Get navaid information by identifier
     * @param id Navaid identifier
     * @param info Output navaid information
     * @return true if navaid found
     */
    virtual bool getNavaidByID(const std::string& id, NavaidInfo& info) = 0;
    
    /**
     * Get navaids within a radius
     * @param center Center position
     * @param radiusNM Search radius in nautical miles
     * @param type Filter by type (empty for all types)
     * @return Vector of navaids within radius
     */
    virtual std::vector<NavaidInfo> getNavaidsNearby(const Position& center, 
                                                      double radiusNM, 
                                                      const std::string& type = "") = 0;
    
    /**
     * Find nearest airport to a position
     * @param position Search position
     * @param info Output airport information
     * @return true if airport found
     */
    virtual bool getNearestAirport(const Position& position, AirportInfo& info) = 0;
};

/**
 * SimConnect-based navdata provider
 * Uses SimConnect facility requests to obtain navigation data
 */
class SimConnectNavdataProvider : public INavdataProvider {
public:
    SimConnectNavdataProvider();
    ~SimConnectNavdataProvider() override;
    
    bool initialize() override;
    void shutdown() override;
    bool isReady() const override;
    
    bool getAirportByICAO(const std::string& icao, AirportInfo& info) override;
    std::vector<AirportInfo> getAirportsNearby(const Position& center, double radiusNM) override;
    bool getNavaidByID(const std::string& id, NavaidInfo& info) override;
    std::vector<NavaidInfo> getNavaidsNearby(const Position& center, 
                                             double radiusNM, 
                                             const std::string& type = "") override;
    bool getNearestAirport(const Position& position, AirportInfo& info) override;
    
    /**
     * Set SimConnect handle to use for requests
     * Must be called before initialize() if using external SimConnect connection
     */
    void setSimConnectHandle(void* hSimConnect);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * Simple cache-based navdata provider
 * Uses a simple in-memory cache of airports and navaids
 * Useful for testing or when no database/SimConnect is available
 */
class CachedNavdataProvider : public INavdataProvider {
public:
    CachedNavdataProvider();
    ~CachedNavdataProvider() override;
    
    bool initialize() override;
    void shutdown() override;
    bool isReady() const override;
    
    bool getAirportByICAO(const std::string& icao, AirportInfo& info) override;
    std::vector<AirportInfo> getAirportsNearby(const Position& center, double radiusNM) override;
    bool getNavaidByID(const std::string& id, NavaidInfo& info) override;
    std::vector<NavaidInfo> getNavaidsNearby(const Position& center, 
                                             double radiusNM, 
                                             const std::string& type = "") override;
    bool getNearestAirport(const Position& position, AirportInfo& info) override;
    
    /**
     * Add airport to cache
     */
    void addAirport(const AirportInfo& info);
    
    /**
     * Add navaid to cache
     */
    void addNavaid(const NavaidInfo& info);
    
    /**
     * Load common airports from a file
     * @param filePath Path to airport data file
     * @return true if loaded successfully
     */
    bool loadAirportsFromFile(const std::string& filePath);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace AICopilot

#endif // NAVDATA_PROVIDER_H

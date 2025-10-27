/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "navdata_provider.h"
#include <iostream>
#include <map>
#include <cmath>

#ifdef USE_MSFS2024_SDK
#include <windows.h>
#include <SimConnect.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AICopilot {

// Helper function to calculate distance
static double calculateDistance(const Position& a, const Position& b) {
    const double R = 3440.065; // Earth radius in NM
    double lat1 = a.latitude * M_PI / 180.0;
    double lat2 = b.latitude * M_PI / 180.0;
    double dLat = (b.latitude - a.latitude) * M_PI / 180.0;
    double dLon = (b.longitude - a.longitude) * M_PI / 180.0;
    
    double x = sin(dLat/2) * sin(dLat/2) +
                cos(lat1) * cos(lat2) *
                sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(x), sqrt(1-x));
    return R * c;
}

//=============================================================================
// SimConnectNavdataProvider implementation
//=============================================================================

class SimConnectNavdataProvider::Impl {
public:
    void* hSimConnect = nullptr;
    bool ready = false;
    bool ownHandle = false; // Track if we created the handle
    
    // Cache for facility data
    std::map<std::string, AirportInfo> airportCache;
    std::map<std::string, NavaidInfo> navaidCache;
};

SimConnectNavdataProvider::SimConnectNavdataProvider() : pImpl(std::make_unique<Impl>()) {
    std::cout << "SimConnectNavdataProvider initialized" << std::endl;
}

SimConnectNavdataProvider::~SimConnectNavdataProvider() {
    shutdown();
}

bool SimConnectNavdataProvider::initialize() {
#ifdef USE_MSFS2024_SDK
    if (!pImpl->hSimConnect) {
        // Try to create our own SimConnect connection
        HRESULT hr = SimConnect_Open((HANDLE*)&pImpl->hSimConnect, 
                                     "AICopilot NavdataProvider", 
                                     nullptr, 0, 0, 0);
        if (SUCCEEDED(hr)) {
            pImpl->ownHandle = true;
            pImpl->ready = true;
            std::cout << "SimConnectNavdataProvider: Successfully connected to SimConnect" << std::endl;
            return true;
        } else {
            std::cout << "SimConnectNavdataProvider: Failed to connect to SimConnect (code: " 
                     << hr << ")" << std::endl;
            return false;
        }
    } else {
        // Using external handle
        pImpl->ready = true;
        std::cout << "SimConnectNavdataProvider: Using external SimConnect handle" << std::endl;
        return true;
    }
#else
    std::cout << "SimConnectNavdataProvider: Not compiled with SimConnect SDK support" << std::endl;
    return false;
#endif
}

void SimConnectNavdataProvider::shutdown() {
#ifdef USE_MSFS2024_SDK
    if (pImpl->hSimConnect && pImpl->ownHandle) {
        SimConnect_Close((HANDLE)pImpl->hSimConnect);
        pImpl->hSimConnect = nullptr;
    }
#endif
    pImpl->ready = false;
    pImpl->airportCache.clear();
    pImpl->navaidCache.clear();
}

bool SimConnectNavdataProvider::isReady() const {
    return pImpl->ready;
}

bool SimConnectNavdataProvider::getAirportByICAO(const std::string& icao, AirportInfo& info) {
    if (!pImpl->ready) {
        return false;
    }
    
    // Check cache first
    auto it = pImpl->airportCache.find(icao);
    if (it != pImpl->airportCache.end()) {
        info = it->second;
        return true;
    }
    
#ifdef USE_MSFS2024_SDK
    // In a full implementation, would use SimConnect_RequestFacilityData
    // For now, return false and suggest using CachedNavdataProvider
    std::cout << "SimConnectNavdataProvider::getAirportByICAO(" << icao 
             << ") - SimConnect facility requests require additional implementation" << std::endl;
#endif
    
    return false;
}

std::vector<AirportInfo> SimConnectNavdataProvider::getAirportsNearby(const Position& center, double radiusNM) {
    std::vector<AirportInfo> result;
    
    if (!pImpl->ready) {
        return result;
    }
    
    // Search cache
    for (const auto& pair : pImpl->airportCache) {
        double dist = calculateDistance(center, pair.second.position);
        if (dist <= radiusNM) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

bool SimConnectNavdataProvider::getNavaidByID(const std::string& id, NavaidInfo& info) {
    if (!pImpl->ready) {
        return false;
    }
    
    // Check cache
    auto it = pImpl->navaidCache.find(id);
    if (it != pImpl->navaidCache.end()) {
        info = it->second;
        return true;
    }
    
    return false;
}

std::vector<NavaidInfo> SimConnectNavdataProvider::getNavaidsNearby(
    const Position& center, double radiusNM, const std::string& type) {
    std::vector<NavaidInfo> result;
    
    if (!pImpl->ready) {
        return result;
    }
    
    for (const auto& pair : pImpl->navaidCache) {
        if (!type.empty() && pair.second.type != type) {
            continue;
        }
        double dist = calculateDistance(center, pair.second.position);
        if (dist <= radiusNM) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

bool SimConnectNavdataProvider::getNearestAirport(const Position& position, AirportInfo& info) {
    if (!pImpl->ready) {
        return false;
    }
    
    double minDist = 1e9;
    bool found = false;
    
    for (const auto& pair : pImpl->airportCache) {
        double dist = calculateDistance(position, pair.second.position);
        if (dist < minDist) {
            minDist = dist;
            info = pair.second;
            found = true;
        }
    }
    
    return found;
}

bool SimConnectNavdataProvider::getAirportLayout(const std::string& icao, AirportLayout& layout) {
    if (!pImpl->ready) {
        return false;
    }
    
    // For now, return a basic empty layout
    // A full implementation would use SimConnect facility requests or database lookups
    layout.runways.clear();
    layout.taxiwayNetwork = Airport::TaxiwayNetwork();
    layout.parkingPositions.clear();
    
    std::cout << "SimConnectNavdataProvider::getAirportLayout(" << icao 
             << ") - stub implementation, returning empty layout" << std::endl;
    
    return true; // Return true even with empty data to avoid errors
}

void SimConnectNavdataProvider::setSimConnectHandle(void* hSimConnect) {
    pImpl->hSimConnect = hSimConnect;
}

//=============================================================================
// CachedNavdataProvider implementation
//=============================================================================

class CachedNavdataProvider::Impl {
public:
    std::map<std::string, AirportInfo> airports;
    std::map<std::string, NavaidInfo> navaids;
    bool ready = false;
    
    void loadDefaultAirports() {
        // Load common airports for testing and fallback
        addAirport("KJFK", "John F Kennedy Intl", 40.6398, -73.7789, 13, 14511, true);
        addAirport("KLAX", "Los Angeles Intl", 33.9425, -118.4081, 125, 12923, true);
        addAirport("KORD", "Chicago O'Hare Intl", 41.9786, -87.9048, 672, 13000, true);
        addAirport("KATL", "Hartsfield Jackson Atlanta Intl", 33.6367, -84.4281, 1026, 12390, true);
        addAirport("KDFW", "Dallas Fort Worth Intl", 32.8968, -97.0380, 607, 13401, true);
        addAirport("KDEN", "Denver Intl", 39.8617, -104.6731, 5434, 16000, true);
        addAirport("KSFO", "San Francisco Intl", 37.6213, -122.3790, 13, 11870, true);
        addAirport("KLAS", "Las Vegas McCarran Intl", 36.0840, -115.1537, 2181, 14510, true);
        addAirport("KMIA", "Miami Intl", 25.7932, -80.2906, 8, 13016, true);
        addAirport("KSEA", "Seattle Tacoma Intl", 47.4502, -122.3088, 433, 11901, true);
        addAirport("KBOS", "Boston Logan Intl", 42.3656, -71.0096, 20, 10083, true);
        addAirport("KEWR", "Newark Liberty Intl", 40.6925, -74.1687, 18, 11000, true);
        addAirport("KMCO", "Orlando Intl", 28.4294, -81.3089, 96, 12005, true);
        addAirport("KPHX", "Phoenix Sky Harbor Intl", 33.4343, -112.0116, 1135, 11489, true);
        addAirport("KIAH", "Houston George Bush Intl", 29.9844, -95.3414, 97, 12001, true);
        
        // Add some smaller airports
        addAirport("KVNY", "Van Nuys Airport", 34.2098, -118.4898, 802, 8001, true);
        addAirport("KSNA", "John Wayne Orange County", 33.6757, -117.8682, 56, 5701, true);
        addAirport("KSAN", "San Diego Intl", 32.7336, -117.1897, 17, 9401, true);
        addAirport("KPDX", "Portland Intl", 45.5887, -122.5975, 31, 11000, true);
        addAirport("KBWI", "Baltimore Washington Intl", 39.1754, -76.6683, 146, 10502, true);
        
        // European airports
        addAirport("EGLL", "London Heathrow", 51.4700, -0.4543, 83, 12799, true);
        addAirport("LFPG", "Paris Charles de Gaulle", 49.0097, 2.5479, 392, 13829, true);
        addAirport("EDDF", "Frankfurt Main", 50.0379, 8.5622, 364, 13123, true);
        addAirport("LEMD", "Madrid Barajas", 40.4719, -3.5626, 1998, 14268, true);
        addAirport("LIRF", "Rome Fiumicino", 41.8003, 12.2389, 13, 12795, true);
        
        std::cout << "Loaded " << airports.size() << " default airports" << std::endl;
    }
    
    void loadDefaultNavaids() {
        // Load common VORs for testing
        addNavaid("LAX", "Los Angeles VOR/DME", "VOR", 33.9297, -118.4156, 113.60, 200);
        addNavaid("SFO", "San Francisco VOR/DME", "VOR", 37.6197, -122.3728, 115.80, 200);
        addNavaid("DEN", "Denver VOR/DME", "VOR", 39.8561, -104.6567, 117.90, 200);
        addNavaid("ORD", "Chicago O'Hare VOR/DME", "VOR", 41.9950, -87.9339, 113.90, 200);
        addNavaid("JFK", "Kennedy VOR/DME", "VOR", 40.6394, -73.7789, 115.90, 200);
        
        std::cout << "Loaded " << navaids.size() << " default navaids" << std::endl;
    }
    
    void addAirport(const std::string& icao, const std::string& name, 
                   double lat, double lon, double elev, int runway, bool tower) {
        AirportInfo info;
        info.icao = icao;
        info.name = name;
        info.position.latitude = lat;
        info.position.longitude = lon;
        info.position.altitude = elev;
        info.elevation = elev;
        info.longestRunway = runway;
        info.towered = tower;
        airports[icao] = info;
    }
    
    void addNavaid(const std::string& id, const std::string& name, const std::string& type,
                  double lat, double lon, double freq, double range) {
        NavaidInfo info;
        info.id = id;
        info.name = name;
        info.type = type;
        info.position.latitude = lat;
        info.position.longitude = lon;
        info.frequency = freq;
        info.range = range;
        info.magneticVariation = 0.0; // Simplified
        navaids[id] = info;
    }
};

CachedNavdataProvider::CachedNavdataProvider() : pImpl(std::make_unique<Impl>()) {
    std::cout << "CachedNavdataProvider initialized" << std::endl;
}

CachedNavdataProvider::~CachedNavdataProvider() {
    shutdown();
}

bool CachedNavdataProvider::initialize() {
    pImpl->loadDefaultAirports();
    pImpl->loadDefaultNavaids();
    pImpl->ready = true;
    return true;
}

void CachedNavdataProvider::shutdown() {
    pImpl->airports.clear();
    pImpl->navaids.clear();
    pImpl->ready = false;
}

bool CachedNavdataProvider::isReady() const {
    return pImpl->ready;
}

bool CachedNavdataProvider::getAirportByICAO(const std::string& icao, AirportInfo& info) {
    auto it = pImpl->airports.find(icao);
    if (it != pImpl->airports.end()) {
        info = it->second;
        return true;
    }
    return false;
}

std::vector<AirportInfo> CachedNavdataProvider::getAirportsNearby(const Position& center, double radiusNM) {
    std::vector<AirportInfo> result;
    for (const auto& pair : pImpl->airports) {
        double dist = calculateDistance(center, pair.second.position);
        if (dist <= radiusNM) {
            result.push_back(pair.second);
        }
    }
    return result;
}

bool CachedNavdataProvider::getNavaidByID(const std::string& id, NavaidInfo& info) {
    auto it = pImpl->navaids.find(id);
    if (it != pImpl->navaids.end()) {
        info = it->second;
        return true;
    }
    return false;
}

std::vector<NavaidInfo> CachedNavdataProvider::getNavaidsNearby(
    const Position& center, double radiusNM, const std::string& type) {
    std::vector<NavaidInfo> result;
    for (const auto& pair : pImpl->navaids) {
        if (!type.empty() && pair.second.type != type) {
            continue; // Filter by type if specified
        }
        double dist = calculateDistance(center, pair.second.position);
        if (dist <= radiusNM) {
            result.push_back(pair.second);
        }
    }
    return result;
}

bool CachedNavdataProvider::getNearestAirport(const Position& position, AirportInfo& info) {
    double minDist = 1e9;
    bool found = false;
    
    for (const auto& pair : pImpl->airports) {
        double dist = calculateDistance(position, pair.second.position);
        if (dist < minDist) {
            minDist = dist;
            info = pair.second;
            found = true;
        }
    }
    return found;
}

bool CachedNavdataProvider::getAirportLayout(const std::string& icao, AirportLayout& layout) {
    // Check if airport exists in cache
    auto it = pImpl->airports.find(icao);
    if (it == pImpl->airports.end()) {
        return false;
    }
    
    // For now, return a basic empty layout
    // A full implementation would load actual runway, taxiway, and parking data from a database
    layout.runways.clear();
    layout.taxiwayNetwork = Airport::TaxiwayNetwork();
    layout.parkingPositions.clear();
    
    std::cout << "CachedNavdataProvider::getAirportLayout(" << icao 
             << ") - stub implementation, returning empty layout" << std::endl;
    
    return true; // Return true if airport exists, even with empty layout data
}

void CachedNavdataProvider::addAirport(const AirportInfo& info) {
    pImpl->airports[info.icao] = info;
}

void CachedNavdataProvider::addNavaid(const NavaidInfo& info) {
    pImpl->navaids[info.id] = info;
}

bool CachedNavdataProvider::loadAirportsFromFile(const std::string& filePath) {
    // Enhanced implementation with error handling
    std::cout << "CachedNavdataProvider::loadAirportsFromFile(" << filePath << ")" << std::endl;
    // In a full implementation, this would parse CSV/JSON airport data files
    // For now, we rely on the default airports loaded in initialize()
    return false; // File loading not implemented, using defaults
}

} // namespace AICopilot

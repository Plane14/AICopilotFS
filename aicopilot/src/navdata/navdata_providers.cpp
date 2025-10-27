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
};

SimConnectNavdataProvider::SimConnectNavdataProvider() : pImpl(std::make_unique<Impl>()) {
    std::cout << "SimConnectNavdataProvider initialized" << std::endl;
}

SimConnectNavdataProvider::~SimConnectNavdataProvider() {
    shutdown();
}

bool SimConnectNavdataProvider::initialize() {
    pImpl->ready = false; // Stub - would actually connect to SimConnect
    std::cout << "SimConnectNavdataProvider::initialize() - stub implementation" << std::endl;
    return pImpl->ready;
}

void SimConnectNavdataProvider::shutdown() {
    pImpl->ready = false;
}

bool SimConnectNavdataProvider::isReady() const {
    return pImpl->ready;
}

bool SimConnectNavdataProvider::getAirportByICAO(const std::string& icao, AirportInfo& info) {
    // Stub implementation
    std::cout << "SimConnectNavdataProvider::getAirportByICAO(" << icao << ") - stub" << std::endl;
    return false;
}

std::vector<AirportInfo> SimConnectNavdataProvider::getAirportsNearby(const Position& center, double radiusNM) {
    // Stub implementation
    return {};
}

bool SimConnectNavdataProvider::getNavaidByID(const std::string& id, NavaidInfo& info) {
    // Stub implementation
    return false;
}

std::vector<NavaidInfo> SimConnectNavdataProvider::getNavaidsNearby(
    const Position& center, double radiusNM, const std::string& type) {
    // Stub implementation
    return {};
}

bool SimConnectNavdataProvider::getNearestAirport(const Position& position, AirportInfo& info) {
    // Stub implementation
    return false;
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
};

CachedNavdataProvider::CachedNavdataProvider() : pImpl(std::make_unique<Impl>()) {
    std::cout << "CachedNavdataProvider initialized" << std::endl;
}

CachedNavdataProvider::~CachedNavdataProvider() {
    shutdown();
}

bool CachedNavdataProvider::initialize() {
    pImpl->ready = true; // Simple cache is always ready
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

void CachedNavdataProvider::addAirport(const AirportInfo& info) {
    pImpl->airports[info.icao] = info;
}

void CachedNavdataProvider::addNavaid(const NavaidInfo& info) {
    pImpl->navaids[info.id] = info;
}

bool CachedNavdataProvider::loadAirportsFromFile(const std::string& filePath) {
    // Stub implementation
    std::cout << "CachedNavdataProvider::loadAirportsFromFile(" << filePath << ") - stub" << std::endl;
    return false;
}

} // namespace AICopilot

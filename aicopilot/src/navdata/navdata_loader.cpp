/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Navigation Database Loader Implementation
*****************************************************************************/

#include "../include/navdata_loader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AICopilot {

constexpr double EARTH_RADIUS_NM = 3440.065;

NavdataLoader::NavdataLoader() = default;

NavdataLoader::~NavdataLoader() {
    shutdown();
}

bool NavdataLoader::initialize(const std::string& navdataPath) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    clear();
    std::cout << "NavdataLoader: Initialized with path: " << navdataPath << std::endl;
    return true;
}

void NavdataLoader::shutdown() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    clear();
}

int NavdataLoader::loadNavaids(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "NavdataLoader: Cannot open navaids file: " << filePath << std::endl;
        return 0;
    }
    
    int count = 0;
    std::string line;
    
    // Skip header
    if (!std::getline(file, line)) {
        file.close();
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> fields;
        
        while (std::getline(iss, token, ',')) {
            // Trim whitespace
            size_t start = token.find_first_not_of(" \t\r\n");
            size_t end = token.find_last_not_of(" \t\r\n");
            if (start != std::string::npos) {
                token = token.substr(start, end - start + 1);
            }
            fields.push_back(token);
        }
        
        if (fields.size() < 9) continue;
        
        try {
            NavaidInfo navaid;
            navaid.type = parseNavaidType(fields[0]);
            navaid.identifier = fields[1];
            navaid.region = fields[2];
            // fields[3] - airport association (optional)
            navaid.position.latitude = std::stod(fields[4]);
            navaid.position.longitude = std::stod(fields[5]);
            navaid.magneticVariation = std::stod(fields[6]);
            
            // Frequency: MHz for VOR, kHz for NDB
            try {
                navaid.frequency = std::stod(fields[7]);
            } catch (...) {
                navaid.frequency = 0;
            }
            
            // Range in nautical miles
            try {
                navaid.range = std::stoi(fields[8]);
            } catch (...) {
                navaid.range = 150;  // Default
            }
            
            navaid.name = navaid.identifier;
            navaid.position.altitude = 0;
            navaid.position.heading = 0;
            
            navaids_[navaid.identifier] = navaid;
            count++;
            
        } catch (const std::exception& e) {
            std::cerr << "NavdataLoader: Error parsing navaid: " << line << std::endl;
        }
    }
    
    file.close();
    std::cout << "NavdataLoader: Loaded " << count << " navaids" << std::endl;
    return count;
}

int NavdataLoader::loadAirways(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "NavdataLoader: Cannot open airways file: " << filePath << std::endl;
        return 0;
    }
    
    int count = 0;
    std::string line;
    
    // Skip header
    if (!std::getline(file, line)) {
        file.close();
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> fields;
        
        while (std::getline(iss, token, ',')) {
            size_t start = token.find_first_not_of(" \t\r\n");
            size_t end = token.find_last_not_of(" \t\r\n");
            if (start != std::string::npos) {
                token = token.substr(start, end - start + 1);
            }
            fields.push_back(token);
        }
        
        if (fields.size() < 7) continue;
        
        try {
            AirwaySegment airway;
            airway.identifier = fields[0];
            airway.type = fields[1];  // "LOW" or "HIGH"
            
            // Start navaid
            std::string startIdent = fields[2];
            if (navaids_.find(startIdent) != navaids_.end()) {
                airway.startPoint = navaids_[startIdent];
            } else {
                continue;  // Skip if start navaid not found
            }
            
            // End navaid
            std::string endIdent = fields[3];
            if (navaids_.find(endIdent) != navaids_.end()) {
                airway.endPoint = navaids_[endIdent];
            } else {
                continue;  // Skip if end navaid not found
            }
            
            // Altitude constraints
            try {
                airway.minimumAltitude = std::stoi(fields[4]);
            } catch (...) {
                airway.minimumAltitude = 0;
            }
            
            try {
                airway.maximumAltitude = std::stoi(fields[5]);
            } catch (...) {
                airway.maximumAltitude = 0;
            }
            
            airways_.push_back(airway);
            count++;
            
        } catch (const std::exception& e) {
            std::cerr << "NavdataLoader: Error parsing airway: " << line << std::endl;
        }
    }
    
    file.close();
    std::cout << "NavdataLoader: Loaded " << count << " airways" << std::endl;
    return count;
}

int NavdataLoader::loadProcedures(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "NavdataLoader: Cannot open procedures file: " << filePath << std::endl;
        return 0;
    }
    
    int count = 0;
    std::string line;
    
    // Skip header
    if (!std::getline(file, line)) {
        file.close();
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> fields;
        
        while (std::getline(iss, token, ',')) {
            size_t start = token.find_first_not_of(" \t\r\n");
            size_t end = token.find_last_not_of(" \t\r\n");
            if (start != std::string::npos) {
                token = token.substr(start, end - start + 1);
            }
            fields.push_back(token);
        }
        
        if (fields.size() < 4) continue;
        
        try {
            Procedure proc;
            proc.airportICAO = fields[0];
            proc.type = fields[1];  // SID, STAR, APP
            proc.identifier = fields[2];
            proc.runway = fields[3];
            
            // Additional fields would contain waypoint info
            // This is a simplified loader - full implementation would parse all waypoints
            
            procedures_.push_back(proc);
            count++;
            
        } catch (const std::exception& e) {
            std::cerr << "NavdataLoader: Error parsing procedure: " << line << std::endl;
        }
    }
    
    file.close();
    std::cout << "NavdataLoader: Loaded " << count << " procedures" << std::endl;
    return count;
}

bool NavdataLoader::getNavaid(const std::string& identifier, NavaidInfo& navaid) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    auto it = navaids_.find(identifier);
    if (it != navaids_.end()) {
        navaid = it->second;
        return true;
    }
    
    return false;
}

std::vector<NavaidInfo> NavdataLoader::getNavaidsNearby(const Position& position,
                                                       double radiusNM,
                                                       const std::string& type) {
    std::vector<NavaidInfo> nearby;
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    for (const auto& pair : navaids_) {
        const NavaidInfo& navaid = pair.second;
        
        if (isWithinRadius(position, navaid.position, radiusNM)) {
            nearby.push_back(navaid);
        }
    }
    
    return nearby;
}

std::vector<AirwaySegment> NavdataLoader::getAirwaysFrom(const std::string& startIdent) {
    std::vector<AirwaySegment> fromStart;
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    for (const auto& airway : airways_) {
        if (airway.startPoint.identifier == startIdent) {
            fromStart.push_back(airway);
        }
    }
    
    return fromStart;
}

std::vector<Procedure> NavdataLoader::getSIDProcedures(const std::string& icao,
                                                      const std::string& runway) {
    std::vector<Procedure> sids;
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    for (const auto& proc : procedures_) {
        if (proc.airportICAO == icao && proc.type == "SID") {
            if (runway.empty() || proc.runway == runway) {
                sids.push_back(proc);
            }
        }
    }
    
    return sids;
}

std::vector<Procedure> NavdataLoader::getSTARProcedures(const std::string& icao,
                                                       const std::string& runway) {
    std::vector<Procedure> stars;
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    for (const auto& proc : procedures_) {
        if (proc.airportICAO == icao && proc.type == "STAR") {
            if (runway.empty() || proc.runway == runway) {
                stars.push_back(proc);
            }
        }
    }
    
    return stars;
}

std::vector<Procedure> NavdataLoader::getApproachProcedures(const std::string& icao,
                                                           const std::string& runway) {
    std::vector<Procedure> approaches;
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    for (const auto& proc : procedures_) {
        if (proc.airportICAO == icao && proc.type == "APP") {
            if (runway.empty() || proc.runway == runway) {
                approaches.push_back(proc);
            }
        }
    }
    
    return approaches;
}

void NavdataLoader::addNavaid(const NavaidInfo& navaid) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    navaids_[navaid.identifier] = navaid;
}

void NavdataLoader::addAirway(const AirwaySegment& airway) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    airways_.push_back(airway);
}

void NavdataLoader::addProcedure(const Procedure& proc) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    procedures_.push_back(proc);
}

std::tuple<int, int, int> NavdataLoader::getStatistics() const {
    std::lock_guard<std::mutex> lock(dbMutex_);
    return std::make_tuple(navaids_.size(), airways_.size(), procedures_.size());
}

void NavdataLoader::clear() {
    navaids_.clear();
    airways_.clear();
    procedures_.clear();
}

// Private helper methods

NavaidType NavdataLoader::parseNavaidType(const std::string& typeStr) {
    if (typeStr == "VOR") return NavaidType::VOR;
    if (typeStr == "NDB") return NavaidType::NDB;
    if (typeStr == "DME") return NavaidType::DME;
    if (typeStr == "TACAN") return NavaidType::TACAN;
    if (typeStr == "FIX" || typeStr == "INT") return NavaidType::INTERSECTION;
    if (typeStr == "AIRPORT") return NavaidType::AIRPORT;
    if (typeStr == "USER") return NavaidType::USER;
    return NavaidType::UNKNOWN;
}

double NavdataLoader::calculateDistance(const Position& p1, const Position& p2) const {
    double lat1 = p1.latitude * M_PI / 180.0;
    double lat2 = p2.latitude * M_PI / 180.0;
    double dLat = (p2.latitude - p1.latitude) * M_PI / 180.0;
    double dLon = (p2.longitude - p1.longitude) * M_PI / 180.0;
    
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(lat1) * std::cos(lat2) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    
    return EARTH_RADIUS_NM * c;
}

bool NavdataLoader::isWithinRadius(const Position& p1, const Position& p2, 
                                  double radiusNM) const {
    return calculateDistance(p1, p2) <= radiusNM;
}

} // namespace AICopilot

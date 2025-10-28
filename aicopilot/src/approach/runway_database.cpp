/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Runway Database Implementation
*****************************************************************************/

#include "../include/runway_database.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AICopilot {

RunwayDatabase::RunwayDatabase() = default;

RunwayDatabase::~RunwayDatabase() {
    shutdown();
}

bool RunwayDatabase::initialize(const std::string& csvFile) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    runwayData_.clear();
    
    if (!csvFile.empty()) {
        int loaded = loadFromCSV(csvFile);
        std::cout << "RunwayDatabase: Loaded " << loaded << " runways from " << csvFile << std::endl;
        return loaded > 0;
    }
    
    return true;
}

void RunwayDatabase::shutdown() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    runwayData_.clear();
}

bool RunwayDatabase::getRunway(const std::string& icao, const std::string& runwayId, 
                              RunwayInfo& runway) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    RunwayKey key{icao, runwayId};
    auto it = runwayData_.find(key);
    
    if (it != runwayData_.end()) {
        runway = it->second;
        return true;
    }
    
    return false;
}

std::vector<RunwayInfo> RunwayDatabase::getAirportRunways(const std::string& icao) {
    std::vector<RunwayInfo> runways;
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    for (const auto& pair : runwayData_) {
        if (pair.first.icao == icao) {
            runways.push_back(pair.second);
        }
    }
    
    return runways;
}

bool RunwayDatabase::selectRunwayForLanding(const std::string& icao,
                                          int windDirection, int windSpeed,
                                          int maxCrosswind,
                                          RunwayInfo& runway) {
    std::vector<RunwayInfo> runways = getAirportRunways(icao);
    
    if (runways.empty()) return false;
    
    RunwayInfo bestRunway = runways[0];
    double bestCrosswind = std::abs(calculateCrosswind(
        runways[0].headingMagnetic, windDirection, windSpeed));
    
    // Find runway with minimum crosswind
    for (const auto& rwy : runways) {
        double crosswind = calculateCrosswind(rwy.headingMagnetic, windDirection, windSpeed);
        double absCrosswind = std::abs(crosswind);
        
        // Prefer headwind over tailwind
        double headwind = calculateHeadwind(rwy.headingMagnetic, windDirection, windSpeed);
        
        if (absCrosswind < maxCrosswind && headwind >= -10.0) {  // Allow slight tailwind
            if (absCrosswind < bestCrosswind) {
                bestCrosswind = absCrosswind;
                bestRunway = rwy;
            }
        }
    }
    
    if (bestCrosswind > maxCrosswind) {
        return false;  // No suitable runway
    }
    
    runway = bestRunway;
    return true;
}

bool RunwayDatabase::selectRunwayForTakeoff(const std::string& icao,
                                          int windDirection, int windSpeed,
                                          RunwayInfo& runway) {
    std::vector<RunwayInfo> runways = getAirportRunways(icao);
    
    if (runways.empty()) return false;
    
    // For takeoff, prefer strong headwind
    RunwayInfo bestRunway = runways[0];
    double bestHeadwind = calculateHeadwind(
        runways[0].headingMagnetic, windDirection, windSpeed);
    
    for (const auto& rwy : runways) {
        double headwind = calculateHeadwind(rwy.headingMagnetic, windDirection, windSpeed);
        
        // Prefer runway with maximum headwind (or minimum tailwind)
        if (headwind > bestHeadwind) {
            bestHeadwind = headwind;
            bestRunway = rwy;
        }
    }
    
    runway = bestRunway;
    return true;
}

bool RunwayDatabase::getILSData(const std::string& icao, const std::string& runwayId, 
                               ILSData& ils) {
    RunwayInfo runway;
    if (getRunway(icao, runwayId, runway)) {
        ils = runway.ilsData;
        return ils.hasILS;
    }
    
    return false;
}

bool RunwayDatabase::isSuitableForLanding(const RunwayInfo& runway,
                                        int aircraftLength,
                                        int requiredDistance) {
    // Check runway length
    if (runway.LDA < requiredDistance) {
        return false;
    }
    
    // Check for closed/unusable runway
    if (runway.length == 0) {
        return false;
    }
    
    return true;
}

bool RunwayDatabase::isSuitableForTakeoff(const RunwayInfo& runway,
                                        int aircraftLength,
                                        int requiredDistance) {
    // Check runway length
    if (runway.TORA < requiredDistance) {
        return false;
    }
    
    // Check for closed runway
    if (runway.length == 0) {
        return false;
    }
    
    return true;
}

int RunwayDatabase::loadFromCSV(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "RunwayDatabase: Cannot open file: " << filePath << std::endl;
        return 0;
    }
    
    int count = 0;
    std::string line;
    
    // Skip header
    if (!std::getline(file, line)) {
        file.close();
        return 0;
    }
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> fields;
        
        while (std::getline(iss, token, ',')) {
            fields.push_back(token);
        }
        
        if (fields.size() < 19) continue;  // Not enough fields
        
        try {
            RunwayInfo runway;
            runway.icaoCode = fields[0];
            runway.runwayId = fields[1];
            runway.thresholdPosition.latitude = std::stod(fields[2]);
            runway.thresholdPosition.longitude = std::stod(fields[3]);
            runway.headingMagnetic = std::stoi(fields[4]);
            runway.length = std::stoi(fields[5]);
            runway.width = std::stoi(fields[6]);
            runway.surfaceType = fields[7];
            
            runway.ilsData.hasILS = (fields[8] == "1");
            if (runway.ilsData.hasILS) {
                runway.ilsData.localizerFrequency = std::stod(fields[9]);
                runway.ilsData.glideslopeFrequency = std::stod(fields[10]);
                runway.ilsData.localizerCourse = std::stod(fields[11]);
                runway.ilsData.glideslopeAngle = 3.0;  // Standard
                runway.ilsData.decisionHeight = std::stoi(fields[12]);
                runway.ilsData.category = fields[13];
                runway.ilsData.minimumRVR = std::stoi(fields[14]);
                runway.ilsData.thresholdPosition = runway.thresholdPosition;
            }
            
            runway.TODA = std::stoi(fields[15]);
            runway.TORA = std::stoi(fields[16]);
            runway.LDA = std::stoi(fields[17]);
            runway.ASDA = std::stoi(fields[18]);
            
            // Set defaults for optional fields
            runway.headingTrue = runway.headingMagnetic;
            runway.displaceThreshold = false;
            runway.displacedThresholdDistance = 0;
            runway.hasVGSI = false;
            runway.vgsiType = "";
            runway.designCode = 0;
            runway.friction = false;
            runway.grooving = false;
            runway.porous = false;
            runway.hasALS = false;
            runway.hasRWYLights = false;
            runway.hasREIL = false;
            
            addRunway(runway);
            count++;
            
        } catch (const std::exception& e) {
            std::cerr << "RunwayDatabase: Error parsing line: " << line << " (" << e.what() << ")" << std::endl;
        }
    }
    
    file.close();
    return count;
}

void RunwayDatabase::addRunway(const RunwayInfo& runway) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    RunwayKey key{runway.icaoCode, runway.runwayId};
    runwayData_[key] = runway;
}

int RunwayDatabase::getRunwayCount() const {
    std::lock_guard<std::mutex> lock(dbMutex_);
    return runwayData_.size();
}

void RunwayDatabase::clear() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    runwayData_.clear();
}

// Private helper methods

double RunwayDatabase::calculateCrosswind(int runwayHeading, int windDirection, 
                                        int windSpeed) const {
    double headingRad = runwayHeading * M_PI / 180.0;
    double windDirRad = windDirection * M_PI / 180.0;
    
    // Crosswind = wind speed × sin(wind direction - runway heading)
    double diff = windDirRad - headingRad;
    return windSpeed * std::sin(diff);
}

double RunwayDatabase::calculateHeadwind(int runwayHeading, int windDirection,
                                       int windSpeed) const {
    double headingRad = runwayHeading * M_PI / 180.0;
    double windDirRad = windDirection * M_PI / 180.0;
    
    // Headwind = wind speed × cos(wind direction - runway heading)
    double diff = windDirRad - headingRad;
    return windSpeed * std::cos(diff);
}

std::string RunwayDatabase::getReciprocal(const std::string& runwayId) const {
    // Parse runway heading from ID (e.g., "04L" -> 04, "22R" -> 22)
    int heading = parseRunwayHeading(runwayId);
    int reciprocal = (heading + 18) % 36;
    
    std::string recId;
    if (reciprocal < 10) recId += "0";
    recId += std::to_string(reciprocal);
    
    // Add suffix
    if (runwayId.length() > 2) {
        char suffix = runwayId[2];
        if (suffix == 'L') recId += 'R';
        else if (suffix == 'R') recId += 'L';
        else if (suffix == 'C') recId += 'C';
    }
    
    return recId;
}

int RunwayDatabase::parseRunwayHeading(const std::string& runwayId) const {
    try {
        return std::stoi(runwayId.substr(0, 2));
    } catch (...) {
        return 0;
    }
}

} // namespace AICopilot

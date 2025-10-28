/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Weather Database Implementation - METAR parsing
*****************************************************************************/

#include "../include/weather_database.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <cmath>

namespace AICopilot {

WeatherDatabase::WeatherDatabase() = default;

WeatherDatabase::~WeatherDatabase() {
    shutdown();
}

bool WeatherDatabase::initialize(const std::string& cacheFile) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    reportCache_.clear();
    
    if (!cacheFile.empty()) {
        int loaded = loadMETARsFromFile(cacheFile);
        std::cout << "WeatherDatabase: Loaded " << loaded << " METAR reports from cache" << std::endl;
    }
    
    return true;
}

void WeatherDatabase::shutdown() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    reportCache_.clear();
}

METARReport WeatherDatabase::parseMETAR(const std::string& metarString) {
    METARReport report;
    report.isValid = false;
    report.observationTime = std::time(nullptr);
    report.windGust = 0;
    report.windVariable = false;
    report.thunderstorm = false;
    report.precipitation = false;
    report.icing = false;
    report.lowVisibility = false;
    report.ceilingFeet = 10000;  // Default no ceiling
    
    // Tokenize METAR string
    std::istringstream iss(metarString);
    std::string token;
    std::vector<std::string> tokens;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        report.parseError = "Empty METAR string";
        return report;
    }
    
    try {
        size_t i = 0;
        
        // Station ID (required)
        if (i < tokens.size()) {
            if (!parseStationId(tokens[i], report)) {
                report.parseError = "Invalid station ID";
                return report;
            }
            i++;
        }
        
        // Date/Time (required)
        if (i < tokens.size()) {
            if (!parseDateTime(tokens[i], report)) {
                report.parseError = "Invalid date/time";
                return report;
            }
            i++;
        }
        
        // Wind (required)
        if (i < tokens.size()) {
            parseWind(tokens[i], report);
            i++;
        }
        
        // Parse remaining tokens
        while (i < tokens.size()) {
            const std::string& tok = tokens[i];
            
            // Handle RMK (remarks) section
            if (tok == "RMK") {
                i++;
                while (i < tokens.size()) {
                    parseRemarks(tokens[i], report);
                    i++;
                }
                break;
            }
            
            // Try parsing each section
            if (parseVisibility(tok, report)) {
                i++;
            } else if (parseWeatherPhenomena(tok, report)) {
                i++;
            } else if (parseCloudLayer(tok, report)) {
                i++;
            } else if (parseTemperatureDewpoint(tok, report)) {
                i++;
            } else if (parseAltimeter(tok, report)) {
                i++;
            } else {
                // Unknown token, skip it
                i++;
            }
        }
        
        // Derive conditions from parsed data
        deriveConditions(report);
        
        report.isValid = true;
        return report;
        
    } catch (const std::exception& e) {
        report.parseError = std::string("Parse exception: ") + e.what();
        return report;
    }
}

bool WeatherDatabase::updateWeather(const std::string& icao, const std::string& metarString) {
    METARReport report = parseMETAR(metarString);
    if (!report.isValid) {
        return false;
    }
    
    report.stationId = icao;
    
    std::lock_guard<std::mutex> lock(cacheMutex_);
    CachedReport cached;
    cached.report = report;
    cached.cacheTime = std::time(nullptr);
    reportCache_[icao] = cached;
    
    return true;
}

bool WeatherDatabase::getWeather(const std::string& icao, METARReport& report) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    auto it = reportCache_.find(icao);
    if (it == reportCache_.end()) {
        return false;
    }
    
    if (isExpired(it->second)) {
        reportCache_.erase(it);
        return false;
    }
    
    report = it->second.report;
    return true;
}

WeatherConditions WeatherDatabase::getConditions(const METARReport& report) {
    WeatherConditions conditions;
    
    conditions.windSpeed = report.windSpeed;
    conditions.windDirection = report.windDirection;
    conditions.visibility = report.visibility;  // Already in statute miles
    
    // Calculate cloud base (ceiling)
    conditions.cloudBase = report.ceilingFeet;
    
    conditions.temperature = report.temperature;
    conditions.icing = report.icing;
    conditions.turbulence = report.windSpeed > 25;  // Rough estimate
    conditions.precipitation = report.precipitation;
    
    return conditions;
}

bool WeatherDatabase::isSuitableForFlight(const METARReport& report) {
    // Minimum VFR: 3 SM visibility, 1000 ft ceiling
    if (report.visibility < 3) return false;
    if (report.ceilingFeet < 1000) return false;
    if (report.windSpeed > 30) return false;  // Excessive wind
    if (report.icing && report.temperature < 5) return false;  // Icing conditions
    
    return true;
}

bool WeatherDatabase::isSuitableForTakeoff(const METARReport& report) {
    // Takeoff minimums typically: 1 SM visibility, 500 ft ceiling
    if (report.visibility < 1.0) return false;
    if (report.ceilingFeet < 500) return false;
    if (report.windSpeed > 35) return false;  // Crosswind limit
    
    return true;
}

bool WeatherDatabase::isSuitableForLanding(const METARReport& report) {
    // Landing minimums typically: 0.5 SM visibility, 300 ft ceiling
    if (report.visibility < 0.5) return false;
    if (report.ceilingFeet < 300) return false;
    if (report.windSpeed > 40) return false;  // Crosswind limit
    
    return true;
}

int WeatherDatabase::loadMETARsFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "WeatherDatabase: Cannot open file: " << filepath << std::endl;
        return 0;
    }
    
    int count = 0;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;  // Skip comments
        
        METARReport report = parseMETAR(line);
        if (report.isValid) {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            CachedReport cached;
            cached.report = report;
            cached.cacheTime = std::time(nullptr);
            reportCache_[report.stationId] = cached;
            count++;
        }
    }
    
    file.close();
    return count;
}

bool WeatherDatabase::saveMETARsToFile(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "WeatherDatabase: Cannot create file: " << filepath << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    file << "# Cached METAR reports - one per line\n";
    file << "# Format: STATIONID DATETIME WIND VISIBILITY WEATHER CLOUDS TEMP/DP ALTIMETER\n\n";
    
    for (const auto& pair : reportCache_) {
        const METARReport& report = pair.second.report;
        
        // Reconstruct METAR string (simplified)
        file << report.stationId << " ";
        
        // Add other fields as needed
        file << "# Cached from " << std::ctime(&pair.second.cacheTime);
    }
    
    file.close();
    return true;
}

void WeatherDatabase::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    reportCache_.clear();
}

int WeatherDatabase::getCacheSize() const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    return reportCache_.size();
}

// Private methods

bool WeatherDatabase::parseStationId(const std::string& token, METARReport& report) {
    if (token.length() < 3 || token.length() > 4) {
        return false;
    }
    
    // Verify it's all uppercase letters (or digits for some international codes)
    for (char c : token) {
        if (!std::isupper(c) && !std::isdigit(c)) {
            return false;
        }
    }
    
    report.stationId = token;
    return true;
}

bool WeatherDatabase::parseDateTime(const std::string& token, METARReport& report) {
    // Format: DDHHMMZ or DDHHMM where DD=day, HH=hour, MM=minute
    if (token.length() < 6 || token.length() > 7) {
        return false;
    }
    
    // Must end with Z
    if (token.back() != 'Z' && !std::isdigit(token.back())) {
        return false;
    }
    
    try {
        int day = std::stoi(token.substr(0, 2));
        int hour = std::stoi(token.substr(2, 2));
        int minute = std::stoi(token.substr(4, 2));
        
        if (day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59) {
            return false;
        }
        
        // Store in report (UTC time)
        // In real implementation, calculate actual Unix timestamp
        report.observationTime = std::time(nullptr);
        
        return true;
    } catch (...) {
        return false;
    }
}

bool WeatherDatabase::parseWind(const std::string& token, METARReport& report) {
    // Format: DDDSSKT or DDDSSGTKT (direction, speed, optional gust, knots)
    // Examples: 31008KT, 180V220KT, 31008G15KT
    
    if (token.find("KT") == std::string::npos) {
        return false;
    }
    
    // Handle variable wind (VRB prefix)
    if (token.substr(0, 3) == "VRB") {
        report.windVariable = true;
        report.windDirection = 0;
        
        size_t ktPos = token.find("KT");
        if (ktPos != std::string::npos && ktPos > 3) {
            try {
                report.windSpeed = std::stoi(token.substr(3, ktPos - 3));
                return true;
            } catch (...) {
                return false;
            }
        }
        return false;
    }
    
    // Regular wind format
    if (token.length() < 5) return false;
    
    try {
        report.windDirection = std::stoi(token.substr(0, 3));
        
        size_t speedStart = 3;
        size_t gPos = token.find('G');
        
        if (gPos != std::string::npos) {
            // Has gust
            report.windSpeed = std::stoi(token.substr(speedStart, gPos - speedStart));
            size_t ktPos = token.find("KT");
            report.windGust = std::stoi(token.substr(gPos + 1, ktPos - gPos - 1));
        } else {
            // No gust
            size_t ktPos = token.find("KT");
            report.windSpeed = std::stoi(token.substr(speedStart, ktPos - speedStart));
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool WeatherDatabase::parseVisibility(const std::string& token, METARReport& report) {
    // Format: 10SM, 5SM, 1/4SM, P6SM (greater than 6)
    
    if (token.find("SM") == std::string::npos) {
        return false;
    }
    
    try {
        std::string visStr = token.substr(0, token.find("SM"));
        
        if (visStr[0] == 'P') {
            // Greater than 6 miles
            report.visibility = 10;  // Default to 10
        } else if (visStr.find('/') != std::string::npos) {
            // Fractional (e.g., 1/4)
            size_t slashPos = visStr.find('/');
            int num = std::stoi(visStr.substr(0, slashPos));
            int denom = std::stoi(visStr.substr(slashPos + 1));
            report.visibility = num / denom;
        } else {
            // Regular number
            report.visibility = std::stoi(visStr);
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool WeatherDatabase::parseWeatherPhenomena(const std::string& token, METARReport& report) {
    // Format: -RA, +RA, TS, CAVOK, VV001 (vertical visibility), etc.
    
    // Special case: CAVOK (no weather)
    if (token == "CAVOK") {
        report.visibility = 10;
        report.weatherPhenomena.push_back("CAVOK");
        return true;
    }
    
    // Vertical visibility format: VVxxx (e.g., VV001 = 100 ft vertical visibility)
    if (token.substr(0, 2) == "VV") {
        try {
            int vv = std::stoi(token.substr(2));
            report.ceilingFeet = vv * 100;
            return true;
        } catch (...) {
            return false;
        }
    }
    
    // Weather codes: RA (rain), SN (snow), SG (snow grains), PL (ice pellets),
    //               GR (hail), GS (small hail), UP (unknown precip), TS (thunderstorm),
    //               FZ (freezing), VC (vicinity)
    
    if (token.length() < 2) return false;
    
    std::vector<std::string> weatherCodes = {
        "RA", "SN", "SG", "PL", "GR", "GS", "UP", "TS", "FZ", "VC", "DZ", "BR", "FG", "FU"
    };
    
    for (const auto& code : weatherCodes) {
        if (token.find(code) != std::string::npos) {
            report.weatherPhenomena.push_back(token);
            return true;
        }
    }
    
    return false;
}

bool WeatherDatabase::parseCloudLayer(const std::string& token, METARReport& report) {
    // Format: FEW010, SCT025, BKN040, OVC080, SKC, CLR
    
    std::vector<std::string> coverageCodes = {"FEW", "SCT", "BKN", "OVC", "SKC", "CLR"};
    
    for (const auto& code : coverageCodes) {
        if (token.substr(0, code.length()) == code) {
            CloudLayer layer;
            layer.coverage = code;
            
            if (code == "SKC" || code == "CLR") {
                layer.altitude = 0;
            } else {
                try {
                    layer.altitude = std::stoi(token.substr(code.length())) * 100;  // In hundreds of feet
                } catch (...) {
                    return false;
                }
            }
            
            // Check for cloud type (CB, TCU)
            if (token.length() > code.length() + 3) {
                std::string remainder = token.substr(code.length() + 3);
                if (remainder == "CB" || remainder == "TCU") {
                    layer.cloudType = remainder;
                }
            }
            
            report.clouds.push_back(layer);
            return true;
        }
    }
    
    return false;
}

bool WeatherDatabase::parseTemperatureDewpoint(const std::string& token, METARReport& report) {
    // Format: 23/14, M05/M08 (with minus sign)
    
    if (token.find('/') == std::string::npos) {
        return false;
    }
    
    try {
        size_t slashPos = token.find('/');
        std::string tempStr = token.substr(0, slashPos);
        std::string dpStr = token.substr(slashPos + 1);
        
        // Parse temperature
        int sign = 1;
        if (tempStr[0] == 'M') {
            sign = -1;
            tempStr = tempStr.substr(1);
        }
        report.temperature = sign * std::stoi(tempStr);
        
        // Parse dewpoint
        sign = 1;
        if (dpStr[0] == 'M') {
            sign = -1;
            dpStr = dpStr.substr(1);
        }
        report.dewpoint = sign * std::stoi(dpStr);
        
        return true;
    } catch (...) {
        return false;
    }
}

bool WeatherDatabase::parseAltimeter(const std::string& token, METARReport& report) {
    // Format: A3000, A2990, etc. (inHg × 100)
    
    if (token[0] != 'A' || token.length() != 5) {
        return false;
    }
    
    try {
        int altSetting = std::stoi(token.substr(1));
        report.altimeterSetting = altSetting / 100.0;
        return true;
    } catch (...) {
        return false;
    }
}

bool WeatherDatabase::parseRemarks(const std::string& token, METARReport& report) {
    // Store remarks for later processing if needed
    report.remarks.push_back(token);
    return true;
}

bool WeatherDatabase::isExpired(const CachedReport& cached) const {
    time_t now = std::time(nullptr);
    time_t age = now - cached.cacheTime;
    return age > (METAR_EXPIRATION_MINUTES * 60);
}

void WeatherDatabase::deriveConditions(METARReport& report) {
    // Determine if thunderstorm is present
    for (const auto& phenom : report.weatherPhenomena) {
        if (phenom.find("TS") != std::string::npos) {
            report.thunderstorm = true;
        }
        
        // Precipitation
        if (phenom.find("RA") != std::string::npos ||
            phenom.find("SN") != std::string::npos ||
            phenom.find("PL") != std::string::npos ||
            phenom.find("GR") != std::string::npos) {
            report.precipitation = true;
        }
    }
    
    // Determine if low visibility
    report.lowVisibility = (report.visibility < 3);
    
    // Calculate ceiling from cloud layers
    report.ceilingFeet = calculateCeiling(report);
    
    // Infer icing conditions
    // Icing typically occurs when temp is 0-15°C and moisture present
    report.icing = (report.temperature >= -20 && report.temperature <= 10) &&
                   (report.precipitation || report.thunderstorm);
}

double WeatherDatabase::calculateCeiling(const METARReport& report) const {
    // Ceiling is the lowest layer of BKN or OVC
    for (const auto& layer : report.clouds) {
        if (layer.coverage == "BKN" || layer.coverage == "OVC") {
            return layer.altitude;
        }
    }
    
    // No ceiling if only FEW/SCT
    return 10000;  // Default if no BKN/OVC
}

} // namespace AICopilot

/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* METAR Parser Implementation - High-performance METAR parsing
* Handles ICAO format with error recovery
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "metar_parser.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace AICopilot {

// ============================================================================
// Regex Pattern Definitions
// ============================================================================

const std::string METARParser::WIND_PATTERN = "^[0-9]{3}|VRB[0-9]{2,3}";
const std::string METARParser::VISIBILITY_PATTERN = "^[0-9]{4}$|^[0-9]+SM$";
const std::string METARParser::CLOUD_PATTERN = "^(FEW|SCT|BKN|OVC|SKC|CLR|VV)";
const std::string METARParser::TEMP_PATTERN = "^M?[0-9]{2}/M?[0-9]{2}$";
const std::string METARParser::ALTIMETER_PATTERN = "^[AQ][0-9]{4}$";

// ============================================================================
// Core Parsing Methods
// ============================================================================

bool METARParser::parseWind(
    const std::string& windToken,
    int& direction,
    int& speed,
    int& gust,
    bool& variable,
    int& minDir,
    int& maxDir) {
    
    direction = 0;
    speed = 0;
    gust = 0;
    variable = false;
    minDir = 0;
    maxDir = 0;
    
    if (windToken.empty() || windToken.size() < 7) return false;
    
    try {
        // Check for variable wind (VRB)
        if (windToken.substr(0, 3) == "VRB") {
            variable = true;
            speed = std::stoi(windToken.substr(3, 2));
        } else {
            // Parse direction (first 3 chars)
            direction = std::stoi(windToken.substr(0, 3));
            if (direction < 0 || direction > 360) return false;
            
            // Parse speed (next 2-3 chars)
            size_t speedStart = 3;
            size_t speedLen = 0;
            while (speedStart + speedLen < windToken.size() && 
                   std::isdigit(windToken[speedStart + speedLen])) {
                speedLen++;
            }
            if (speedLen == 0 || speedLen > 3) return false;
            speed = std::stoi(windToken.substr(speedStart, speedLen));
        }
        
        // Parse gusts (G prefix)
        size_t gustPos = windToken.find('G');
        if (gustPos != std::string::npos && gustPos + 2 <= windToken.size()) {
            size_t gustLen = 0;
            while (gustPos + 1 + gustLen < windToken.size() && 
                   std::isdigit(windToken[gustPos + 1 + gustLen])) {
                gustLen++;
            }
            if (gustLen > 0) {
                gust = std::stoi(windToken.substr(gustPos + 1, gustLen));
            }
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool METARParser::parseVisibility(
    const std::string& visToken,
    double& visibilitySmiles) {
    
    visibilitySmiles = 10.0;  // Default unlimited
    
    if (visToken.empty()) return false;
    
    try {
        // Format: 9999, 10SM, 1/4SM, etc.
        if (visToken.find("SM") != std::string::npos) {
            // Statute miles format
            std::string numPart = visToken.substr(0, visToken.find("SM"));
            
            if (numPart.find('/') != std::string::npos) {
                // Fractional format (1/4SM)
                size_t slashPos = numPart.find('/');
                double numerator = std::stod(numPart.substr(0, slashPos));
                double denominator = std::stod(numPart.substr(slashPos + 1));
                if (denominator != 0) {
                    visibilitySmiles = numerator / denominator;
                }
            } else {
                visibilitySmiles = std::stod(numPart);
            }
            return true;
        } else if (std::all_of(visToken.begin(), visToken.end(), ::isdigit)) {
            // Meter format (4 digits: 9999)
            int meters = std::stoi(visToken);
            visibilitySmiles = convertVisibilityToStatuteMiles(meters, 'M');
            return true;
        }
    } catch (...) {
        return false;
    }
    
    return false;
}

bool METARParser::parseCloudLayer(
    const std::string& cloudToken,
    CloudCoverage& coverage,
    int& altitudeAgl,
    bool& isCB,
    bool& isTCU) {
    
    coverage = CloudCoverage::UNKNOWN;
    altitudeAgl = 0;
    isCB = false;
    isTCU = false;
    
    if (cloudToken.empty() || cloudToken.size() < 3) return false;
    
    try {
        // Parse coverage (first 3 chars)
        std::string covStr = cloudToken.substr(0, 3);
        if (covStr == "SKC") {
            coverage = CloudCoverage::SKC;
            return true;
        } else if (covStr == "CLR") {
            coverage = CloudCoverage::CLR;
            return true;
        } else if (covStr == "FEW") {
            coverage = CloudCoverage::FEW;
        } else if (covStr == "SCT") {
            coverage = CloudCoverage::SCT;
        } else if (covStr == "BKN") {
            coverage = CloudCoverage::BKN;
        } else if (covStr == "OVC") {
            coverage = CloudCoverage::OVC;
        } else if (covStr == "VV") {
            coverage = CloudCoverage::VV;
            // Vertical visibility format: VV002
            if (cloudToken.size() >= 5) {
                altitudeAgl = std::stoi(cloudToken.substr(2, 3)) * 100;  // VV values are in hundreds
            }
            return true;
        } else {
            return false;
        }
        
        // Parse altitude (3 digits = hundreds of feet)
        if (cloudToken.size() >= 6) {
            std::string altStr = cloudToken.substr(3, 3);
            if (std::all_of(altStr.begin(), altStr.end(), ::isdigit)) {
                altitudeAgl = std::stoi(altStr) * 100;
            } else {
                return false;
            }
        } else {
            return false;
        }
        
        // Check for cloud type (CB, TCU)
        if (cloudToken.find("CB") != std::string::npos) {
            isCB = true;
        }
        if (cloudToken.find("TCU") != std::string::npos) {
            isTCU = true;
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool METARParser::parseTemperatureDewpoint(
    const std::string& tempToken,
    int& temperature,
    int& dewpoint) {
    
    temperature = 0;
    dewpoint = 0;
    
    if (tempToken.empty() || tempToken.find('/') == std::string::npos) return false;
    
    try {
        size_t slashPos = tempToken.find('/');
        std::string tempStr = tempToken.substr(0, slashPos);
        std::string dewStr = tempToken.substr(slashPos + 1);
        
        // Parse temperature
        if (tempStr.substr(0, 1) == "M") {
            temperature = -std::stoi(tempStr.substr(1));
        } else {
            temperature = std::stoi(tempStr);
        }
        
        // Parse dewpoint
        if (dewStr.substr(0, 1) == "M") {
            dewpoint = -std::stoi(dewStr.substr(1));
        } else {
            dewpoint = std::stoi(dewStr);
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool METARParser::parseAltimeter(
    const std::string& altimeterToken,
    double& altimeterInHg,
    double& altimeterMbar) {
    
    altimeterInHg = 29.92;  // Standard pressure
    altimeterMbar = 1013.0;
    
    if (altimeterToken.empty() || altimeterToken.size() < 5) return false;
    
    try {
        char unit = altimeterToken[0];  // 'A' for inHg, 'Q' for hectopascals
        std::string valueStr = altimeterToken.substr(1);
        
        if (!std::all_of(valueStr.begin(), valueStr.end(), ::isdigit)) return false;
        
        if (unit == 'A') {
            // inHg format: A3012 = 30.12 inHg
            int value = std::stoi(valueStr);
            altimeterInHg = value / 100.0;
            altimeterMbar = altimeterInHg * 33.8639;
            return true;
        } else if (unit == 'Q') {
            // Hectopascals format: Q1018 = 1018 mb
            int value = std::stoi(valueStr);
            altimeterMbar = value;
            altimeterInHg = value / 33.8639;
            return true;
        }
        
        return false;
    } catch (...) {
        return false;
    }
}

bool METARParser::parseWeatherPhenomena(
    const std::string& phenomToken,
    WeatherPhenomenaCode& phenomena,
    int& intensity) {
    
    phenomena = WeatherPhenomenaCode::UNKNOWN;
    intensity = 0;
    
    if (phenomToken.empty()) return false;
    
    std::string token = phenomToken;
    
    // Check for intensity prefix
    if (token[0] == '+') {
        intensity = 1;  // Heavy
        token = token.substr(1);
    } else if (token[0] == '-') {
        intensity = -1;  // Light
        token = token.substr(1);
    }
    
    // Parse weather code
    if (token == "RA") phenomena = WeatherPhenomenaCode::RA;
    else if (token == "SN") phenomena = WeatherPhenomenaCode::SN;
    else if (token == "RASN") phenomena = WeatherPhenomenaCode::RASN;
    else if (token == "SG") phenomena = WeatherPhenomenaCode::SG;
    else if (token == "IC") phenomena = WeatherPhenomenaCode::IC;
    else if (token == "PE") phenomena = WeatherPhenomenaCode::PE;
    else if (token == "GR") phenomena = WeatherPhenomenaCode::GR;
    else if (token == "GS") phenomena = WeatherPhenomenaCode::GS;
    else if (token == "UP") phenomena = WeatherPhenomenaCode::UP;
    else if (token == "DZ") phenomena = WeatherPhenomenaCode::DZ;
    else if (token == "PL") phenomena = WeatherPhenomenaCode::PL;
    else if (token == "FZRA") phenomena = WeatherPhenomenaCode::FZRA;
    else if (token == "FZDZ") phenomena = WeatherPhenomenaCode::FZDZ;
    else if (token == "TS") phenomena = WeatherPhenomenaCode::TS;
    else if (token == "TSRA") phenomena = WeatherPhenomenaCode::TSRA;
    else if (token == "TSSN") phenomena = WeatherPhenomenaCode::TSSN;
    else if (token == "TSGS") phenomena = WeatherPhenomenaCode::TSGS;
    else if (token == "TSGR") phenomena = WeatherPhenomenaCode::TSGR;
    else if (token == "FG") phenomena = WeatherPhenomenaCode::FG;
    else if (token == "MIFG") phenomena = WeatherPhenomenaCode::MIFG;
    else if (token == "CAVOK") phenomena = WeatherPhenomenaCode::CAVOK;
    else return false;
    
    return phenomena != WeatherPhenomenaCode::UNKNOWN;
}

// ============================================================================
// Utility Methods
// ============================================================================

std::string METARParser::identifyTokenType(const std::string& token) {
    if (matchesPattern(token, "^[0-9]{3}[0-9]{2,3}")) return "wind";
    if (matchesPattern(token, "^[0-9]{4}$|^[0-9]+SM$")) return "visibility";
    if (matchesPattern(token, "^(FEW|SCT|BKN|OVC|SKC|CLR|VV)")) return "clouds";
    if (matchesPattern(token, "^M?[0-9]{2}/M?[0-9]{2}$")) return "temperature";
    if (matchesPattern(token, "^[AQ][0-9]{4}$")) return "altimeter";
    if (matchesPattern(token, "^[+\\-]?[A-Z]{2,4}$")) return "weather";
    return "unknown";
}

bool METARParser::matchesPattern(
    const std::string& token,
    const std::string& pattern) {
    
    try {
        std::regex re(pattern);
        return std::regex_search(token, re);
    } catch (...) {
        return false;
    }
}

bool METARParser::extractSignedInteger(
    const std::string& token,
    size_t startPos,
    size_t length,
    int& value) {
    
    if (startPos + length > token.size()) return false;
    
    try {
        std::string numStr = token.substr(startPos, length);
        bool negative = false;
        
        if (numStr[0] == 'M') {
            negative = true;
            numStr = numStr.substr(1);
        }
        
        value = std::stoi(numStr);
        if (negative) value = -value;
        return true;
    } catch (...) {
        return false;
    }
}

bool METARParser::extractDouble(
    const std::string& token,
    double& value) {
    
    try {
        value = std::stod(token);
        return true;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// Conversion Methods
// ============================================================================

double METARParser::convertVisibilityToStatuteMiles(
    double value,
    char fromUnit) {
    
    switch (fromUnit) {
        case 'M':  // Meters
            return value / Visibility::STATUTE_MILE_TO_METERS;
        case 'K':  // Kilometers
            return value / Visibility::STATUTE_MILE_TO_KM;
        case 'F':  // Feet
            return value / Visibility::STATUTE_MILE_TO_FEET;
        default:
            return value;
    }
}

double METARParser::convertVisibilityFromStatuteMiles(
    double smiles,
    char toUnit) {
    
    switch (toUnit) {
        case 'M':  // Meters
            return smiles * Visibility::STATUTE_MILE_TO_METERS;
        case 'K':  // Kilometers
            return smiles * Visibility::STATUTE_MILE_TO_KM;
        default:
            return smiles;
    }
}

double METARParser::feetToMeters(double feet) {
    return feet * 0.3048;
}

double METARParser::metersToFeet(double meters) {
    return meters / 0.3048;
}

double METARParser::convertWindSpeed(double knots, char toUnit) {
    switch (toUnit) {
        case 'K':  // Kilometers per hour
            return knots * 1.852;
        case 'M':  // Meters per second
            return knots * 0.51444444;
        default:
            return knots;
    }
}

std::string METARParser::normalizeICAOCode(const std::string& code) {
    std::string normalized = code;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::toupper);
    return normalized;
}

bool METARParser::isValidICAOCode(const std::string& code) {
    if (code.size() != 4) return false;
    for (char c : code) {
        if (!std::isalpha(c)) return false;
    }
    return true;
}

std::vector<std::string> METARParser::tokenizeMETAR(
    const std::string& metarString) {
    
    std::vector<std::string> tokens;
    std::istringstream iss(metarString);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

bool METARParser::isCAVOK(
    const std::vector<WeatherPhenomenaCode>& weatherCodes,
    double visibility,
    double ceiling) {
    
    // CAVOK: Ceiling and Visibility OK
    // - Visibility >= 10 SM
    // - No clouds below 5000 ft
    // - No weather phenomena
    
    for (const auto& code : weatherCodes) {
        if (code == WeatherPhenomenaCode::CAVOK) return true;
    }
    
    return visibility >= 10.0 && ceiling >= 5000.0 && weatherCodes.empty();
}

// ============================================================================
// Private Helper Methods
// ============================================================================

int METARParser::parseSignedInt(const std::string& str) {
    try {
        if (str.empty()) return 0;
        if (str[0] == 'M') {
            return -std::stoi(str.substr(1));
        }
        return std::stoi(str);
    } catch (...) {
        return 0;
    }
}

double METARParser::parseFraction(const std::string& str) {
    try {
        size_t slashPos = str.find('/');
        if (slashPos == std::string::npos) return 0.0;
        
        double numerator = std::stod(str.substr(0, slashPos));
        double denominator = std::stod(str.substr(slashPos + 1));
        
        if (denominator == 0.0) return 0.0;
        return numerator / denominator;
    } catch (...) {
        return 0.0;
    }
}

}  // namespace AICopilot

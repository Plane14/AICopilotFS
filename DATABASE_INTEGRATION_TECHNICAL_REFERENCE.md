# Database Integration - Technical Reference & Code Examples

**Document:** Implementation Guide for Database Integration  
**Project:** AICopilotFS  
**Date:** October 27, 2025

---

## TABLE OF CONTENTS
1. [Terrain Database - HGT Parser](#terrain-hgt-parser)
2. [Weather System - METAR Parser](#weather-metar-parser)
3. [Navigation Database - FAX Parser](#navigation-fax-parser)
4. [Database Caching Architecture](#database-caching-architecture)
5. [Integration Code Examples](#integration-code-examples)

---

## TERRAIN HGT PARSER

### File Format Specification

**HGT File Naming Convention:**
```
<latitude><longitude>.hgt
Example: N40W074.hgt (New York area)

Format:
  N = North latitude (S = South)
  40 = Latitude value
  W = West longitude (E = East)
  074 = Longitude value

File Size: 1,442,401 bytes (1201 × 1201 samples × 2 bytes each)
```

### Header Implementation

```cpp
// File: aicopilot/include/terrain/hgt_loader.h
#ifndef HGT_LOADER_H
#define HGT_LOADER_H

#include <string>
#include <vector>
#include <cstdint>
#include "../aicopilot_types.h"

namespace AICopilot {
namespace Terrain {

class HGTLoader {
public:
    /**
     * Load elevation data from HGT file
     * @param filePath Path to .hgt file
     * @return true if successful
     */
    bool loadFromFile(const std::string& filePath);
    
    /**
     * Get elevation at latitude/longitude
     * @param latitude [-90, 90]
     * @param longitude [-180, 180]
     * @return Elevation in feet MSL, or -32768 for invalid/water
     */
    int16_t getElevation(double latitude, double longitude) const;
    
    /**
     * Interpolate elevation (bilinear)
     * @return Interpolated elevation in feet
     */
    double interpolateElevation(double latitude, double longitude) const;
    
    /**
     * Get bounding box of this tile
     */
    void getBounds(double& minLat, double& maxLat, 
                   double& minLon, double& maxLon) const;
    
    bool isLoaded() const { return data_.size() > 0; }
    
private:
    // 1201 × 1201 grid of elevation samples
    std::vector<int16_t> data_;
    
    // Tile origin
    double originLatitude_;   // NW corner
    double originLongitude_;  // NW corner
    
    // Convert lat/lon to grid indices
    bool latLonToIndices(double lat, double lon, 
                        int& row, int& col) const;
    
    // Read big-endian int16 from byte stream
    static int16_t readBigEndianInt16(const uint8_t* ptr);
};

class TerrainTileCache {
public:
    TerrainTileCache(size_t maxTiles = 16);
    
    /**
     * Get or load tile from cache
     */
    std::shared_ptr<HGTLoader> getTile(double latitude, double longitude);
    
    /**
     * Set cache directory
     */
    void setDataDirectory(const std::string& path) { dataDir_ = path; }
    
    /**
     * Get elevation with automatic tile loading
     */
    double getElevation(double latitude, double longitude);
    
    /**
     * Preload tiles around a position
     */
    void preloadTiles(double latitude, double longitude, double radiusDegrees);
    
private:
    std::map<std::string, std::shared_ptr<HGTLoader>> cache_;
    std::queue<std::string> loadOrder_;  // LRU tracking
    size_t maxTiles_;
    std::string dataDir_;
    
    std::string getTileKey(double lat, double lon) const;
    std::string getTilePath(double lat, double lon) const;
};

} // namespace Terrain
} // namespace AICopilot

#endif // HGT_LOADER_H
```

### Implementation

```cpp
// File: aicopilot/src/terrain/hgt_loader.cpp
#include "hgt_loader.h"
#include <fstream>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace AICopilot {
namespace Terrain {

// ============================================================================
// HGTLoader Implementation
// ============================================================================

bool HGTLoader::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open HGT file: " << filePath << std::endl;
        return false;
    }
    
    // Extract tile coordinates from filename
    // Example: N40W074.hgt
    std::string filename = filePath.substr(filePath.find_last_of("/\\") + 1);
    filename = filename.substr(0, filename.find_last_of("."));
    
    if (filename.length() != 8) {
        std::cerr << "Invalid HGT filename format: " << filename << std::endl;
        return false;
    }
    
    // Parse latitude
    char latDir = filename[0];  // N or S
    int latValue = std::stoi(filename.substr(1, 2));
    originLatitude_ = (latDir == 'S') ? -latValue : latValue;
    
    // Parse longitude
    char lonDir = filename[3];  // E or W
    int lonValue = std::stoi(filename.substr(4, 3));
    originLongitude_ = (lonDir == 'W') ? -lonValue : lonValue;
    
    // Latitude range: [originLatitude_, originLatitude_ + 1.0)
    // Longitude range: [originLongitude_, originLongitude_ + 1.0)
    
    // Read all elevation data
    data_.resize(1201 * 1201);
    
    for (size_t i = 0; i < data_.size(); ++i) {
        uint8_t buffer[2];
        file.read((char*)buffer, 2);
        
        if (file.gcount() != 2) {
            std::cerr << "Unexpected EOF reading HGT file" << std::endl;
            return false;
        }
        
        data_[i] = readBigEndianInt16(buffer);
    }
    
    std::cout << "Loaded HGT tile: " << filename 
              << " (" << originLatitude_ << ", " << originLongitude_ << ")"
              << std::endl;
    
    return true;
}

int16_t HGTLoader::readBigEndianInt16(const uint8_t* ptr) {
    return (static_cast<int16_t>(ptr[0]) << 8) | ptr[1];
}

int16_t HGTLoader::getElevation(double latitude, double longitude) const {
    int row, col;
    if (!latLonToIndices(latitude, longitude, row, col)) {
        return -32768;  // Invalid
    }
    return data_[row * 1201 + col];
}

double HGTLoader::interpolateElevation(double latitude, double longitude) const {
    // Get grid cell
    double fracLat = (latitude - originLatitude_) * 1200.0;
    double fracLon = (longitude - originLongitude_) * 1200.0;
    
    int row0 = static_cast<int>(fracLat);
    int col0 = static_cast<int>(fracLon);
    
    if (row0 < 0 || row0 >= 1200 || col0 < 0 || col0 >= 1200) {
        return -32768.0;
    }
    
    int row1 = row0 + 1;
    int col1 = col0 + 1;
    
    double u = fracLat - row0;  // [0, 1]
    double v = fracLon - col0;  // [0, 1]
    
    // Get 4 corner elevations
    double e00 = data_[row0 * 1201 + col0];
    double e01 = data_[row0 * 1201 + col1];
    double e10 = data_[row1 * 1201 + col0];
    double e11 = data_[row1 * 1201 + col1];
    
    // Bilinear interpolation
    double e0 = e00 * (1 - v) + e01 * v;
    double e1 = e10 * (1 - v) + e11 * v;
    return e0 * (1 - u) + e1 * u;
}

void HGTLoader::getBounds(double& minLat, double& maxLat,
                          double& minLon, double& maxLon) const {
    minLat = originLatitude_;
    maxLat = originLatitude_ + 1.0;
    minLon = originLongitude_;
    maxLon = originLongitude_ + 1.0;
}

bool HGTLoader::latLonToIndices(double lat, double lon,
                               int& row, int& col) const {
    double fracLat = (lat - originLatitude_) * 1200.0;
    double fracLon = (lon - originLongitude_) * 1200.0;
    
    row = static_cast<int>(fracLat);
    col = static_cast<int>(fracLon);
    
    return (row >= 0 && row < 1200 && col >= 0 && col < 1200);
}

// ============================================================================
// TerrainTileCache Implementation
// ============================================================================

TerrainTileCache::TerrainTileCache(size_t maxTiles)
    : maxTiles_(maxTiles), dataDir_(".") {}

std::string TerrainTileCache::getTileKey(double lat, double lon) const {
    int latBase = static_cast<int>(std::floor(lat));
    int lonBase = static_cast<int>(std::floor(lon));
    
    std::ostringstream oss;
    oss << (lat >= 0 ? 'N' : 'S')
        << std::setfill('0') << std::setw(2) << std::abs(latBase)
        << (lon >= 0 ? 'E' : 'W')
        << std::setfill('0') << std::setw(3) << std::abs(lonBase);
    
    return oss.str();
}

std::string TerrainTileCache::getTilePath(double lat, double lon) const {
    std::string key = getTileKey(lat, lon);
    std::ostringstream oss;
    oss << dataDir_ << "/" << key << ".hgt";
    return oss.str();
}

std::shared_ptr<HGTLoader> TerrainTileCache::getTile(double latitude, 
                                                      double longitude) {
    std::string key = getTileKey(latitude, longitude);
    
    // Check cache
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }
    
    // Remove oldest tile if cache is full
    if (cache_.size() >= maxTiles_) {
        std::string oldKey = loadOrder_.front();
        loadOrder_.pop();
        cache_.erase(oldKey);
    }
    
    // Load tile from disk
    auto tile = std::make_shared<HGTLoader>();
    std::string path = getTilePath(latitude, longitude);
    
    if (!tile->loadFromFile(path)) {
        std::cerr << "Failed to load tile: " << path << std::endl;
        return nullptr;
    }
    
    cache_[key] = tile;
    loadOrder_.push(key);
    
    return tile;
}

double TerrainTileCache::getElevation(double latitude, double longitude) {
    auto tile = getTile(latitude, longitude);
    if (!tile) {
        return 0.0;  // Sea level default
    }
    return tile->interpolateElevation(latitude, longitude);
}

void TerrainTileCache::preloadTiles(double latitude, double longitude, 
                                    double radiusDegrees) {
    int minLat = static_cast<int>(std::floor(latitude - radiusDegrees));
    int maxLat = static_cast<int>(std::ceil(latitude + radiusDegrees));
    int minLon = static_cast<int>(std::floor(longitude - radiusDegrees));
    int maxLon = static_cast<int>(std::ceil(longitude + radiusDegrees));
    
    for (int lat = minLat; lat <= maxLat; ++lat) {
        for (int lon = minLon; lon <= maxLon; ++lon) {
            getTile(lat + 0.5, lon + 0.5);  // Load tile
        }
    }
}

} // namespace Terrain
} // namespace AICopilot
```

---

## WEATHER METAR PARSER

### METAR Format Example

```
KJFK 121851Z 31008KT 10SM FEW250 M04/M17 A3034 RMK AO2 SLP279 T10441172
```

### Header

```cpp
// File: aicopilot/include/weather/metar_parser.h
#ifndef METAR_PARSER_H
#define METAR_PARSER_H

#include <string>
#include <vector>
#include <optional>
#include "../aicopilot_types.h"

namespace AICopilot {
namespace Weather {

struct METARData {
    std::string stationID;
    
    // Day/Time of observation (UTC)
    int day;        // Day of month (01-31)
    int hour;       // Hour (00-23)
    int minute;     // Minute (00-59)
    
    // Wind
    std::optional<int> windDirection;   // degrees (0-360)
    std::optional<int> windSpeed;       // knots
    std::optional<int> windGust;        // knots
    bool windVariablePrecision = false;
    
    // Visibility
    std::optional<double> visibility;   // statute miles
    std::vector<std::string> runwayVisualRange;  // RVR
    
    // Weather phenomena
    std::vector<std::string> weather;   // RA, SN, BR, etc.
    
    // Clouds
    struct CloudLayer {
        std::string coverage;   // SKC, CLR, FEW, SCT, BKN, OVC
        std::optional<int> altitude;  // feet AGL
    };
    std::vector<CloudLayer> clouds;
    
    // Temperature & Dew Point
    std::optional<double> temperature;   // Celsius
    std::optional<double> dewpoint;      // Celsius
    
    // Altimeter
    std::optional<double> altimeter;     // inHg
    
    // Remarks section
    std::string remarks;
    std::optional<double> sealevelPressure;  // hPa (from RMK)
};

class METARParser {
public:
    /**
     * Parse METAR string
     * @param rawMetar Complete METAR report
     * @return Parsed data or error
     */
    static std::optional<METARData> parse(const std::string& rawMetar);
    
private:
    static bool parseWind(const std::string& token, METARData& data);
    static bool parseVisibility(const std::string& token, METARData& data);
    static bool parseWeather(const std::string& token, METARData& data);
    static bool parseClouds(const std::string& token, METARData& data);
    static bool parseTemperature(const std::string& token, METARData& data);
    static bool parseAltimeter(const std::string& token, METARData& data);
    
    static std::vector<std::string> tokenize(const std::string& metar);
    static double parseTemperatureValue(const std::string& tempStr);
    static double parseVisibilityValue(const std::string& visStr);
};

} // namespace Weather
} // namespace AICopilot

#endif // METAR_PARSER_H
```

### Implementation

```cpp
// File: aicopilot/src/weather/metar_parser.cpp
#include "metar_parser.h"
#include <sstream>
#include <regex>
#include <cstdlib>
#include <iostream>

namespace AICopilot {
namespace Weather {

std::vector<std::string> METARParser::tokenize(const std::string& metar) {
    std::istringstream iss(metar);
    std::vector<std::string> tokens;
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

double METARParser::parseTemperatureValue(const std::string& tempStr) {
    if (tempStr.empty()) return 0.0;
    
    // Format: M04 (negative 4°C) or 15 (positive 15°C)
    int sign = 1;
    std::string numStr = tempStr;
    
    if (numStr[0] == 'M') {
        sign = -1;
        numStr = numStr.substr(1);
    }
    
    try {
        return sign * std::stod(numStr);
    } catch (...) {
        return 0.0;
    }
}

bool METARParser::parseWind(const std::string& token, METARData& data) {
    // Format: 31008KT or 310V350 (variable)
    if (token.length() < 5 || token.find("KT") == std::string::npos) {
        return false;
    }
    
    std::string windPart = token.substr(0, token.length() - 2);
    
    // Check for variable wind (e.g., 310V350)
    size_t vPos = windPart.find('V');
    if (vPos != std::string::npos) {
        windPart = windPart.substr(0, vPos);  // Use first direction
    }
    
    try {
        int direction = std::stoi(windPart.substr(0, 3));
        int speed = std::stoi(windPart.substr(3, 2));
        
        data.windDirection = direction;
        data.windSpeed = speed;
        
        // Check for gust (e.g., 31008G15KT)
        size_t gPos = token.find('G');
        if (gPos != std::string::npos) {
            int gustEnd = token.find("KT");
            std::string gustStr = token.substr(gPos + 1, gustEnd - gPos - 1);
            data.windGust = std::stoi(gustStr);
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool METARParser::parseVisibility(const std::string& token, METARData& data) {
    // Format: 10SM, 1 1/4SM, 5SM, etc.
    if (token.find("SM") == std::string::npos) {
        return false;
    }
    
    std::string visStr = token.substr(0, token.length() - 2);
    
    // Handle fractions (e.g., "1/4")
    size_t slashPos = visStr.find('/');
    if (slashPos != std::string::npos) {
        try {
            double numerator = std::stod(visStr.substr(0, slashPos));
            double denominator = std::stod(visStr.substr(slashPos + 1));
            data.visibility = numerator / denominator;
            return true;
        } catch (...) {
            return false;
        }
    }
    
    try {
        data.visibility = std::stod(visStr);
        return true;
    } catch (...) {
        return false;
    }
}

bool METARParser::parseTemperature(const std::string& token, METARData& data) {
    // Format: M04/M17 or 15/10
    if (token.find('/') == std::string::npos || token.length() < 5) {
        return false;
    }
    
    size_t slashPos = token.find('/');
    std::string tempStr = token.substr(0, slashPos);
    std::string dpStr = token.substr(slashPos + 1);
    
    data.temperature = parseTemperatureValue(tempStr);
    data.dewpoint = parseTemperatureValue(dpStr);
    
    return true;
}

bool METARParser::parseAltimeter(const std::string& token, METARData& data) {
    // Format: A3034 (30.34 inHg)
    if (token.length() < 5 || token[0] != 'A') {
        return false;
    }
    
    try {
        int altimeterInt = std::stoi(token.substr(1));
        data.altimeter = altimeterInt / 100.0;
        return true;
    } catch (...) {
        return false;
    }
}

bool METARParser::parseClouds(const std::string& token, METARData& data) {
    // Format: FEW250, BKN100, OVC050
    if (token.length() < 3) return false;
    
    std::string coverage = token.substr(0, 3);
    if (coverage == "SKC" || coverage == "CLR") {
        data.clouds.push_back({coverage, std::nullopt});
        return true;
    }
    
    if (coverage != "FEW" && coverage != "SCT" && coverage != "BKN" && 
        coverage != "OVC") {
        return false;
    }
    
    try {
        int altitude = std::stoi(token.substr(3)) * 100;  // Convert to feet
        data.clouds.push_back({coverage, altitude});
        return true;
    } catch (...) {
        return false;
    }
}

bool METARParser::parseWeather(const std::string& token, METARData& data) {
    // Format: RA, -RA, +TSRA, BR, FG, etc.
    if (token.empty()) return false;
    
    // Weather codes are usually 2-5 characters
    static const std::vector<std::string> weatherCodes = {
        "RA", "SN", "SG", "IC", "PL", "UP",      // Precipitation
        "BR", "FG", "FU", "VA", "SA", "PY",      // Obscuration
        "VC", "RE",                               // Vicinity
        "TS", "CB"                                // Convection
    };
    
    for (const auto& code : weatherCodes) {
        if (token.find(code) != std::string::npos) {
            data.weather.push_back(token);
            return true;
        }
    }
    
    return false;
}

std::optional<METARData> METARParser::parse(const std::string& rawMetar) {
    METARData data;
    auto tokens = tokenize(rawMetar);
    
    if (tokens.empty()) {
        return std::nullopt;
    }
    
    // First token is station ID
    data.stationID = tokens[0];
    
    // Second token is date/time
    if (tokens.size() > 1 && tokens[1].length() >= 7 && tokens[1][4] == 'Z') {
        std::string dateTime = tokens[1];
        try {
            data.day = std::stoi(dateTime.substr(0, 2));
            data.hour = std::stoi(dateTime.substr(2, 2));
            data.minute = std::stoi(dateTime.substr(4, 2));
        } catch (...) {
            // Invalid format
        }
    }
    
    // Parse remaining tokens
    for (size_t i = 2; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        
        if (token == "RMK") {
            // Remarks section starts
            data.remarks = "";
            for (size_t j = i + 1; j < tokens.size(); ++j) {
                if (!data.remarks.empty()) data.remarks += " ";
                data.remarks += tokens[j];
            }
            break;
        }
        
        // Try to parse token
        parseWind(token, data) ||
        parseVisibility(token, data) ||
        parseWeather(token, data) ||
        parseClouds(token, data) ||
        parseTemperature(token, data) ||
        parseAltimeter(token, data);
    }
    
    return data;
}

} // namespace Weather
} // namespace AICopilot
```

---

## NAVIGATION FAX PARSER

### FAA NavAID Format

```
Type,Ident,Name,City,State,Lat,Lon,Elev,Freq,Range,Slaved,Type(Extra)
VOR,12345,CHICAGO,ORD,IL,41-58-39.0N,087-57-27.0W,1550,113.00,200,N,STANDARD
NDB,54321,MIDWAY,MDW,IL,41-46-34.0N,087-44-27.0W,620,385,25,N,LO
```

### Header

```cpp
// File: aicopilot/include/navdata/fax_parser.h
#ifndef FAX_PARSER_H
#define FAX_PARSER_H

#include <string>
#include <vector>
#include <optional>
#include "../navdata_provider.h"

namespace AICopilot {
namespace NavData {

class FAXParser {
public:
    /**
     * Parse FAA NavAID database file
     * @param filePath Path to .txt file
     * @return Vector of parsed navaids
     */
    static std::vector<NavaidInfo> parseFile(const std::string& filePath);
    
    /**
     * Parse single line from FAA format
     */
    static std::optional<NavaidInfo> parseLine(const std::string& line);
    
private:
    static double parseCoordinate(const std::string& coordStr);
    static std::vector<std::string> csvParse(const std::string& line);
};

} // namespace NavData
} // namespace AICopilot

#endif // FAX_PARSER_H
```

### Implementation

```cpp
// File: aicopilot/src/navdata/fax_parser.cpp
#include "fax_parser.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>

namespace AICopilot {
namespace NavData {

std::vector<std::string> FAXParser::csvParse(const std::string& line) {
    std::vector<std::string> result;
    std::istringstream iss(line);
    std::string field;
    
    while (std::getline(iss, field, ',')) {
        // Trim whitespace
        field.erase(0, field.find_first_not_of(" \t"));
        field.erase(field.find_last_not_of(" \t") + 1);
        result.push_back(field);
    }
    
    return result;
}

double FAXParser::parseCoordinate(const std::string& coordStr) {
    // Format: 41-58-39.0N or 087-57-27.0W
    // Returns: 41.9775 or -87.9575
    
    std::vector<std::string> parts;
    std::istringstream iss(coordStr);
    std::string part;
    
    while (std::getline(iss, part, '-')) {
        parts.push_back(part);
    }
    
    if (parts.size() < 3) return 0.0;
    
    double degrees = std::stod(parts[0]);
    double minutes = std::stod(parts[1]);
    double seconds = std::stod(parts[2]);
    
    // Remove direction letter from seconds
    char direction = 0;
    std::string secondsStr = parts[2];
    if (!secondsStr.empty()) {
        direction = secondsStr.back();
        if (direction == 'N' || direction == 'S' || 
            direction == 'E' || direction == 'W') {
            secondsStr.pop_back();
            seconds = std::stod(secondsStr);
        }
    }
    
    double result = degrees + minutes/60.0 + seconds/3600.0;
    
    if (direction == 'S' || direction == 'W') {
        result = -result;
    }
    
    return result;
}

std::optional<NavaidInfo> FAXParser::parseLine(const std::string& line) {
    if (line.empty() || line[0] == '#') {
        return std::nullopt;  // Skip comments
    }
    
    auto fields = csvParse(line);
    
    // Minimum fields required
    if (fields.size() < 8) {
        return std::nullopt;
    }
    
    NavaidInfo info;
    
    try {
        info.type = fields[0];          // VOR, NDB, DME, etc.
        
        // Ident might be numeric, store as string
        info.id = fields[1];
        info.name = fields[2];          // Name
        // fields[3] = City (unused)
        // fields[4] = State (unused)
        
        info.position.latitude = parseCoordinate(fields[5]);
        info.position.longitude = parseCoordinate(fields[6]);
        
        info.position.altitude = std::stod(fields[7]);  // Elevation in feet
        
        // Frequency
        if (fields.size() > 8) {
            info.frequency = std::stod(fields[8]);
        }
        
        // Range in nautical miles
        if (fields.size() > 9) {
            info.range = std::stod(fields[9]);
        }
        
        // Magnetic variation (can be calculated separately)
        info.magneticVariation = 0.0;  // TODO: Calculate using GeographicLib
        
        return info;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing line: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<NavaidInfo> FAXParser::parseFile(const std::string& filePath) {
    std::vector<NavaidInfo> navaids;
    std::ifstream file(filePath);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open FAX file: " << filePath << std::endl;
        return navaids;
    }
    
    std::string line;
    int lineNum = 0;
    int skipped = 0;
    
    while (std::getline(file, line)) {
        ++lineNum;
        
        auto result = parseLine(line);
        if (result) {
            navaids.push_back(result.value());
        } else {
            ++skipped;
        }
    }
    
    std::cout << "Loaded " << navaids.size() << " navaids from " 
              << filePath << " (skipped " << skipped << " lines)" << std::endl;
    
    return navaids;
}

} // namespace NavData
} // namespace AICopilot
```

---

## DATABASE CACHING ARCHITECTURE

```cpp
// File: aicopilot/include/database/database_cache.h
#ifndef DATABASE_CACHE_H
#define DATABASE_CACHE_H

#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

namespace AICopilot {
namespace Database {

template<typename TKey, typename TValue>
class LRUCache {
public:
    LRUCache(size_t maxSize) : maxSize_(maxSize) {}
    
    /**
     * Get value from cache
     */
    bool get(const TKey& key, TValue& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }
    
    /**
     * Put value in cache
     */
    void put(const TKey& key, const TValue& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (cache_.size() >= maxSize_) {
            // Remove least recently used
            auto oldest = lruOrder_.front();
            lruOrder_.pop();
            cache_.erase(oldest);
        }
        
        cache_[key] = value;
        lruOrder_.push(key);
    }
    
    /**
     * Clear cache
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        while (!lruOrder_.empty()) {
            lruOrder_.pop();
        }
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }
    
private:
    size_t maxSize_;
    std::map<TKey, TValue> cache_;
    std::queue<TKey> lruOrder_;
    mutable std::mutex mutex_;
};

/**
 * Async tile loader for terrain/weather data
 */
class AsyncTileLoader {
public:
    using TileLoadCallback = std::function<void(const std::string& key, bool success)>;
    
    AsyncTileLoader(size_t numThreads = 2);
    ~AsyncTileLoader();
    
    /**
     * Queue tile for async loading
     */
    void queueTile(const std::string& key, const std::string& filePath,
                   TileLoadCallback callback);
    
    /**
     * Shutdown loader threads
     */
    void shutdown();
    
private:
    struct TileLoadRequest {
        std::string key;
        std::string filePath;
        TileLoadCallback callback;
    };
    
    std::vector<std::thread> threads_;
    std::queue<TileLoadRequest> queue_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    bool shutdownFlag_ = false;
    
    void workerThread();
};

} // namespace Database
} // namespace AICopilot

#endif // DATABASE_CACHE_H
```

---

## INTEGRATION CODE EXAMPLES

### Example 1: Integrating HGT Loader with TerrainAwareness

```cpp
// File: aicopilot/src/terrain/terrain_awareness_enhanced.cpp

#include "terrain_awareness.h"
#include "terrain/hgt_loader.h"
#include <iostream>

namespace AICopilot {

class TerrainAwarenessImpl {
private:
    std::unique_ptr<Terrain::TerrainTileCache> elevationCache_;
    
public:
    TerrainAwarenessImpl() 
        : elevationCache_(std::make_unique<Terrain::TerrainTileCache>(16)) {
        // Initialize with data directory
        elevationCache_->setDataDirectory("./data/srtm");
    }
    
    void initialize(const std::string& srtmDataPath) {
        elevationCache_->setDataDirectory(srtmDataPath);
        
        // Preload tiles around common flight areas
        std::vector<std::pair<double, double>> commonAreas = {
            {40.6398, -73.7789},   // KJFK
            {33.9425, -118.4081},  // KLAX
            {41.9786, -87.9048},   // KORD
        };
        
        for (const auto& pos : commonAreas) {
            elevationCache_->preloadTiles(pos.first, pos.second, 2.0);
        }
    }
};

// Enhanced getTerrainElevation using real data
double TerrainAwareness::getTerrainElevation(const Position& pos) const {
    if (!elevationCache_) {
        return 0.0;  // Fallback
    }
    
    // Get elevation from cache (loads tiles as needed)
    return elevationCache_->getElevation(pos.latitude, pos.longitude);
}

} // namespace AICopilot
```

### Example 2: Integrating METAR with WeatherSystem

```cpp
// File: aicopilot/src/weather/weather_system_enhanced.cpp

#include "weather_system.h"
#include "weather/metar_parser.h"
#include <curl/curl.h>

namespace AICopilot {

class WeatherSystemImpl {
private:
    std::map<std::string, Weather::METARData> metarCache_;
    std::chrono::time_point<std::chrono::system_clock> lastUpdate_;
    
    // CURL callback for HTTP responses
    static size_t writeCallback(void* contents, size_t size, 
                               size_t nmemb, std::string* buffer) {
        buffer->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
public:
    bool fetchMETARForAirport(const std::string& icao, 
                             Weather::METARData& outData) {
        // Check cache first
        auto it = metarCache_.find(icao);
        if (it != metarCache_.end()) {
            outData = it->second;
            return true;
        }
        
        // Fetch from Aviation Weather Center API
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        std::string url = "https://aviationweather.gov/adds/dataserver?"
                         "request=retrieve&dataSource=metars&"
                         "stationString=" + icao + 
                         "&hoursBeforeNow=3&format=xml";
        
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            return false;
        }
        
        // Parse XML response to extract METAR string
        // (Simplified - use XML library in real implementation)
        size_t startPos = response.find("<raw_text>");
        if (startPos == std::string::npos) {
            return false;
        }
        
        startPos += 10;
        size_t endPos = response.find("</raw_text>", startPos);
        std::string metarString = response.substr(startPos, endPos - startPos);
        
        // Parse METAR
        auto parsed = Weather::METARParser::parse(metarString);
        if (!parsed) {
            return false;
        }
        
        outData = parsed.value();
        metarCache_[icao] = outData;
        
        return true;
    }
};

} // namespace AICopilot
```

---

**Document Complete**

For implementation details, refer to the main analysis document:
`DATABASE_INTEGRATION_ANALYSIS.md`

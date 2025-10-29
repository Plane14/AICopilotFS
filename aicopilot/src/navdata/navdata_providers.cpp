/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "navdata_provider.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef USE_MSFS2024_SDK
#include <windows.h>
#include <SimConnect.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

struct DefaultAirportRecord {
    const char* icao;
    const char* name;
    double latitude;
    double longitude;
    double elevation;
    int longestRunway;
    bool towered;
    std::initializer_list<const char*> runwayIdents;
};

struct DefaultNavaidRecord {
    const char* id;
    const char* name;
    const char* type;
    double latitude;
    double longitude;
    double frequency;
    double range;
};

const std::vector<DefaultAirportRecord>& defaultAirportDataset() {
    static const std::vector<DefaultAirportRecord> records = {
        {"KJFK", "John F Kennedy Intl", 40.6398, -73.7789, 13.0, 14511, true,
         {"04L", "04R", "13L", "13R", "22L", "22R", "31L", "31R"}},
        {"KLAX", "Los Angeles Intl", 33.9425, -118.4081, 125.0, 12923, true,
         {"06L", "06R", "24L", "24R"}},
        {"KORD", "Chicago O'Hare Intl", 41.9786, -87.9048, 672.0, 13000, true,
         {"09L", "09R", "10C", "10L", "10R", "27L", "27R", "28C", "28R"}},
        {"KATL", "Hartsfield Jackson Atlanta Intl", 33.6367, -84.4281, 1026.0, 12390, true,
         {"08L", "08R", "09L", "09R", "26L", "26R", "27L", "27R"}},
        {"KDFW", "Dallas Fort Worth Intl", 32.8968, -97.0380, 607.0, 13401, true,
         {"13L", "13R", "17L", "17R", "18L", "18R", "31L", "31R", "35L", "35R", "36L", "36R"}},
        {"KDEN", "Denver Intl", 39.8617, -104.6731, 5434.0, 16000, true,
         {"07", "08", "16L", "16R", "17L", "17R", "25", "26", "34L", "34R", "35L", "35R"}},
        {"KSFO", "San Francisco Intl", 37.6213, -122.3790, 13.0, 11870, true,
         {"01L", "01R", "10L", "10R", "19L", "19R", "28L", "28R"}},
        {"KLAS", "Las Vegas McCarran Intl", 36.0840, -115.1537, 2181.0, 14510, true,
         {"01L", "01R", "08L", "08R", "19L", "19R", "26L", "26R"}},
        {"KMIA", "Miami Intl", 25.7932, -80.2906, 8.0, 13016, true,
         {"08L", "08R", "09", "12", "26L", "26R", "27", "30"}},
        {"KSEA", "Seattle Tacoma Intl", 47.4502, -122.3088, 433.0, 11901, true,
         {"16L", "16C", "16R", "34L", "34C", "34R"}},
        {"KBOS", "Boston Logan Intl", 42.3656, -71.0096, 20.0, 10083, true,
         {"04L", "04R", "09", "14", "22L", "22R", "27", "32"}},
        {"KEWR", "Newark Liberty Intl", 40.6925, -74.1687, 18.0, 11000, true,
         {"04L", "04R", "11", "22L", "22R", "29"}},
        {"KMCO", "Orlando Intl", 28.4294, -81.3089, 96.0, 12005, true,
         {"17L", "17R", "18L", "18R", "35L", "35R", "36L", "36R"}},
        {"KPHX", "Phoenix Sky Harbor Intl", 33.4343, -112.0116, 1135.0, 11489, true,
         {"07L", "07R", "08", "08L", "08R", "25L", "25R", "26"}},
        {"KIAH", "Houston George Bush Intl", 29.9844, -95.3414, 97.0, 12001, true,
         {"08L", "08R", "09", "10", "26L", "26R", "27", "28"}},
        {"KVNY", "Van Nuys Airport", 34.2098, -118.4898, 802.0, 8001, true,
         {"16L", "16R", "34L", "34R"}},
        {"KSNA", "John Wayne Orange County", 33.6757, -117.8682, 56.0, 5701, true,
         {"02L", "02R", "20L", "20R"}},
        {"KSAN", "San Diego Intl", 32.7336, -117.1897, 17.0, 9401, true,
         {"09", "27"}},
        {"KPDX", "Portland Intl", 45.5887, -122.5975, 31.0, 11000, true,
         {"10L", "10R", "28L", "28R"}},
        {"KBWI", "Baltimore Washington Intl", 39.1754, -76.6683, 146.0, 10502, true,
         {"10", "15L", "15R", "28", "33L", "33R"}},
        {"EGLL", "London Heathrow", 51.4700, -0.4543, 83.0, 12799, true,
         {"09L", "09R", "27L", "27R"}},
        {"LFPG", "Paris Charles de Gaulle", 49.0097, 2.5479, 392.0, 13829, true,
         {"08L", "08R", "09L", "09R", "26L", "26R", "27L", "27R"}},
        {"EDDF", "Frankfurt Main", 50.0379, 8.5622, 364.0, 13123, true,
         {"07C", "07L", "07R", "18", "25C", "25L", "25R"}},
        {"LEMD", "Madrid Barajas", 40.4719, -3.5626, 1998.0, 14268, true,
         {"14L", "14R", "18L", "18R", "32L", "32R", "36L", "36R"}},
        {"LIRF", "Rome Fiumicino", 41.8003, 12.2389, 13.0, 12795, true,
         {"07", "16L", "16R", "25", "34L", "34R"}}
    };
    return records;
}

const std::vector<DefaultNavaidRecord>& defaultNavaidDataset() {
    static const std::vector<DefaultNavaidRecord> records = {
        {"LAX", "Los Angeles VOR/DME", "VOR", 33.9297, -118.4156, 113.60, 200.0},
        {"SFO", "San Francisco VOR/DME", "VOR", 37.6197, -122.3728, 115.80, 200.0},
        {"DEN", "Denver VOR/DME", "VOR", 39.8561, -104.6567, 117.90, 200.0},
        {"ORD", "Chicago O'Hare VOR/DME", "VOR", 41.9950, -87.9339, 113.90, 200.0},
        {"JFK", "Kennedy VOR/DME", "VOR", 40.6394, -73.7789, 115.90, 200.0}
    };
    return records;
}

std::string trim(const std::string& value) {
    size_t start = 0;
    size_t end = value.size();
    while (start < end && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool parseBooleanToken(const std::string& token) {
    const std::string lowered = toLower(trim(token));
    return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "y" || lowered == "t";
}

std::vector<std::string> splitRunwayList(const std::string& field) {
    std::vector<std::string> result;
    std::string current;
    for (char ch : field) {
        if (ch == ';' || ch == '|' || ch == '/') {
            const std::string token = trim(current);
            if (!token.empty()) {
                result.push_back(token);
            }
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    const std::string token = trim(current);
    if (!token.empty()) {
        result.push_back(token);
    }

    return result;
}

std::vector<std::string> parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                current.push_back('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            fields.push_back(trim(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    fields.push_back(trim(current));
    return fields;
}

std::pair<int, char> parseRunwayIdent(const std::string& ident) {
    int number = 0;
    size_t idx = 0;
    while (idx < ident.size() && std::isdigit(static_cast<unsigned char>(ident[idx]))) {
        number = number * 10 + (ident[idx] - '0');
        ++idx;
    }
    if (number == 0) {
        return {0, '\0'};
    }
    if (number % 360 == 0 && number > 36) {
        number %= 360;
    }
    if (number > 36) {
        number %= 36;
    }
    if (number == 0) {
        number = 36;
    }
    char side = '\0';
    if (idx < ident.size()) {
        side = static_cast<char>(std::toupper(static_cast<unsigned char>(ident[idx])));
    }
    return {number, side};
}

double headingFromRunwayIdent(const std::string& ident) {
    const auto parsed = parseRunwayIdent(ident);
    if (parsed.first == 0) {
        return 0.0;
    }
    double heading = parsed.first * 10.0;
    if (heading >= 360.0) {
        heading -= 360.0;
    }
    switch (parsed.second) {
        case 'L': heading -= 2.5; break;
        case 'R': heading += 2.5; break;
        default: break;
    }
    if (heading < 0.0) {
        heading += 360.0;
    }
    return heading;
}

void populateDefaultAirports(std::map<std::string, AirportInfo>& cache) {
    for (const auto& record : defaultAirportDataset()) {
        if (cache.find(record.icao) != cache.end()) {
            continue;
        }
        AirportInfo info;
        info.icao = record.icao;
        info.iata.clear();
        info.name = record.name;
        info.position.latitude = record.latitude;
        info.position.longitude = record.longitude;
        info.position.altitude = record.elevation;
        info.elevation = record.elevation;
        info.longestRunway = record.longestRunway;
        info.towered = record.towered;
        info.runways.assign(record.runwayIdents.begin(), record.runwayIdents.end());
        cache.emplace(info.icao, std::move(info));
    }
}

void populateDefaultNavaids(std::map<std::string, NavaidInfo>& cache) {
    for (const auto& record : defaultNavaidDataset()) {
        if (cache.find(record.id) != cache.end()) {
            continue;
        }
        NavaidInfo info;
        info.id = record.id;
        info.name = record.name;
        info.type = record.type;
        info.position.latitude = record.latitude;
        info.position.longitude = record.longitude;
        info.frequency = record.frequency;
        info.range = record.range;
        info.magneticVariation = 0.0;
        cache.emplace(info.id, std::move(info));
    }
}

AirportLayout buildLayoutFromAirport(const AirportInfo& info) {
    AirportLayout layout;

    const double length = info.longestRunway > 0 ? static_cast<double>(info.longestRunway) : 6000.0;
    const double width = std::clamp(length / 50.0, 75.0, 200.0);

    Airport::LatLonAlt ref(info.position.latitude, info.position.longitude, info.position.altitude);
    std::set<std::string> processed;
    auto addRunway = [&](const std::string& ident) {
        const std::string trimmed = trim(ident);
        if (trimmed.empty() || !processed.insert(trimmed).second) {
            return;
        }

        Airport::Runway runway;
        runway.runway_ident = trimmed;
        runway.runway_number = parseRunwayIdent(trimmed).first;
        runway.heading_true = headingFromRunwayIdent(trimmed);
        runway.length_feet = length;
        runway.width_feet = width;
        runway.threshold_lat_lon = ref;
        runway.surface_type = Airport::Runway::Surface::Asphalt;
        runway.lighting_type = info.towered ? Airport::Runway::Lighting::FullHighIntensity
                                            : Airport::Runway::Lighting::PartialMediumIntensity;
        runway.has_ils = true;
        runway.has_glideslope = true;
        runway.displaced_threshold_feet = 0;
        runway.is_active = true;
        layout.runways.push_back(runway);
    };

    if (!info.runways.empty()) {
        for (const auto& ident : info.runways) {
            addRunway(ident);
        }
    } else {
        addRunway("09");
        addRunway("27");
    }

    Airport::TaxiwayNetwork network;
    Airport::TaxiwayNode runwayNode(1, ref);
    runwayNode.name = "Runway Hold";
    runwayNode.type = Airport::TaxiwayNode::NodeType::RunwayHold;
    network.add_node(runwayNode);

    Airport::TaxiwayNode apronNode(2, ref);
    apronNode.name = "Apron";
    apronNode.type = Airport::TaxiwayNode::NodeType::ParkingArea;
    network.add_node(apronNode);

    Airport::TaxiwayEdge edge(1, runwayNode.node_id, apronNode.node_id, length / 4.0);
    edge.surface = Airport::TaxiwayEdge::Surface::Concrete;
    edge.max_speed_knots = 15.0;
    network.add_edge(edge);

    layout.taxiwayNetwork = network;

    Airport::ParkingPosition parking(1, ref);
    parking.type = Airport::ParkingPosition::Type::Gate;
    parking.gate_name = "G1";
    parking.has_jetway = info.towered;
    parking.has_fueling = true;
    layout.parkingPositions.push_back(parking);

    return layout;
}

} // namespace

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
    bool defaultsLoaded = false;
    
    // Cache for facility data
    std::map<std::string, AirportInfo> airportCache;
    std::map<std::string, NavaidInfo> navaidCache;

    void ensureDefaultData() {
        if (!defaultsLoaded) {
            populateDefaultAirports(airportCache);
            populateDefaultNavaids(navaidCache);
            defaultsLoaded = true;
        }
    }
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
        } else {
            std::cout << "SimConnectNavdataProvider: Failed to connect to SimConnect (code: " 
                     << hr << ")" << std::endl;
        }
    } else {
        // Using external handle
        pImpl->ready = true;
        std::cout << "SimConnectNavdataProvider: Using external SimConnect handle" << std::endl;
    }
    if (!pImpl->ready) {
        pImpl->ensureDefaultData();
        pImpl->ready = true;
        std::cout << "SimConnectNavdataProvider: Falling back to cached navigation dataset" << std::endl;
    } else {
        pImpl->ensureDefaultData();
    }
    return true;
#else
    pImpl->ensureDefaultData();
    pImpl->ready = true;
    std::cout << "SimConnectNavdataProvider: Using cached navigation dataset (SimConnect disabled)" << std::endl;
    return true;
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
    pImpl->defaultsLoaded = false;
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
    pImpl->ensureDefaultData();

    std::string normalized = icao;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    
    // Check cache first
    auto it = pImpl->airportCache.find(normalized);
    if (it != pImpl->airportCache.end()) {
        info = it->second;
        return true;
    }
    
#ifdef USE_MSFS2024_SDK
    if (pImpl->hSimConnect) {
        std::cout << "SimConnectNavdataProvider::getAirportByICAO(" << normalized
                  << ") - not cached, SimConnect facility lookup not yet implemented" << std::endl;
    }
#endif
    
    return false;
}

std::vector<AirportInfo> SimConnectNavdataProvider::getAirportsNearby(const Position& center, double radiusNM) {
    std::vector<AirportInfo> result;
    
    if (!pImpl->ready) {
        return result;
    }
    pImpl->ensureDefaultData();
    
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
    pImpl->ensureDefaultData();
    
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
    pImpl->ensureDefaultData();
    
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
    pImpl->ensureDefaultData();
    
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
    pImpl->ensureDefaultData();
    std::string normalized = icao;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    
    auto it = pImpl->airportCache.find(normalized);
    if (it == pImpl->airportCache.end()) {
        return false;
    }

    layout = buildLayoutFromAirport(it->second);
    return true;
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
        auto before = airports.size();
        populateDefaultAirports(airports);
        std::cout << "Loaded " << (airports.size() - before) << " default airports (" << airports.size()
                  << " total)" << std::endl;
    }
    
    void loadDefaultNavaids() {
        auto before = navaids.size();
        populateDefaultNavaids(navaids);
        std::cout << "Loaded " << (navaids.size() - before) << " default navaids (" << navaids.size()
                  << " total)" << std::endl;
    }
    
    void addAirport(const std::string& icao, const std::string& name, 
                   double lat, double lon, double elev, int runway, bool tower) {
        AirportInfo info;
        std::string normalized = icao;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });
        info.icao = normalized;
        info.name = name;
        info.position.latitude = lat;
        info.position.longitude = lon;
        info.position.altitude = elev;
        info.elevation = elev;
        info.longestRunway = runway;
        info.towered = tower;
        info.runways.clear();
        airports[normalized] = info;
    }
    
    void addNavaid(const std::string& id, const std::string& name, const std::string& type,
                  double lat, double lon, double freq, double range) {
        NavaidInfo info;
        std::string normalized = id;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });
        info.id = normalized;
        info.name = name;
        info.type = type;
        info.position.latitude = lat;
        info.position.longitude = lon;
        info.frequency = freq;
        info.range = range;
        info.magneticVariation = 0.0; // Simplified
        navaids[normalized] = info;
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
    std::string normalized = icao;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    auto it = pImpl->airports.find(normalized);
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
    std::string normalized = id;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    auto it = pImpl->navaids.find(normalized);
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
    std::string normalized = icao;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    auto it = pImpl->airports.find(normalized);
    if (it == pImpl->airports.end()) {
        return false;
    }

    layout = buildLayoutFromAirport(it->second);
    return true;
}

void CachedNavdataProvider::addAirport(const AirportInfo& info) {
    pImpl->airports[info.icao] = info;
}

void CachedNavdataProvider::addNavaid(const NavaidInfo& info) {
    pImpl->navaids[info.id] = info;
}

bool CachedNavdataProvider::loadAirportsFromFile(const std::string& filePath) {
    std::cout << "CachedNavdataProvider::loadAirportsFromFile(" << filePath << ")" << std::endl;

    std::ifstream input(filePath);
    if (!input.is_open()) {
        std::cerr << "CachedNavdataProvider: Unable to open airport file " << filePath << std::endl;
        return false;
    }

    const std::string lowerPath = toLower(filePath);
    if (lowerPath.size() < 4 || lowerPath.substr(lowerPath.size() - 4) != ".csv") {
        std::cerr << "CachedNavdataProvider: Only CSV airport files are supported at this time" << std::endl;
    }

    std::string line;
    bool headerProcessed = false;
    size_t loadedCount = 0;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        auto fields = parseCsvLine(line);
        if (!headerProcessed) {
            headerProcessed = true;
            if (!fields.empty()) {
                const std::string firstHeader = toLower(fields[0]);
                if (firstHeader.find("icao") != std::string::npos) {
                    // Header row detected; read next line for data
                    continue;
                }
            }
        }

        if (fields.size() < 6) {
            std::cerr << "CachedNavdataProvider: Skipping malformed row (expected at least 6 columns)" << std::endl;
            continue;
        }

        try {
            const std::string icaoRaw = trim(fields[0]);
            if (icaoRaw.empty()) {
                continue;
            }
            std::string normalized = icaoRaw;
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });

            AirportInfo info;
            info.icao = normalized;
            info.name = trim(fields[1]);
            info.position.latitude = std::stod(fields[2]);
            info.position.longitude = std::stod(fields[3]);
            info.position.altitude = (fields.size() > 4 && !fields[4].empty()) ? std::stod(fields[4]) : 0.0;
            info.elevation = info.position.altitude;
            info.longestRunway = (fields.size() > 5 && !fields[5].empty()) ? static_cast<int>(std::stod(fields[5])) : 0;
            info.towered = (fields.size() > 6) ? parseBooleanToken(fields[6]) : true;
            if (fields.size() > 7) {
                info.runways = splitRunwayList(fields[7]);
            }
            if (info.runways.empty()) {
                info.runways = {"09", "27"};
            }

            pImpl->airports[normalized] = info;
            ++loadedCount;
        } catch (const std::exception& ex) {
            std::cerr << "CachedNavdataProvider: Skipping row due to parse error - " << ex.what() << std::endl;
        }
    }

    if (loadedCount > 0) {
        std::cout << "CachedNavdataProvider: Loaded " << loadedCount << " airport entries from file" << std::endl;
        return true;
    }

    std::cout << "CachedNavdataProvider: No airport entries were loaded from file" << std::endl;
    return false;
}

} // namespace AICopilot

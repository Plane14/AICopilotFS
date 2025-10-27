# SDK Integration Implementation Guide

## Quick Reference: How to Implement Each Feature

### Weather Integration

#### 1. Get Current Weather at Aircraft Position

```cpp
// Request interpolated weather at current position
void GetWeatherAtPosition(double latitude, double longitude, double altitude) {
    SimConnect_WeatherRequestInterpolatedObservation(hSimConnect,
        REQUEST_WEATHER_ID,
        latitude,
        longitude, 
        altitude);
}

// Handle response
void HandleWeatherResponse(SIMCONNECT_RECV_WEATHER_OBSERVATION* pData) {
    // pData->szMetar contains METAR string
    // Parse format: "KJFK 121251Z 30008KT 10SM FEW250 23/14 A3012"
    
    // Extract components:
    // - Wind: 30008KT = 300° at 8 knots
    // - Visibility: 10SM = 10 statute miles
    // - Clouds: FEW250 = few clouds at 25,000 feet
    // - Temperature: 23°C / 14°C dewpoint
    // - Altimeter: A3012 = 30.12 inHg
}
```

#### 2. Detect Icing Conditions

```cpp
struct WeatherData {
    float temperature;      // degrees Celsius
    float dewPoint;         // degrees Celsius
    float windSpeed;        // knots
    bool hasPrecipitation;  // rain/snow present
};

bool CheckIcingRisk(const WeatherData& weather) {
    // Icing occurs when:
    // 1. Temperature between 0°C and -20°C (optimal icing range)
    // 2. AND visible moisture present (clouds, precipitation)
    
    bool isInIcingTemp = (weather.temperature >= -20.0f && 
                         weather.temperature <= 0.0f);
    
    // Dew point spread < 5°C indicates saturation
    float dewPointSpread = weather.temperature - weather.dewPoint;
    bool isSaturated = (dewPointSpread < 5.0f);
    
    return isInIcingTemp && (weather.hasPrecipitation || isSaturated);
}
```

#### 3. Assess Weather Suitability for Flight

```cpp
enum WeatherRisk { SAFE = 0, MARGINAL = 1, UNSUITABLE = 2 };

WeatherRisk AssessWeatherForFlight(const WeatherData& weather,
                                   AircraftType aircraftType) {
    int riskScore = 0;
    
    // Single-engine aircraft restrictions
    if (aircraftType == SINGLE_ENGINE_PROP) {
        // Severe wind is critical
        if (weather.windSpeed > 30.0f) return UNSUITABLE;
        
        // Low ceiling/visibility
        if (weather.ceiling < 1000.0f || weather.visibility < 3.0f) 
            return UNSUITABLE;
        
        // Icing is dangerous without de-ice equipment
        if (CheckIcingRisk(weather)) 
            return UNSUITABLE;
        
        // Thunderstorms are unacceptable
        if (weather.thunderstorms) 
            return UNSUITABLE;
    }
    
    // Multi-engine aircraft can handle more
    else if (aircraftType == MULTI_ENGINE) {
        if (weather.windSpeed > 50.0f) return UNSUITABLE;
        if (weather.ceiling < 500.0f) return UNSUITABLE;
    }
    
    // Turbojet/commercial aircraft
    else if (aircraftType == COMMERCIAL) {
        // Can handle worst weather with proper training
        if (weather.windSpeed > 70.0f) return MARGINAL;
    }
    
    return (riskScore > 5) ? MARGINAL : SAFE;
}
```

---

### Autopilot Integration

#### 1. Enable Autopilot with Heading Hold

```cpp
void EnableAutopilotHeadingHold(double targetHeading) {
    // First, set the heading bug
    SimConnect_TransmitClientEvent(hSimConnect,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        EVENT_HEADING_BUG_SET,
        (DWORD)targetHeading,
        GROUP_AUTOPILOT,
        SIMCONNECT_EVENT_FLAG_DEFAULT);
    
    // Enable heading hold mode
    SimConnect_TransmitClientEvent(hSimConnect,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        EVENT_AUTOPILOT_HEADING_HOLD,
        1,  // 1 = enable
        GROUP_AUTOPILOT,
        SIMCONNECT_EVENT_FLAG_DEFAULT);
    
    // Turn on autopilot master if not already on
    SimConnect_TransmitClientEvent(hSimConnect,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        EVENT_AUTOPILOT_MASTER,
        1,  // 1 = ON
        GROUP_AUTOPILOT,
        SIMCONNECT_EVENT_FLAG_DEFAULT);
}
```

#### 2. Enable Altitude Hold

```cpp
void EnableAltitudeHold(double targetAltitude) {
    // Set target altitude
    SimConnect_TransmitClientEvent(hSimConnect,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        EVENT_ALTITUDE_BUG_SET,
        (DWORD)targetAltitude,
        GROUP_AUTOPILOT,
        SIMCONNECT_EVENT_FLAG_DEFAULT);
    
    // Enable altitude hold
    SimConnect_TransmitClientEvent(hSimConnect,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        EVENT_AUTOPILOT_ALTITUDE_HOLD,
        1,  // 1 = enable
        GROUP_AUTOPILOT,
        SIMCONNECT_EVENT_FLAG_DEFAULT);
    
    // Enable master autopilot
    SimConnect_TransmitClientEvent(hSimConnect,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        EVENT_AUTOPILOT_MASTER,
        1,
        GROUP_AUTOPILOT,
        SIMCONNECT_EVENT_FLAG_DEFAULT);
}
```

#### 3. Monitor Autopilot Status

```cpp
struct AutopilotStatus {
    bool masterEnabled;
    bool headingHoldEnabled;
    bool altitudeHoldEnabled;
    bool speedHoldEnabled;
    double targetHeading;
    double targetAltitude;
    double targetSpeed;
};

void RequestAutopilotStatus() {
    SimConnect_RequestDataOnSimObject(hSimConnect,
        REQUEST_AUTOPILOT_STATUS,
        DEFINITION_AUTOPILOT_STATE,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        SIMCONNECT_PERIOD_SIM_FRAME);
}

void HandleAutopilotData(const AutopilotStatus& status) {
    // Log status changes
    if (status.masterEnabled) {
        // Autopilot is active
        // Check what modes are enabled
        if (status.headingHoldEnabled) {
            LOG(INFO) << "Heading hold: " << status.targetHeading;
        }
        if (status.altitudeHoldEnabled) {
            LOG(INFO) << "Altitude hold: " << status.targetAltitude;
        }
    }
}
```

---

### Lighting Control

#### 1. Enable Navigation Lights

```cpp
void EnableNavigationLights(bool enable) {
    SimConnect_TransmitClientEvent(hSimConnect,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        enable ? EVENT_NAV_LIGHTS_ON : EVENT_NAV_LIGHTS_OFF,
        0,  // no data
        GROUP_LIGHTING,
        SIMCONNECT_EVENT_FLAG_DEFAULT);
}
```

#### 2. Enable Landing Lights (Takeoff/Landing)

```cpp
void EnableLandingLights(bool enable) {
    SimConnect_TransmitClientEvent(hSimConnect,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        enable ? EVENT_LANDING_LIGHTS_ON : EVENT_LANDING_LIGHTS_OFF,
        0,
        GROUP_LIGHTING,
        SIMCONNECT_EVENT_FLAG_DEFAULT);
}
```

#### 3. Control Strobe Lights

```cpp
void EnableStrobeLights(bool enable) {
    SimConnect_TransmitClientEvent(hSimConnect,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        enable ? EVENT_STROBE_LIGHTS_ON : EVENT_STROBE_LIGHTS_OFF,
        0,
        GROUP_LIGHTING,
        SIMCONNECT_EVENT_FLAG_DEFAULT);
}
```

#### 4. Set Panel Light Brightness (0-100%)

```cpp
void SetPanelLights(int brightness) {
    // Brightness: 0-100 scale
    SimConnect_TransmitClientEvent(hSimConnect,
        SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
        EVENT_PANEL_LIGHT_SET,
        brightness,
        GROUP_LIGHTING,
        SIMCONNECT_EVENT_FLAG_DEFAULT);
}
```

#### 5. Integrated Lighting Sequence for Departure

```cpp
void PerformDepartureIgnitionSequence() {
    // Daylight departure
    if (isDaylight) {
        EnableNavigationLights(true);   // Standard day operation
        EnableStrobeLights(true);        // Strobe for visibility
    }
    
    // Night departure
    else {
        EnableNavigationLights(true);
        EnableStrobeLights(true);
        SetPanelLights(50);              // Medium panel brightness
        EnableLandingLights(true);       // Landing lights for takeoff
    }
    
    // Once airborne at 1000 ft
    if (altitude > 1000.0) {
        if (!isDaylight) {
            EnableLandingLights(false);   // Retract/turn off landing lights
        }
    }
}
```

---

### ATC Integration

#### 1. Get Available ATC Menu at Current Airport

```cpp
struct ATCOption {
    int id;
    std::string text;
    float confidence;  // AI scoring
};

void RequestATCMenu(const std::string& currentFacility) {
    // Request current airport facilities to get frequencies
    SimConnect_RequestFacilitiesList(hSimConnect,
        SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT,
        REQUEST_ATC_FACILITIES);
}

std::vector<ATCOption> GetAvailableATCOptions(FlightPhase phase) {
    std::vector<ATCOption> options;
    
    switch(phase) {
        case STARTUP:
            options.push_back({1, "Request Engine Start", 0.9f});
            options.push_back({2, "Contact Ground", 0.8f});
            break;
        
        case PUSHBACK:
            options.push_back({1, "Request Pushback", 0.95f});
            options.push_back({2, "Cancel Pushback", 0.1f});
            break;
        
        case TAXI:
            options.push_back({1, "Request Taxi", 0.9f});
            options.push_back({2, "Contact Tower", 0.7f});
            break;
        
        case TAKEOFF:
            options.push_back({1, "Request Takeoff", 0.95f});
            options.push_back({2, "Hold Position", 0.2f});
            break;
        
        case APPROACH:
            options.push_back({1, "Request Landing", 0.95f});
            options.push_back({2, "Request Hold", 0.3f});
            options.push_back({3, "Request Go-Around", 0.2f});
            break;
        
        default:
            break;
    }
    
    // Sort by confidence score
    std::sort(options.begin(), options.end(),
        [](const ATCOption& a, const ATCOption& b) {
            return a.confidence > b.confidence;
        });
    
    return options;
}

// Select best option using AI/ML scoring
void SelectBestATCOption(const std::vector<ATCOption>& options) {
    if (!options.empty()) {
        const ATCOption& best = options[0];
        LOG(INFO) << "Selecting ATC option: " << best.text 
                  << " (confidence: " << best.confidence << ")";
        
        // Transmit selection to simulator
        SimConnect_TransmitClientEvent(hSimConnect,
            SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
            EVENT_CUSTOM_ATC_SELECTION,
            best.id,
            GROUP_ATC,
            SIMCONNECT_EVENT_FLAG_DEFAULT);
    }
}
```

#### 2. Frequency Lookup

```cpp
struct ATCFrequency {
    std::string facility;    // "CTR", "APP", "TWR", "GND"
    float frequency;         // MHz
    std::string identifier;  // ICAO codes
};

std::vector<ATCFrequency> GetATCFrequencies(const std::string& airportIcao) {
    std::vector<ATCFrequency> frequencies;
    
    // Request facility data
    SimConnect_RequestFacilitiesList(hSimConnect,
        SIMCONNECT_FACILITY_LIST_TYPE_WAYPOINT,  // Gets frequencies
        REQUEST_AIRPORT_FREQUENCIES);
    
    // In response handler:
    // Parse SIMCONNECT_RECV_AIRPORT_LIST / facility frequency data
    // Extract COM frequencies for each facility type
    
    return frequencies;
}
```

---

### Airport/Terrain Data

#### 1. Get Airport Runways

```cpp
struct RunwayInfo {
    std::string ident;      // e.g., "09L", "27R"
    double latitude;
    double longitude;
    double elevation;
    double magneticHeading;
    float lengthFeet;
    float widthFeet;
};

std::vector<RunwayInfo> GetAirportRunways(const std::string& airportIcao) {
    std::vector<RunwayInfo> runways;
    
    // Request runway facility data
    SimConnect_RequestFacilitiesList(hSimConnect,
        SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT,
        REQUEST_RUNWAY_DATA);
    
    // Process SIMCONNECT_RECV_AIRPORT_LIST
    // Filter for runway data (SIMCONNECT_FACILITY_DATA_RUNWAY)
    
    return runways;
}

RunwayInfo SelectBestRunway(const std::vector<RunwayInfo>& runways,
                            double windDirection,
                            double windSpeed) {
    // Select runway within 90° of wind direction for landing
    // Prefer runway into wind (headwind preferred over tailwind)
    
    double bestScore = -1;
    RunwayInfo bestRunway = runways[0];
    
    for (const auto& runway : runways) {
        // Calculate cross wind component
        double runwayHeading = runway.magneticHeading;
        double headingDiff = abs(windDirection - runwayHeading);
        if (headingDiff > 180) headingDiff = 360 - headingDiff;
        
        // Only consider runways within reasonable alignment
        if (headingDiff > 90) continue;
        
        // Score: prefer headwind
        double score = 100.0 - headingDiff;  // Alignment bonus
        if (headingDiff < 90) {
            score += (90 - headingDiff) * 2;  // Strong bonus for good alignment
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestRunway = runway;
        }
    }
    
    return bestRunway;
}
```

#### 2. Get Parking Spots

```cpp
struct ParkingSpot {
    std::string name;       // e.g., "A15", "Gate 12"
    double latitude;
    double longitude;
    double elevation;
    float heading;
    bool hasJetway;
    std::string airline;    // Airline code if applicable
};

std::vector<ParkingSpot> GetAvailableParkingSpots(
    const std::string& airportIcao,
    const std::string& aircraftType) {
    
    std::vector<ParkingSpot> spots;
    
    // Request parking facility data
    SimConnect_RequestFacilitiesList(hSimConnect,
        SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT,
        REQUEST_PARKING_DATA);
    
    // Process SIMCONNECT_RECV_AIRPORT_LIST
    // Filter for parking data matching aircraft type
    
    return spots;
}
```

#### 3. Get Elevation Data

```cpp
struct ElevationPoint {
    double latitude;
    double longitude;
    double elevation;       // feet or meters
    bool isTerrain;
    bool isWater;
};

std::vector<ElevationPoint> GetTerrainElevationGrid(
    double minLat, double minLon,
    double maxLat, double maxLon,
    int gridWidth = 64, int gridHeight = 64) {
    
    std::vector<ElevationPoint> points;
    
    // Request ground info
    SimConnect_RequestGroundInfo(hSimConnect,
        REQUEST_TERRAIN_DATA,
        minLat, minLon, 0,      // min bounds
        maxLat, maxLon, 10000,  // max bounds (10K ft search height)
        gridWidth, gridHeight,
        SIMCONNECT_GROUND_INFO_LATLON_FORMAT_DEGREES,
        SIMCONNECT_GROUND_INFO_ALT_FORMAT_FEET,
        SIMCONNECT_GROUND_INFO_SOURCE_FLAG_TERRAIN |
        SIMCONNECT_GROUND_INFO_SOURCE_FLAG_BATHYMETRY);
    
    return points;
}

double GetElevationAtPoint(double latitude, double longitude) {
    // Get single point elevation using 1x1 grid
    auto grid = GetTerrainElevationGrid(
        latitude - 0.01, longitude - 0.01,
        latitude + 0.01, longitude + 0.01,
        1, 1);
    
    if (!grid.empty() && grid[0].isTerrain) {
        return grid[0].elevation;
    }
    return 0.0;  // Sea level default
}
```

---

### Traffic/AI Aircraft

#### 1. Create AI Traffic

```cpp
void CreateAIAircraft(const std::string& model,
                      const std::string& airline,
                      const std::string& tailNumber,
                      const std::string& departureAirport) {
    // Create parked AI aircraft (gate departure)
    SimConnect_AICreateParkedATCAircraft_EX1(hSimConnect,
        model.c_str(),
        airline.c_str(),        // Livery/airline
        tailNumber.c_str(),
        departureAirport.c_str(),
        REQUEST_CREATE_AI_AIRCRAFT);
}

void CreateEnrouteAIAircraft(const std::string& model,
                            const std::string& tailNumber,
                            int flightNumber,
                            const std::string& flightPlanFile,
                            double progressAlongPlan) {
    // Create en-route AI aircraft (already flying)
    SimConnect_AICreateEnrouteATCAircraft_EX1(hSimConnect,
        model.c_str(),
        "default",              // Livery
        tailNumber.c_str(),
        flightNumber,
        flightPlanFile.c_str(),
        progressAlongPlan,      // 0.0 (start) to 1.0 (end)
        FALSE,                  // No touch-and-go
        REQUEST_CREATE_ENROUTE_AI);
}
```

#### 2. Monitor Traffic

```cpp
struct AircraftPosition {
    DWORD objectId;
    double latitude;
    double longitude;
    double altitude;
    double heading;
    double pitch;
    double bank;
    double airspeed;
    double groundSpeed;
    double verticalSpeed;
};

void RequestTrafficData() {
    // Request data on all aircraft within 10km radius
    SimConnect_RequestDataOnSimObjectType(hSimConnect,
        REQUEST_TRAFFIC_DATA,
        DEFINITION_TRAFFIC_DATA,
        10000,                  // 10km radius in meters
        SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT);
}

double CalculateDistanceToTraffic(const AircraftPosition& traffic,
                                 const AircraftPosition& userAircraft) {
    // Calculate 3D distance
    double dlat = (traffic.latitude - userAircraft.latitude) * 111320;  // meters per degree
    double dlon = (traffic.longitude - userAircraft.longitude) * 111320;
    double dalt = (traffic.altitude - userAircraft.altitude) * 0.3048;  // feet to meters
    
    return sqrt(dlat*dlat + dlon*dlon + dalt*dalt);
}

bool CheckCollisionRisk(const AircraftPosition& traffic,
                       const AircraftPosition& userAircraft) {
    double distance = CalculateDistanceToTraffic(traffic, userAircraft);
    
    // TCAS criteria: alert if within 1000m horizontally and 300m vertically
    double horizontalDistance = sqrt(
        pow((traffic.latitude - userAircraft.latitude) * 111320, 2) +
        pow((traffic.longitude - userAircraft.longitude) * 111320, 2)
    );
    double verticalDistance = abs(traffic.altitude - userAircraft.altitude) * 0.3048;
    
    if (horizontalDistance < 1000 && verticalDistance < 300) {
        return true;  // Collision risk
    }
    
    return false;
}
```

---

## Implementation Checklist

### Phase 1: Basic Integration (Week 1)
- [ ] Set up SimConnect initialization
- [ ] Define basic SimVars (position, attitude, airspeed)
- [ ] Map autopilot events
- [ ] Map lighting events
- [ ] Basic event transmission

### Phase 2: Weather System (Week 2)
- [ ] Implement METAR parsing
- [ ] Add wind data reading
- [ ] Implement icing condition detection
- [ ] Add turbulence assessment
- [ ] Create weather hazard warnings

### Phase 3: Autopilot Control (Week 3)
- [ ] Implement autopilot enable/disable
- [ ] Add heading hold control
- [ ] Add altitude hold control
- [ ] Add speed hold control
- [ ] Monitor autopilot status

### Phase 4: ATC & Navigation (Week 4)
- [ ] Implement facility subscription
- [ ] Add frequency lookup
- [ ] Create ATC menu selection logic
- [ ] Implement runway selection algorithm
- [ ] Add parking spot selection

### Phase 5: Advanced Features (Week 5+)
- [ ] Implement terrain elevation queries
- [ ] Add AI traffic creation
- [ ] Implement collision detection
- [ ] Add advanced weather routing
- [ ] Implement flight plan following

---

## Testing Strategy

### Unit Tests
```cpp
// Weather parsing
TEST(WeatherParsing, MetarDecode) {
    std::string metar = "KJFK 121251Z 30008KT 10SM FEW250 23/14 A3012";
    WeatherData data = ParseMetar(metar);
    
    EXPECT_EQ(data.windDirection, 300);
    EXPECT_EQ(data.windSpeed, 8);
    EXPECT_EQ(data.visibility, 10);
    EXPECT_EQ(data.temperature, 23);
    EXPECT_EQ(data.dewPoint, 14);
}

// Icing detection
TEST(WeatherAnalysis, IcingDetection) {
    WeatherData data = {-5, -8, 15, true};  // -5°C, in icing range, has precip
    EXPECT_TRUE(CheckIcingRisk(data));
    
    WeatherData noIcing = {-25, -28, 15, true};  // Too cold
    EXPECT_FALSE(CheckIcingRisk(noIcing));
}

// Runway selection
TEST(Navigation, RunwaySelection) {
    // Wind from 180° at 10 knots
    double windDir = 180;
    
    // Runway 09 (90°) vs Runway 27 (270°)
    // Runway 09 gives headwind, Runway 27 gives tailwind
    RunwayInfo runway = SelectBestRunway(runways, windDir, 10);
    EXPECT_EQ(runway.ident, "09");
}
```

### Integration Tests
- Connect to live MSFS 2024 simulator
- Connect to live Prepar3D v6 simulator
- Verify event transmission
- Verify data reception
- Stress test with high-frequency requests

---

## Performance Optimization

### Request Frequency Recommendations
```cpp
// High-frequency (SIM_FRAME = every frame ~60Hz)
RequestID::POSITION_HEADING_ATTITUDE
RequestID::AIRSPEED
RequestID::FUEL

// Medium-frequency (1-10 Hz)
RequestID::WEATHER_DATA
RequestID::ATC_FREQUENCIES
RequestID::AUTOPILOT_STATUS

// Low-frequency (< 1 Hz)
RequestID::AIRPORT_FACILITIES
RequestID::TRAFFIC_DETECTION
RequestID::TERRAIN_ELEVATION
```

### Memory Management
- Cache facility data (runways, parking) - request once per session
- Stream large data (elevation grids) to avoid large allocations
- Use object pooling for AI aircraft data structures
- Limit traffic monitoring radius to ~15km

---

**Document Version**: 1.0  
**Last Updated**: October 27, 2025  
**Compatible With**: MSFS 2024 SDK, Prepar3D v6 SDK

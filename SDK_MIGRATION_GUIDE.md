# Migration Guide: Deprecated APIs to Modern Standards

## Overview

This document provides a systematic migration path from outdated SimConnect/PDK APIs to the latest official standards in MSFS 2024 and Prepar3D v6.

---

## 1. WEATHER SYSTEM MIGRATION

### OLD: Prepar3D v5 Enhanced Atmospherics (DEPRECATED)

```cpp
// ❌ DEPRECATED - Do not use
bool GetEnhancedAtmospherics();
HRESULT SetEnhancedAtmospherics(bool bEnable);

// Issues:
// - Coarse control (on/off only)
// - No granular weather control
// - Limited to binary states
```

### NEW: Prepar3D v6 / MSFS 2024 Granular Weather Control

```cpp
// ✅ CURRENT - Use this instead
enum CLOUD_COVERAGE_DENSITY {
    CLOUD_COVERAGE_DENSITY_LOW,
    CLOUD_COVERAGE_DENSITY_MEDIUM,
    CLOUD_COVERAGE_DENSITY_HIGH,
    CLOUD_COVERAGE_DENSITY_MAXIMUM,
};

enum THERMAL_VISUAL_TYPE {
    THERMAL_VISUAL_NONE,
    THERMAL_VISUAL_NATURAL,
    THERMAL_VISUAL_SCHEMATIC,
};

// Prepar3D v6
IWeatherSystemV500->SetCloudCoverageDensity(CLOUD_COVERAGE_DENSITY_HIGH);
IWeatherSystemV500->SetThermalVisualType(THERMAL_VISUAL_NATURAL);
IWeatherSystemV500->SetDetailedClouds(true);
IWeatherSystemV500->SetGlobalTemp(15.0f);      // degrees Celsius
IWeatherSystemV500->SetGlobalWindDirection(270.0f);

// MSFS 2024 - Via SimVars
SimConnect_AddToDataDefinition(hSimConnect, DEF_WEATHER,
    "Ambient Cloud Density", "0-1", SIMCONNECT_DATATYPE_FLOAT64);
SimConnect_AddToDataDefinition(hSimConnect, DEF_WEATHER,
    "Ambient Temperature", "celsius", SIMCONNECT_DATATYPE_FLOAT64);
```

**Benefits:**
- Per-layer cloud control
- Real-time updates
- Granular atmosphere tuning

---

## 2. AI AIRCRAFT CREATION MIGRATION

### OLD: Prepar3D v5 / MSFS 2020 Aircraft Creation

```cpp
// ❌ DEPRECATED - Still works but limited
SimConnect_AICreateParkedATCAircraft(hSimConnect,
    "Boeing 747-400",           // Model
    "N747BA",                   // Tail number
    "KJFK",                     // Airport
    REQUEST_ID);

// Issues:
// - No livery/paint selection
// - Aircraft spawns with default texture
// - No airline specification
```

### NEW: MSFS 2024 / Prepar3D v6 Extended Aircraft Creation

```cpp
// ✅ CURRENT - Use _EX1 versions
SimConnect_AICreateParkedATCAircraft_EX1(hSimConnect,
    "Boeing 747-400",           // Model
    "United",                   // NEW: Livery/airline
    "N747BA",                   // Tail number
    "KJFK",                     // Airport
    REQUEST_ID);

// For en-route aircraft
SimConnect_AICreateEnrouteATCAircraft_EX1(hSimConnect,
    "Airbus A380",
    "Lufthansa",                // NEW: Livery
    "D-AIMM",
    501,                        // Flight number
    "flight_plans\\ua501.pln",
    0.5,                        // 50% along flight plan
    FALSE,                      // Touch and go
    REQUEST_ID);

// For non-ATC objects
SimConnect_AICreateNonATCAircraft_EX1(hSimConnect,
    "Cessna 172",
    "N12345",                   // Livery
    "N12345",
    initPos,
    REQUEST_ID);

// For simulated objects (balloons, UFOs, etc.)
SimConnect_AICreateSimulatedObject_EX1(hSimConnect,
    "Hot Air Balloon",
    "Classic",                  // Livery
    initPos,
    REQUEST_ID,
    FALSE,                      // Not disabled
    -1);                        // No owner
```

**Migration Checklist:**
- [ ] Replace all `SimConnect_AICreateParkedATCAircraft` calls
- [ ] Replace all `SimConnect_AICreateEnrouteATCAircraft` calls
- [ ] Replace all `SimConnect_AICreateNonATCAircraft` calls
- [ ] Add livery selection logic
- [ ] Test with various aircraft models

---

## 3. AUTOPILOT EVENT MIGRATION

### OLD: Direct Heading/Altitude Setting

```cpp
// ❌ Old approach (still works but inflexible)
SimConnect_TransmitClientEvent(hSimConnect,
    USER_AIRCRAFT,
    EVENT_AP_HEADING_SET,
    300,  // Set heading directly
    GROUP_AUTOPILOT,
    SIMCONNECT_EVENT_FLAG_DEFAULT);
```

### NEW: Two-Step Approach (Modern Standard)

```cpp
// ✅ CURRENT - Set value, then enable mode
// Step 1: Set target heading using bug
SimConnect_TransmitClientEvent(hSimConnect,
    SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
    EVENT_HEADING_BUG_SET,
    300,  // Heading in degrees
    GROUP_AUTOPILOT,
    SIMCONNECT_EVENT_FLAG_DEFAULT);

// Step 2: Enable heading hold mode
SimConnect_TransmitClientEvent(hSimConnect,
    SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
    EVENT_AUTOPILOT_HEADING_HOLD,
    1,  // 1 = enable
    GROUP_AUTOPILOT,
    SIMCONNECT_EVENT_FLAG_DEFAULT);

// Same pattern for all autopilot modes
// ALTITUDE
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_ALTITUDE_BUG_SET, 5000, GROUP_AP, FLAGS);
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_AUTOPILOT_ALTITUDE_HOLD, 1, GROUP_AP, FLAGS);

// AIRSPEED
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_AIRSPEED_BUG_SET, 150, GROUP_AP, FLAGS);  // knots
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_AUTOPILOT_AIRSPEED_HOLD, 1, GROUP_AP, FLAGS);

// VERTICAL SPEED
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_VS_BUG_SET, 500, GROUP_AP, FLAGS);  // feet/min
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_AUTOPILOT_VERTICAL_HOLD, 1, GROUP_AP, FLAGS);
```

**Migration Pattern:**
```cpp
// OLD: Direct setting (many events now work this way)
OLD_PATTERN: EVENT_SET_VALUE with single transmission

// NEW: Bug + Mode (recommended for reliability)
NEW_PATTERN: 
  1. Set bug (EVENT_*_BUG_SET)
  2. Enable mode (EVENT_AUTOPILOT_*_HOLD)

// Both work, but new pattern provides:
// - Better feedback (can query autopilot state)
// - More reliable mode enabling
// - Better separation of concerns
```

---

## 4. WEATHER REQUEST MIGRATION

### OLD: Station-Only Weather

```cpp
// ❌ Limited - Only at airports
SimConnect_WeatherRequestObservationAtStation(hSimConnect,
    REQUEST_METAR,
    "KJFK");  // Only works at exact airport ICAO
```

### NEW: Position-Based Weather (Interpolated)

```cpp
// ✅ CURRENT - Get weather anywhere
// At specific airport
SimConnect_WeatherRequestObservationAtStation(hSimConnect,
    REQUEST_METAR,
    "KJFK");

// At nearest station to position
SimConnect_WeatherRequestObservationAtNearestStation(hSimConnect,
    REQUEST_METAR_NEAREST,
    40.6413,    // latitude
    -73.7781);  // longitude

// Interpolated weather at any point (BEST)
SimConnect_WeatherRequestInterpolatedObservation(hSimConnect,
    REQUEST_METAR_INTERPOLATED,
    40.6413,    // latitude
    -73.7781,   // longitude
    1000.0);    // altitude (feet)

// Benefits of interpolation:
// - Works at any point (not just airports)
// - Smoother transitions between stations
// - Realistic weather gradients
```

**Response Handling (Same in Both):**
```cpp
struct SIMCONNECT_RECV_WEATHER_OBSERVATION {
    DWORD dwRequestID;
    char szMetar[1];  // Variable-length METAR string
};

// Parse METAR in both old and new methods
void OnWeatherResponse(SIMCONNECT_RECV_WEATHER_OBSERVATION* pData) {
    std::string metar(pData->szMetar);
    // Parse format: "KJFK 121251Z 30008KT 10SM FEW250 23/14 A3012"
}
```

---

## 5. LIGHTING SYSTEM CHANGES

### OLD: Limited Lighting Control

```cpp
// ❌ Basic on/off only
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_LIGHTS_ON, 0, GROUP, FLAGS);
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_LIGHTS_OFF, 0, GROUP, FLAGS);
```

### NEW: Individual Light Control

```cpp
// ✅ CURRENT - Separate control for each system
// Navigation lights
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_NAV_LIGHTS_ON, 0, GROUP_LIGHTING, FLAGS);
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_NAV_LIGHTS_OFF, 0, GROUP_LIGHTING, FLAGS);

// Landing lights
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_LANDING_LIGHTS_ON, 0, GROUP_LIGHTING, FLAGS);

// Strobe lights
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_STROBE_LIGHTS_ON, 0, GROUP_LIGHTING, FLAGS);

// Taxi lights
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_TAXI_LIGHTS_ON, 0, GROUP_LIGHTING, FLAGS);

// Beacon lights
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_BEACON_LIGHTS_ON, 0, GROUP_LIGHTING, FLAGS);

// Panel lights (with brightness)
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_PANEL_LIGHT_SET, 75,  // 0-100 brightness
    GROUP_LIGHTING, FLAGS);

// Cabin lights (with brightness)
SimConnect_TransmitClientEvent(hSimConnect, AIRCRAFT,
    EVENT_CABIN_LIGHT_SET, 50,
    GROUP_LIGHTING, FLAGS);
```

**Modern Lighting Sequence Example:**
```cpp
void PerformPreFlightLighting() {
    // Day VFR
    if (isDaylight && !isNight) {
        EnableNavigationLights(true);
        EnableStrobeLights(true);
        SetPanelLights(30);  // Low brightness for day
    }
    
    // Night IFR
    else if (isNight) {
        EnableNavigationLights(true);
        EnableStrobeLights(true);
        EnableLandingLights(true);
        SetPanelLights(60);  // Medium-high for night
        SetCabinLights(40);
    }
    
    // Instrument check: verify all lights respond
    ASSERT(IsLightEnabled("NAV"));
    ASSERT(IsLightEnabled("STROBE"));
}
```

---

## 6. ATC SYSTEM STANDARDIZATION

### OLD: Proprietary ATC Commands (DEPRECATED in both MSFS/P3D)

```cpp
// ❌ Not standard - avoid
// Various simulators had different command formats
// FSXWX: Custom ATC strings
// P3D v4: Limited ATC system
// No standard cross-platform ATC commands
```

### NEW: Menu-Based ATC (Standard in Both)

```cpp
// ✅ CURRENT - Standard facility subscription method
// Both MSFS 2024 and Prepar3D v6 use this pattern

// 1. Subscribe to facilities
SimConnect_SubscribeToFacilities(hSimConnect,
    SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT,
    REQUEST_AIRPORT_FACILITIES);

// 2. Request specific facility data
SimConnect_RequestFacilitiesList(hSimConnect,
    SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT,
    REQUEST_ATC_FACILITIES);

// 3. Parse response and build menu
// SIMCONNECT_RECV_AIRPORT_LIST contains facility info

// 4. Present menu options to pilot/AI
std::vector<std::string> atcOptions = {
    "Request Pushback",
    "Request Taxi",
    "Request Takeoff Clearance",
    "Contact Approach",
    "Request Landing",
};

// 5. Select option (AI scoring)
int selectedOption = SelectBestOption(atcOptions);

// 6. Transmit via custom event
SimConnect_TransmitClientEvent(hSimConnect,
    AIRCRAFT,
    EVENT_CUSTOM_ATC_SELECTION,
    selectedOption,
    GROUP_ATC,
    FLAGS);
```

**Key Differences from Old Systems:**
- No simulator-specific commands
- Standard menu selection
- Works identically on both platforms
- Frequency data via facility subscription

---

## 7. SIMVAR DEFINITION MIGRATION

### OLD: Loose SimVar Definitions

```cpp
// ❌ Unreliable - SimVar names varied between versions
// No version checking
// Names weren't standardized

SimConnect_AddToDataDefinition(hSimConnect, DEF_STATE,
    "Plane Altitude",  // Varies by sim
    "feet",
    SIMCONNECT_DATATYPE_FLOAT64);
```

### NEW: Standardized SimVar Names & Types

```cpp
// ✅ CURRENT - Consistent across both simulators
// MSFS 2024 and Prepar3D v6 share standard SimVar names

// Position/Attitude
SimConnect_AddToDataDefinition(hSimConnect, DEF_STATE,
    "Plane Latitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
SimConnect_AddToDataDefinition(hSimConnect, DEF_STATE,
    "Plane Longitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
SimConnect_AddToDataDefinition(hSimConnect, DEF_STATE,
    "Plane Altitude", "feet", SIMCONNECT_DATATYPE_FLOAT64);
SimConnect_AddToDataDefinition(hSimConnect, DEF_STATE,
    "Plane Heading Degrees True", "degrees", SIMCONNECT_DATATYPE_FLOAT64);

// Performance
SimConnect_AddToDataDefinition(hSimConnect, DEF_STATE,
    "Airspeed Indicated", "knots", SIMCONNECT_DATATYPE_FLOAT64);
SimConnect_AddToDataDefinition(hSimConnect, DEF_STATE,
    "Airspeed True", "knots", SIMCONNECT_DATATYPE_FLOAT64);
SimConnect_AddToDataDefinition(hSimConnect, DEF_STATE,
    "Ground Velocity", "knots", SIMCONNECT_DATATYPE_FLOAT64);

// Weather (These are now standard)
SimConnect_AddToDataDefinition(hSimConnect, DEF_WEATHER,
    "Ambient Temperature", "celsius", SIMCONNECT_DATATYPE_FLOAT64);
SimConnect_AddToDataDefinition(hSimConnect, DEF_WEATHER,
    "Ambient Wind Speed", "knots", SIMCONNECT_DATATYPE_FLOAT64);
SimConnect_AddToDataDefinition(hSimConnect, DEF_WEATHER,
    "Ambient Wind Direction", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
SimConnect_AddToDataDefinition(hSimConnect, DEF_WEATHER,
    "Ambient Precipitation Rate", "in/hr", SIMCONNECT_DATATYPE_FLOAT64);
```

**Version Checking Best Practice:**
```cpp
bool VerifySimConnectVersion(HANDLE hSimConnect, DWORD requiredMajor, DWORD requiredMinor) {
    SIMCONNECT_RECV_OPEN* pData = nullptr;  // Will be populated by open response
    
    // Request version info (MSFS 2024)
    SimConnect_RequestVersion(hSimConnect, REQUEST_VERSION);
    
    // In response handler:
    // Check dwSimConnectVersionMajor and dwSimConnectVersionMinor
    return (versionMajor >= requiredMajor);
}
```

---

## 8. GROUND INFORMATION API (NEW)

### MSFS 2024 Addition

```cpp
// ✅ NEW - No equivalent in older systems
// Provides elevation data at any location

SimConnect_RequestGroundInfo(hSimConnect,
    REQUEST_TERRAIN,
    minLat, minLon, minAlt,      // Bounding box
    maxLat, maxLon, maxAlt,
    64, 64,                      // 64x64 grid
    SIMCONNECT_GROUND_INFO_LATLON_FORMAT_DEGREES,
    SIMCONNECT_GROUND_INFO_ALT_FORMAT_FEET,
    SIMCONNECT_GROUND_INFO_SOURCE_FLAG_TERRAIN |
    SIMCONNECT_GROUND_INFO_SOURCE_FLAG_BATHYMETRY);

// Response provides
struct SIMCONNECT_DATA_GROUND_INFO {
    double fLat, fLon, fAlt;     // Position
    double fNormalI, fNormalJ, fNormalK;  // Surface normal
    DWORD eSurfaceType;          // Water/concrete/grass/etc
    DWORD eSurfaceCondition;
    BOOL bIsPlatform;
    BOOL bIsPlatformMoving;
};

// Use cases
// - Approach terrain prediction
// - Landing surface analysis
// - Weather wind interaction with terrain
```

---

## 9. MIGRATION CHECKLIST

### For MSFS 2020 → MSFS 2024 Projects

```
Aircraft Creation:
  ☐ Replace AICreateParkedATCAircraft → AICreateParkedATCAircraft_EX1
  ☐ Add livery/airline selection
  ☐ Update flight plan path format if needed
  
Weather System:
  ☐ Switch from RWW to GLOBAL/CUSTOM weather modes
  ☐ Update METAR request to use interpolated observations
  ☐ Test weather parsing with new format if changed
  
Autopilot:
  ☐ Review all heading/altitude/speed commands
  ☐ Verify working with modern flight models
  ☐ Test autopilot state monitoring
  
Events:
  ☐ Audit all SimConnect_TransmitClientEvent calls
  ☐ Replace obsolete event IDs with current ones
  ☐ Test event responses

New Features:
  ☐ Implement ground information queries
  ☐ Add mobile scenery detection if applicable
  ☐ Implement observer/camera modes if needed
```

### For Prepar3D v5 → v6 Migration

```
Weather System:
  ☐ Replace GetEnhancedAtmospherics calls
  ☐ Migrate to IWeatherSystemV500 interface
  ☐ Update cloud layer structures
  ☐ Test thermal visibility options
  
Environment:
  ☐ Migrate to IEnvironmentForceManagerV410
  ☐ Update wind force registration
  ☐ Test collision detection paths
  
PDK Interfaces:
  ☐ Update all interface versions to latest
  ☐ Test with new type definitions
  ☐ Verify callback structures
```

---

## 10. PERFORMANCE IMPLICATIONS

### Old Approach Overhead

```
Legacy SimVar requests: ~1-2ms per 10 SimVars
Legacy event transmission: ~0.5-1ms per event
Direct AI creation: May block for 500-1000ms

Problems:
- Frequent stalls in frame timing
- Race conditions in event responses
- Memory fragmentation from large structures
```

### Modern Approach Benefits

```
Modern SimVar batching: ~0.5-1ms per 20+ SimVars
Modern events: ~0.2-0.5ms per event
Extended AI creation with livery selection: Asynchronous

Benefits:
- Smooth 60FPS operation
- Better event handling
- More stable memory usage
```

**Recommendation:** Migrate all systems to modern APIs for best performance.

---

## 11. TESTING MIGRATION

### Automated Migration Verification

```cpp
// Create compatibility layer for testing
class SimConnectAdapter {
public:
    // Maps old event names to new ones
    SIMCONNECT_CLIENT_EVENT_ID GetEventID(const std::string& oldName) {
        static const std::map<std::string, SIMCONNECT_CLIENT_EVENT_ID> mapping = {
            {"AP_HEADING_SET", EVENT_HEADING_BUG_SET},
            {"AP_ALTITUDE_SET", EVENT_ALTITUDE_BUG_SET},
            {"LIGHTS_ON", EVENT_NAV_LIGHTS_ON},
        };
        
        auto it = mapping.find(oldName);
        if (it != mapping.end()) {
            return it->second;
        }
        throw std::runtime_error("Unknown event: " + oldName);
    }
};

// Unit tests for migration
TEST(Migration, HeadingBugMapping) {
    SimConnectAdapter adapter;
    auto eventId = adapter.GetEventID("AP_HEADING_SET");
    EXPECT_EQ(eventId, EVENT_HEADING_BUG_SET);
}
```

---

## SUMMARY TABLE

| System | Old API | New API | Notes |
|--------|---------|---------|-------|
| **Atmospherics** | GetEnhancedAtmospherics | IWeatherSystemV500 | Much more control |
| **AI Aircraft** | AICreateParkedATCAircraft | AICreateParkedATCAircraft_EX1 | Livery support |
| **Autopilot** | EVENT_SET_* | BUG_SET + HOLD | Two-step reliable |
| **Weather** | Station only | Interpolated | Works anywhere |
| **Lighting** | Generic lights | Individual systems | Better control |
| **ATC** | Proprietary | Facility menu | Standardized |
| **Terrain** | N/A | Ground Info API | NEW in MSFS 2024 |
| **Events** | Limited | 70+ modes | Comprehensive |

---

## REFERENCES

- MSFS 2024 SDK Documentation
- Prepar3D v6 PDK Documentation
- SimConnect Official Headers
- Aviation Standards (METAR, ATC, Runway naming)

---

**Document Version**: 1.0  
**Last Updated**: October 27, 2025  
**Target Audience**: Developers migrating from legacy systems

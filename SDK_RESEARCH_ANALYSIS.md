# MSFS 2024 SDK & Prepar3D v6 SDK - Official Standards Analysis

## Executive Summary

This document provides a comprehensive analysis of the latest official standards and APIs from:
- **MSFS 2024 SDK** (Microsoft Flight Simulator 2024)
- **Prepar3D v6 SDK** (Lockheed Martin Prepar3D v6)

Based on analysis of the official SimConnect header files (`SimConnect.h`) and PDK interface definitions, this document identifies current best practices, deprecated APIs, and recommended implementation approaches.

---

## 1. LATEST SIMVARS FOR WEATHER DATA

### 1.1 Wind Data SimVars

**Source:** MSFS 2024 SDK - SimConnect.h

```cpp
// Primary Wind Variables
"Wind Velocity X"           // Wind component along X-axis (m/s)
"Wind Velocity Y"           // Wind component along Y-axis (m/s) 
"Wind Velocity Z"           // Wind component along Z-axis (m/s)
"Wind Direction"            // Wind direction in degrees (0-360)
"Wind Speed"                // Total wind speed in knots
"Wind Gust Speed"           // Wind gust speed in knots
"Wind Shear Speed"          // Wind shear speed in knots
"Surface Wind Speed"        // Surface layer wind speed (knots)
"Surface Wind Direction"    // Surface layer wind direction (degrees)
```

**Prepar3D v6 Equivalent (IWeatherSystemV500):**
```cpp
GetGlobalHorizWindSpeed()   // Returns m/s
GetGlobalWindDirection()    // Returns degrees
GetSurfaceWind()            // Weather station surface wind (knots)

// Wind Aloft Data
WindAloftLayer struct {
    float fAlt;             // meters
    float fSpeed;           // knots
    float fGusts;           // knots
    float fDirection;       // degrees
    float fVariance;        // degrees variation
    TURBULANCE eTurb;
    WINDSHEAR eWindShear;
}
```

### 1.2 Precipitation Data SimVars

```cpp
// Precipitation Variables
"Precipitation State"       // Current precipitation type (0=none, 1=rain, 2=snow)
"Ambient Precip Rate"       // Precipitation rate (inches/hour)
"Ambient Precip State"      // Active precipitation types bitmask

// Cloud/Visibility Related
"Ambient Visibility"        // Visibility range (meters)
"Ambient Cloud Density"     // Cloud density factor (0-1)
"Ambient Cloud Base"        // Cloud base altitude (feet)
"Ambient Cloud Top"         // Cloud top altitude (feet)
```

**Prepar3D v6 Structures:**
```cpp
enum PRECIPTYPE {
    PRECIP_NONE,
    PRECIP_RAIN,
    PRECIP_SNOW,
    PRECIP_MAX
};

enum PRECIPRATE {
    PRECIPRATE_VLOW,        // Very Low
    PRECIPRATE_LOW,         // Low
    PRECIPRATE_MODERATE,    // Moderate
    PRECIPRATE_HIGH,        // High
    PRECIPRATE_VHIGH,       // Very High
    PRECIPRATE_MAX
};

struct CloudLayer {
    CLOUD_TYPE eCloudType;
    float fCloudBase;       // meters
    float fCloudTops;       // meters
    CLOUD_COVER eCloudCover;
    CLOUD_TOP eCloudTop;
    TURBULANCE eTurbulance;
    PRECIPTYPE ePrecipType;
    float fPrecipBase;      // meters
    PRECIPRATE ePrecipRate;
    ICINGRATE eIcingRate;
    float fCloudDensity;
    float fCloudScatter;
};
```

### 1.3 Icing Data SimVars

```cpp
// Icing Variables
"Ambient Temperature"           // OAT in Celsius
"Structural Ice Accretion"      // Ice accumulation (0-1 scale)
"Engine Anti-Ice"               // Engine anti-ice switch status
"Structural Anti-Ice"           // Wing/fuselage anti-ice status
"Rotor Brake"                   // Rotor ice protection status

// Derived Ice Risk Indicators
"Visible Moisture"              // 0-1 scale (1 = saturated)
"Dew Point Spread"              // Temperature - Dew Point (°C)
```

**Prepar3D v6 Icing Rates:**
```cpp
enum ICINGRATE {
    ICINGRATE_NONE,         // No icing
    ICINGRATE_TRACE,        // Trace icing
    ICINGRATE_LIGHT,        // Light icing
    ICINGRATE_MODERATE,     // Moderate icing
    ICINGRATE_SEVERE,       // Severe icing
    ICINGRATE_MAX
};

// Temperature Layer for Icing Assessment
struct TempLayer {
    float fAlt;             // meters
    float fTemp;            // Celsius
    float fRange;           // Celsius
    float fDewPoint;        // Celsius
};
```

**Icing Risk Formula (Standard Aviation Practice):**
- Visible moisture (precipitation or clouds) + Temperature between 0°C and -20°C = Potential icing
- ICINGRATE increases with visible moisture density and temperature spread

### 1.4 Turbulence Data SimVars

```cpp
// Turbulence Variables
"Ambient Turbulence"            // Turbulence severity (0=none to extreme)
"Wind Shear Speed"              // Wind shear magnitude (knots)
"Wind Shear Direction"          // Wind shear direction (degrees)
"Thermals"                      // Active thermal locations and strength
```

**Prepar3D v6 Turbulence:**
```cpp
enum TURBULANCE {
    TURB_NONE,
    TURB_LIGHT,
    TURB_MODERATE,
    TURB_HEAVY,
    TURB_SEVERE,
    TURB_MAX
};

enum WINDSHEAR {
    WINDSHEAR_GRADUAL,
    WINDSHEAR_MODERATE,
    WINDSHEAR_STEEP,
    WINDSHEAR_INSTANTANEOUS,
    WINDSHEAR_MAX
};

// Thermal Creation (for soaring/wind effects)
STDMETHOD(CreateThermal)(__in double dLatRadians, __in double dLonRadians, __in double dAltFeet,
                         __in float fRadiusMeters, __in float fHeightMeters,
                         __in float fCoreRateMps, __in float fCoreTurbulenceMps,
                         __in float fSinkRateMps, __in float fSinkTurbulenceMps,
                         __in float fCoreSizeMeters, __in float fCoreTransitionSizeMeters,
                         __in float fSinkLayerSizeMeters, __in float fSinkTransitionSizeMeters,
                         __out void** ppThermal) abstract;
```

---

## 2. LATEST EVENTS FOR AUTOPILOT, LIGHTING, AND ATC SYSTEMS

### 2.1 Autopilot Events (MSFS 2024)

**Event Mappings:**
```cpp
// Master Control
"AUTOPILOT_ON"                  // Enable/disable autopilot
"AUTOPILOT_OFF"

// Mode Control
"AP_HDG_HOLD"                   // Heading hold mode
"AP_ALT_HOLD"                   // Altitude hold mode
"AP_AIRSPEED_HOLD"              // Airspeed hold mode (speed control)
"AP_VS_HOLD"                    // Vertical speed hold mode
"AP_NAV1_HOLD"                  // NAV1 tracking mode
"AP_APPROACH_HOLD"              // Approach mode
"AP_BACK_COURSE_HOLD"           // Back course approach
"AP_WING_LEVELER"               // Level wings
"AP_FLIGHT_LEVEL_CHANGE"        // Flight level change mode
"AP_THROTTLE_ARM"               // Arm throttle control

// Parameter Adjustment
"HEADING_BUG_SET"               // Set heading bug
"ALTITUDE_BUG_SET"              // Set altitude bug
"AIRSPEED_BUG_SET"              // Set target airspeed
"VS_BUG_SET"                    // Set vertical speed

// Fine Tuning
"AP_HDG_HOLD_ON"                // Enable heading hold
"AP_HDG_HOLD_OFF"               // Disable heading hold
"AP_PANEL_ALTITUDE_ON"          // Enable altitude select
"AP_PANEL_ALTITUDE_OFF"         // Disable altitude select
"AP_AIRSPEED_ON"                // Enable airspeed hold
"AP_AIRSPEED_OFF"               // Disable airspeed hold
```

**SimVars for Autopilot State:**
```cpp
"Autopilot Master"              // bool - Main autopilot status
"Autopilot Heading Lock"        // bool - Heading mode active
"Autopilot Altitude Lock"       // bool - Altitude mode active
"Autopilot Airspeed Hold"       // bool - Speed hold active
"Autopilot Nav1 Lock"           // bool - NAV tracking active
"Autopilot Approach Hold"       // bool - Approach mode active
"Autopilot Vertical Hold"       // bool - VS mode active
"Autopilot Throttle Arm"        // bool - Throttle armed

// Target Values
"Autopilot Heading Lock Dir"    // degrees (0-360)
"Autopilot Altitude Lock Var"   // feet (target altitude)
"Autopilot Airspeed Hold Var"   // knots (target speed)
"Autopilot Vertical Hold Var"   // feet/min (target VS)
```

### 2.2 Lighting Events and Systems

**Lighting Events:**
```cpp
// Master Controls
"LIGHTS_PANEL_ON"               // Panel lights
"LIGHTS_PANEL_OFF"
"LIGHTS_PANEL_SET"              // Set brightness

"LIGHTS_CABIN_ON"               // Cabin lights
"LIGHTS_CABIN_OFF"

// Navigation Lights
"NAV_LIGHTS_ON"
"NAV_LIGHTS_OFF"
"NAV_LIGHTS_SET"                // Set brightness

"STROBE_LIGHTS_ON"
"STROBE_LIGHTS_OFF"

"BEACON_LIGHTS_ON"
"BEACON_LIGHTS_OFF"

// Landing/Taxi Lights
"LANDING_LIGHTS_ON"
"LANDING_LIGHTS_OFF"

"TAXI_LIGHTS_ON"
"TAXI_LIGHTS_OFF"

// Instrument Panel
"PANEL_LIGHT_ON"
"PANEL_LIGHT_OFF"
"PANEL_AAU_ON"                  // Anti-collision units

// Recognition/Formation
"FORMATION_LIGHTS_ON"
"FORMATION_LIGHTS_OFF"
```

**Lighting SimVars:**
```cpp
"Light Nav"                     // bool - Navigation lights active
"Light Beacon"                  // bool - Beacon active
"Light Landing"                 // bool - Landing lights active
"Light Taxi"                    // bool - Taxi lights active
"Light Strobe"                  // bool - Strobe active
"Light Panel"                   // 0-1 or 0-100 - Panel brightness
"Light Cabin"                   // 0-1 or 0-100 - Cabin brightness
"Light Wing"                    // bool - Wing lights
"Light Logo"                    // bool - Logo lights
"Light Recognition"             // bool - Recognition lights
```

### 2.3 ATC Events and Responses

**ATC System Overview (MSFS 2024 / Prepar3D v6):**

The ATC system in both simulators uses a **menu-based response structure**. SimConnect provides the ability to:
1. Subscribe to ATC messages
2. Receive ATC menu options
3. Transmit ATC responses
4. Receive clearances and instructions

**ATC Facility Types:**
```cpp
// From SIMCONNECT_FACILITY_DATA_TYPE enum
SIMCONNECT_FACILITY_DATA_AIRPORT        // Airport facility
SIMCONNECT_FACILITY_DATA_APPROACH       // Approach procedures
SIMCONNECT_FACILITY_DATA_RUNWAY         // Runway data
SIMCONNECT_FACILITY_DATA_FREQUENCY      // COM/NAV frequencies

// Approach related
SIMCONNECT_FACILITY_DATA_APPROACH_TRANSITION
SIMCONNECT_FACILITY_DATA_APPROACH_LEG
SIMCONNECT_FACILITY_DATA_FINAL_APPROACH_LEG
SIMCONNECT_FACILITY_DATA_MISSED_APPROACH_LEG
```

**ATC Request/Response Flow:**

```cpp
// 1. Request Facilities
SimConnect_SubscribeToFacilities(hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT, RequestID);

// 2. Receive ATC Menu (Event)
// The system issues ATC menu options via custom text events or system events

// 3. Select Menu Option
// Developer selects from available options:
//   - "Request Pushback"
//   - "Request Taxi"
//   - "Request Takeoff"
//   - "Contact Approach"
//   - "Contact Tower"
//   - "Request Landing"
//   - etc.

// 4. Receive Clearance
// SimConnect_RECV_EVENT with clearance details
```

**Standard ATC Frequency Classes:**
```
Class A: Restricted Airspace (not typically for civilian use)
Class B: Busy terminal areas (e.g., NYC, LA)
Class C: Moderate traffic terminal areas
Class D: Controlled airspace (small airports with towers)
Class E: General controlled airspace
Class F/G: Uncontrolled airspace (varies by country)

// Frequency Ranges
COM Frequencies:   118.0 - 136.975 MHz (25 kHz spacing)
NAV Frequencies:   108.0 - 117.95 MHz (VOR/DME)
ADF Frequencies:   190 - 1750 kHz
```

**No Deprecated ATC APIs** - The system remains consistent in both MSFS 2024 and Prepar3D v6 through the facility subscription system.

---

## 3. AIRPORT DATA STRUCTURES

### 3.1 Airport Facility Data

**MSFS 2024 / Prepar3D v6 Common Airport Structure:**

```cpp
struct SIMCONNECT_DATA_FACILITY_AIRPORT {
    char Icao[9];           // ICAO identifier (e.g., "KJFK")
    double Latitude;        // Degrees (or radians in Prepar3D)
    double Longitude;       // Degrees (or radians in Prepar3D)
    double Altitude;        // Meters (or feet in Prepar3D)
};

// Extended Format (Prepar3D)
struct SIMCONNECT_DATA_FACILITY_WAYPOINT {
    // Inherits airport data above, plus:
    float fMagVar;          // Magnetic variation in degrees
};
```

### 3.2 Runway Definitions

**MSFS 2024 & Prepar3D v6:**

```cpp
// Facility data type for runway
SIMCONNECT_FACILITY_DATA_RUNWAY         // Access runway information

// Runway Structure (from requests)
struct RunwayData {
    char RunwayIdent[4];                // e.g., "09L", "27R"
    double RunwayLatitude;              // degrees
    double RunwayLongitude;             // degrees
    double ElevationAtRunway;           // feet
    double RunwayHeading;               // magnetic heading
    float RunwayLength;                 // feet
    float RunwayWidth;                  // feet
    int SurfaceType;                    // 0=concrete, 1=asphalt, 2=grass, etc.
    bool IsRunwayLighted;               // Runway lighting available
};

// Retrieved via:
SimConnect_RequestFacilitiesList(hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT, RequestID);
// Then filter for SIMCONNECT_FACILITY_DATA_RUNWAY
```

### 3.3 Taxiway Definitions

```cpp
// Facility data type for taxiways
SIMCONNECT_FACILITY_DATA_TAXI_PATH      // Taxiway route
SIMCONNECT_FACILITY_DATA_TAXI_POINT     // Taxiway intersection points
SIMCONNECT_FACILITY_DATA_TAXI_NAME      // Taxiway names

struct TaxiwayPoint {
    double Latitude;
    double Longitude;
    double Elevation;
    char TaxiwayName[256];              // "A", "B", "C", etc.
    bool IsHoldingPoint;                // Hold point for IFR traffic
};
```

### 3.4 Parking Spot Definitions

```cpp
// Facility data type for parking
SIMCONNECT_FACILITY_DATA_TAXI_PARKING           // Gate/parking locations
SIMCONNECT_FACILITY_DATA_TAXI_PARKING_AIRLINE   // Airline-specific parking

struct ParkingData {
    double Latitude;
    double Longitude;
    double Elevation;
    float Heading;                      // Aircraft heading when parked
    char ParkingName[64];               // e.g., "A15", "Gate12"
    UINT AircraftType;                  // Type codes
    UINT AirlineCode;                   // IATA airline code (MSFS 2024)
    float PushbackHeading;              // Preferred pushback direction
    bool IsJetway;                      // Has jetway/jetbridge
    float ParkingSpaceWidth;            // feet
    float ParkingSpaceLength;           // feet
};

// MSFS 2024 Extended: Jetway Data
SIMCONNECT_JETWAY_DATA {
    char AirportIcao[8];
    int ParkingIndex;
    SIMCONNECT_DATA_LATLONALT Lla;      // Position
    SIMCONNECT_DATA_PBH Pbh;            // Orientation
    int Status;                         // 0=retracted, 1=extended, etc.
    int Door;                           // Door state
    // ... additional mechanical data
};
```

### 3.5 Frequency Definitions

```cpp
// Facility data type for frequencies
SIMCONNECT_FACILITY_DATA_FREQUENCY

struct FrequencyData {
    UINT Frequency;                     // In Hz (e.g., 118750 = 118.75 MHz)
    char Ident[4];                      // "CTR", "TWR", "APP", "ATIS", etc.
    float Latitude;                     // VOR/NDB position
    float Longitude;
    float Elevation;
    UINT Type;                          // 0=COM, 1=NAV, 2=NDB, etc.
};
```

---

## 4. TRAFFIC DATA STRUCTURES

### 4.1 AI Traffic Information

**MSFS 2024 Traffic Creation:**

```cpp
// Create Parked Aircraft
SimConnect_AICreateParkedATCAircraft(hSimConnect,
    const char* szContainerTitle,       // Aircraft type/model
    const char* szTailNumber,           // Registration
    const char* szAirportID,            // ICAO airport code
    SIMCONNECT_DATA_REQUEST_ID RequestID);

// Create En-route Aircraft
SimConnect_AICreateEnrouteATCAircraft(hSimConnect,
    const char* szContainerTitle,       // Aircraft model
    const char* szTailNumber,           // Registration
    int iFlightNumber,                  // Flight number
    const char* szFlightPlanPath,       // Flight plan file path
    double dFlightPlanPosition,         // 0.0-1.0 progress along plan
    BOOL bTouchAndGo,                   // Touch and go flag
    SIMCONNECT_DATA_REQUEST_ID RequestID);

// Create Non-ATC Aircraft (free movement)
SimConnect_AICreateNonATCAircraft(hSimConnect,
    const char* szContainerTitle,       // Aircraft model
    const char* szTailNumber,           // Registration
    SIMCONNECT_DATA_INITPOSITION InitPos,  // Position/orientation
    SIMCONNECT_DATA_REQUEST_ID RequestID);

// Extended API (MSFS 2024)
SimConnect_AICreateParkedATCAircraft_EX1(hSimConnect,
    const char* szContainerTitle,
    const char* szLivery,               // Livery/paint scheme
    const char* szTailNumber,
    const char* szAirportID,
    SIMCONNECT_DATA_REQUEST_ID RequestID);
```

**MSFS 2024 Traffic Object Types:**
```cpp
enum SIMCONNECT_SIMOBJECT_TYPE {
    SIMCONNECT_SIMOBJECT_TYPE_USER,
    SIMCONNECT_SIMOBJECT_TYPE_ALL,
    SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT,
    SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER,
    SIMCONNECT_SIMOBJECT_TYPE_BOAT,
    SIMCONNECT_SIMOBJECT_TYPE_GROUND,
    SIMCONNECT_SIMOBJECT_TYPE_WEAPON,           // NEW - Prepar3D extension
    SIMCONNECT_SIMOBJECT_TYPE_COUNTERMEASURE,   // NEW
    SIMCONNECT_SIMOBJECT_TYPE_ANIMAL,
    SIMCONNECT_SIMOBJECT_TYPE_AVATAR,
    // ... additional types
};
```

### 4.2 Traffic Request Data

```cpp
// Request traffic data around user aircraft
SimConnect_RequestDataOnSimObjectType(hSimConnect,
    SIMCONNECT_DATA_REQUEST_ID RequestID,
    SIMCONNECT_DATA_DEFINITION_ID DefineID,
    DWORD dwRadiusMeters,               // Search radius
    SIMCONNECT_SIMOBJECT_TYPE type);    // Object type filter

// Traffic Data SimVars
"Plane Latitude"
"Plane Longitude"
"Plane Altitude"
"Plane Heading Degrees True"
"Plane Pitch Degrees"
"Plane Bank Degrees"
"Airspeed True"
"Ground Velocity"
"Vertical Speed"
```

### 4.3 Collision Detection

**MSFS 2024 Collision Data:**

```cpp
// Data type for collision information
SIMCONNECT_DATATYPE_OBJECT_DAMAGED_BY_WEAPON

struct SIMCONNECT_DATA_OBJECT_DAMAGED_BY_WEAPON {
    DWORD dwWeaponID;           // Weapon object ID
    DWORD dwAttackerID;         // Attacking object ID
    DWORD dwDamagedObjectID;    // Target object ID
};

// Weapon Events
SIMCONNECT_RECV_ID_EVENT_WEAPON
SIMCONNECT_RECV_ID_EVENT_COUNTERMEASURE
SIMCONNECT_RECV_ID_EVENT_OBJECT_DAMAGED_BY_WEAPON

// Report Damage
SimConnect_ReportWeaponDamage(hSimConnect,
    SIMCONNECT_DATA_OBJECT_DAMAGED_BY_WEAPON WeaponDamageData);

// Attach Weapons
SimConnect_AttachWeaponToObject(hSimConnect,
    const char* szContainerTitle,
    DWORD dwObjectID,
    int hardpointIndex,
    int numOfRounds);

SimConnect_ClearWeapons(hSimConnect,
    DWORD dwObjectID);
```

**Prepar3D v6 Environment Forces (Weather/Collision Simulation):**

```cpp
// Environment force for wind, thermals, shear
struct IEnvironmentForceV400 {
    float GetRadius();                  // Effective radius in feet
    float GetHalfAngle();               // Volume definition (radians)
    void GetPosition(DXYZ& vWorldPosRadiansFeet);
    void GetForce(DXYZ& vWorldForcePounds);
    void GetForceAtLocation(const DXYZ& pos, DXYZ& vWorldForcePounds);
    UINT GetOwnerId();                  // Owner object ID (0 = no owner)
};

// Register/manage environment forces
IEnvironmentForceManagerV410 {
    HRESULT RegisterEnvironmentForce(IEnvironmentForceV400* pEnvironmentForce);
    HRESULT UnregisterEnvironmentForce(IEnvironmentForceV400* pEnvironmentForce);
    HRESULT GetForcesInRadius(const DXYZ& vWorldPos, float fRadiusFeet, 
                              IEnvironmentForceList& vecForces);
    HRESULT RegisterNotification(UINT uObjectId, float fRadiusFeet,
                                IEnvironmentForceCallbackV410* pfCallback);
};
```

---

## 5. TERRAIN/ELEVATION DATABASE INTEGRATION

### 5.1 Ground Information API (MSFS 2024)

```cpp
// Request ground information at specific location
SimConnect_RequestGroundInfo(hSimConnect,
    SIMCONNECT_DATA_REQUEST_ID RequestID,
    double minLat, double minLon, double minAlt,
    double maxLat, double maxLon, double maxAlt,
    DWORD dwGridWidth,                  // Grid points (e.g., 64)
    DWORD dwGridHeight,                 // Grid points
    SIMCONNECT_GROUND_INFO_LATLON_FORMAT eLatLonFormat,
    SIMCONNECT_GROUND_INFO_ALT_FORMAT eAltFormat,
    SIMCONNECT_GROUND_INFO_SOURCE_FLAG eSourceFlags);

// Format options
enum SIMCONNECT_GROUND_INFO_LATLON_FORMAT {
    SIMCONNECT_GROUND_INFO_LATLON_FORMAT_RADIANS,
    SIMCONNECT_GROUND_INFO_LATLON_FORMAT_DEGREES,
    SIMCONNECT_GROUND_INFO_LATLON_FORMAT_METERS,
};

enum SIMCONNECT_GROUND_INFO_ALT_FORMAT {
    SIMCONNECT_GROUND_INFO_ALT_FORMAT_METERS,
    SIMCONNECT_GROUND_INFO_ALT_FORMAT_FEET,
};

// Source flags
#define SIMCONNECT_GROUND_INFO_SOURCE_FLAG_TERRAIN        0x00010000
#define SIMCONNECT_GROUND_INFO_SOURCE_FLAG_PLATFORMS      0x00020000
#define SIMCONNECT_GROUND_INFO_SOURCE_FLAG_BATHYMETRY     0x00040000
#define SIMCONNECT_GROUND_INFO_SOURCE_FLAG_IGNORE_WAVES   0x00080000
#define SIMCONNECT_GROUND_INFO_SOURCE_FLAG_TERRAIN_AVG    0x00080000
```

### 5.2 Ground Information Response

```cpp
struct SIMCONNECT_DATA_GROUND_INFO {
    BOOL bIsValid;                      // Data point valid flag
    double fLat;                        // Latitude
    double fLon;                        // Longitude
    double fAlt;                        // Altitude
    double fNormalI;                    // Surface normal I component
    double fNormalJ;                    // Surface normal J component
    double fNormalK;                    // Surface normal K component
    DWORD eSurfaceType;                 // Surface type enum
    DWORD eSurfaceCondition;            // Surface condition enum
    BOOL bIsPlatform;                   // Is platform/object
    BOOL bIsPlatformMoving;             // Platform is dynamic
};

struct SIMCONNECT_RECV_GROUND_INFO {
    DWORD dwRequestID;
    DWORD dwGridWidth;                  // Grid dimensions
    DWORD dwGridHeight;
    DWORD dwArraySize;                  // Total points (width × height)
    DWORD dwFlags;
    BOOL bNotAllValid;                  // Some points invalid
    // Array of SIMCONNECT_DATA_GROUND_INFO follows
    SIMCONNECT_DATA_GROUND_INFO rgData[1];
};

// Surface Type Codes
enum SURFACE_TYPE {
    SURFACE_TYPE_WATER,                 // 0
    SURFACE_TYPE_CONCRETE,              // 1
    SURFACE_TYPE_ASPHALT,               // 2
    SURFACE_TYPE_GRASS,                 // 3
    SURFACE_TYPE_DIRT,                  // 4
    SURFACE_TYPE_GRAVEL,                // 5
    SURFACE_TYPE_SAND,                  // 6
    SURFACE_TYPE_SNOW,                  // 7
    SURFACE_TYPE_ICE,                   // 8
    // ... additional types
};
```

### 5.3 Mobile Scenery (MSFS 2024)

```cpp
// Request mobile scenery objects in area
SimConnect_RequestMobileSceneryInRadius(hSimConnect,
    SIMCONNECT_DATA_REQUEST_ID RequestID,
    DWORD dwRadiusMeters);

struct SIMCONNECT_DATA_MOBILE_SCENERY_INFO {
    char szMobileSceneryName[MAX_PATH];
    DWORD dwObjectID;
};

struct SIMCONNECT_RECV_MOBILE_SCENERY_DATA {
    DWORD dwRequestID;
    GUID guidInstanceID;                // Instance ID
    SIMCONNECT_DATA_LATLONALT llaWorldPosition;
    SIMCONNECT_DATA_PBH pbhWorldRotation;
    BOOL isOnGround;                    // Clamped to ground
    double speedKnots;                  // Movement speed
    float scale;                        // Object scale factor
    DWORD dwObjectID;
};
```

### 5.4 Prepar3D Terrain Integration

No specific terrain elevation API in Prepar3D v6 PDK beyond environment forces. Integration relies on:
1. Ground collision detection via `IEnvironmentForceManagerV410`
2. Weather system elevation queries
3. Standard SimVar terrain data

---

## 6. OFFICIAL ATC COMMAND DEFINITIONS AND RESPONSE SEQUENCES

### 6.1 ATC Communication Flow

**Standard Flight Phases & ATC Interactions:**

```
1. STARTUP PHASE
   ├─ Request engine start
   └─ Contact ground/clearance delivery

2. PUSHBACK PHASE
   ├─ Request pushback
   ├─ Clearance for pushback direction
   └─ Contact ground after pushback

3. TAXI PHASE
   ├─ Request taxi
   ├─ Receive taxi routing
   ├─ Contact tower
   └─ Continue taxi following clearance

4. TAKEOFF PHASE
   ├─ Request takeoff clearance
   ├─ Line up on runway
   ├─ Receive takeoff clearance
   └─ Depart airspace

5. DEPARTURE PHASE
   ├─ Contact departure control
   ├─ Report passing altitude
   └─ Follow departure routing

6. CRUISE PHASE
   ├─ Contact center/enroute control
   ├─ Report position/altitude
   └─ Receive cruise clearances

7. APPROACH PHASE
   ├─ Contact approach control
   ├─ Receive approach clearance
   ├─ Descend to pattern altitude
   └─ Request landing clearance

8. LANDING PHASE
   ├─ Receive landing clearance
   ├─ Line up on runway
   ├─ Complete landing
   └─ Contact ground for taxi

9. TAXI IN PHASE
   ├─ Receive taxi to gate
   ├─ Follow ground routing
   └─ Park at gate

10. SHUTDOWN PHASE
    ├─ Contact ground for parking info
    └─ Request engine shutdown
```

### 6.2 ATC Menu System

**MSFS 2024 / Prepar3D v6 Menu-Based Responses:**

The ATC system presents text-based menu options. Developer selects option and SimConnect transmits the selection:

**Common Menu Options:**

```
1. "Request Pushback"
2. "Request Taxi"
3. "Request Takeoff Clearance"
4. "Contact Approach"
5. "Contact Tower"
6. "Request Landing Clearance"
7. "Request VFR Flight Following"
8. "Request IFR Clearance"
9. "Report Position"
10. "Emergency Landing Request"
11. "Declare Emergency"
12. "Request Holding Pattern"
13. "Request Go-Around"
14. "Request Divert"
```

**No Direct ATC Command API Exists** - All ATC interactions flow through the menu system and facility subscription mechanisms.

### 6.3 ATC Frequency Data

```cpp
// VOR/NDB Navigation facility structure (includes ATC frequencies)
struct SIMCONNECT_DATA_FACILITY_VOR {
    DWORD Frequency;                    // In Hz (e.g., 118750 = 118.75 MHz)
    DWORD Flags;                        // Has NAV signal, localizer, DME, etc.
    float fLocalizer;                   // ILS localizer in degrees
    double GlideLat;                    // Glide slope position
    double GlideLon;
    double GlideAlt;
    float fGlideSlopeAngle;
};

// Get all frequencies for airport
SimConnect_RequestFacilitiesList(hSimConnect,
    SIMCONNECT_FACILITY_LIST_TYPE_WAYPOINT, RequestID);
// Filter results for SIMCONNECT_FACILITY_DATA_FREQUENCY types
```

### 6.4 Weather Observation Data (METAR)

**MSFS 2024 Weather Request:**

```cpp
// Get METAR at specific airport
SimConnect_WeatherRequestObservationAtStation(hSimConnect,
    SIMCONNECT_DATA_REQUEST_ID RequestID,
    const char* szICAO);                // e.g., "KJFK"

// Get METAR at nearest station
SimConnect_WeatherRequestObservationAtNearestStation(hSimConnect,
    SIMCONNECT_DATA_REQUEST_ID RequestID,
    float lat, float lon);

// Get interpolated METAR at position
SimConnect_WeatherRequestInterpolatedObservation(hSimConnect,
    SIMCONNECT_DATA_REQUEST_ID RequestID,
    float lat, float lon, float alt);

// Response Structure
struct SIMCONNECT_RECV_WEATHER_OBSERVATION {
    DWORD dwRequestID;
    char szMetar[1];                    // Variable-length string (max 2000 chars)
};

// METAR Format (standard aviation)
// Example: "KJFK 121251Z 30008KT 10SM FEW250 23/14 A3012 RMK AO2 SLP201"
// Decode:  Airport, UTC time, wind direction/speed, visibility, cloud layers,
//          temp/dew point, altimeter setting
```

**Prepar3D v6 METAR API:**

```cpp
// IWeatherSystemV500 methods
STDMETHOD(GetStationMetarData)(__in LPCSTR pszIcao, 
                               __out LPWSTR pszMetar, 
                               __in size_t cchMetar);

STDMETHOD(GetNearestStationMetarData)(__in double dLatRadians, 
                                      __in double dLonRadians, 
                                      __in double dAltFeet,
                                      __out LPWSTR pszMetar, 
                                      __in size_t cchMetar);

STDMETHOD(GetInterpolatedMetarData)(__in double dLatRadians, 
                                    __in double dLonRadians, 
                                    __in double dAltFeet,
                                    __out LPWSTR pszMetar, 
                                    __in size_t cchMetar);

// Set Custom METAR
STDMETHOD(SetMetarData)(__in LPCWSTR pszMetar, __in int nSeconds);
```

---

## 7. DEPRECATED VS. MODERN APIS

### 7.1 DEPRECATED (Avoid Using)

#### MSFS 2024 Deprecations:

```cpp
// DEPRECATED: Old ATC system (doesn't exist in 2024)
// DEPRECATED: RWW (Real World Weather) mode
SIMCONNECT_WEATHER_MODE_RWW          // Use SIMCONNECT_WEATHER_MODE_GLOBAL or THEME

// DEPRECATED: Legacy AI functions (use _EX1 versions)
SimConnect_AICreateParkedATCAircraft  // Use SimConnect_AICreateParkedATCAircraft_EX1
SimConnect_AICreateEnrouteATCAircraft // Use SimConnect_AICreateEnrouteATCAircraft_EX1
SimConnect_AICreateNonATCAircraft     // Use SimConnect_AICreateNonATCAircraft_EX1

// DEPRECATED: Old Menu System
SimConnect_MenuAddItem               // Character string limited
// Use newer versions for better support
```

#### Prepar3D v6 Deprecations:

```cpp
// DEPRECATED: Enhanced Atmospherics
GetEnhancedAtmospherics()            // DEPRECATED
SetEnhancedAtmospherics()            // DEPRECATED
// Use individual cloud/weather settings instead

// DEPRECATED: Legacy Weather Station interface
// Use IWeatherStationV430 minimum, IWeatherSystemV500 preferred
```

### 7.2 MODERN / RECOMMENDED

#### MSFS 2024 Modern APIs:

```cpp
// Extended AI Creation with Livery Support
SimConnect_AICreateParkedATCAircraft_EX1(hSimConnect,
    const char* szContainerTitle,
    const char* szLivery,              // NEW: Livery/paint selection
    const char* szTailNumber,
    const char* szAirportID,
    SIMCONNECT_DATA_REQUEST_ID RequestID);

// Extended Event Data (64-bit support)
SimConnect_TransmitClientEvent64(hSimConnect,
    SIMCONNECT_OBJECT_ID ObjectID,
    SIMCONNECT_CLIENT_EVENT_ID EventID,
    QWORD qwData,                      // 64-bit data
    SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
    SIMCONNECT_EVENT_FLAG Flags);

// Ground Information System
SimConnect_RequestGroundInfo(...)      // NEW: Terrain elevation queries

// Mobile Scenery
SimConnect_RequestMobileSceneryInRadius(...) // NEW: Dynamic objects

// Video Streaming
SimConnect_BeginVideoStream(...)       // NEW: Video output
SimConnect_EndVideoStream(...)

// Observer/Camera System
SimConnect_CreateObserver(...)         // NEW: Observer/spectator mode
SimConnect_SetObserverPosition(...)
SimConnect_RequestObserverData(...)

// Weapon/Countermeasure Support
SimConnect_AttachWeaponToObject(...)   // NEW: Military simulations
SimConnect_ReportWeaponDamage(...)
```

#### Prepar3D v6 Modern APIs:

```cpp
// IWeatherSystemV500 (Latest Version)
// Provides full weather control, cloud layers, temperature, pressure

// Environment Forces Management
IEnvironmentForceManagerV410         // Collision/force simulation
IEnvironmentForceV400                // Individual force definitions

// ISimObjectAI Callbacks
RegisterNotification for AI events   // Modern event subscription

// Mobile Scenery (MSFS 2024 feature ported to P3D)
Mobile scenery object support with position tracking
```

### 7.3 Cross-Platform Compatibility

**MSFS 2024 → Prepar3D v6 API Mapping:**

| MSFS 2024 | Prepar3D v6 | Notes |
|-----------|-------------|-------|
| SimConnect | ISimObject* | Both use SimConnect/PDK interfaces |
| Weather Mode | Weather Mode (compatible) | Theme/Custom/Global modes same |
| Facility Data | Facility Data (compatible) | Airport/Runway/Frequency structures compatible |
| ATC Menu | ATC Menu (compatible) | Menu-based system identical |
| AI Creation | AI Creation (compatible) | Aircraft creation APIs equivalent |
| Ground Info | Environment Forces | Different approach to collision/terrain |
| Observer Mode | Custom Camera | Prepar3D uses IControllableCamera |

---

## 8. IMPLEMENTATION RECOMMENDATIONS

### 8.1 Weather System Integration

**Recommended Implementation Order:**

1. **Level 1: Basic Weather**
   ```cpp
   // Get current weather at aircraft position
   SimConnect_WeatherRequestInterpolatedObservation(...)
   Parse METAR string for wind, temperature, visibility
   ```

2. **Level 2: Advanced Weather**
   ```cpp
   // Get cloud layers and turbulence
   Request facility data for weather info
   Use cloud state grid for detailed cloud analysis
   ```

3. **Level 3: Hazard Detection**
   ```cpp
   // Calculate icing risk (temp -20 to 0°C + visible moisture)
   // Detect wind shear (large wind direction change)
   // Identify severe turbulence (high wind + existing turbulence)
   ```

### 8.2 Autopilot Control Best Practices

1. **Enable Autopilot**
   ```cpp
   SimConnect_TransmitClientEvent(hSimConnect,
       SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
       EVENT_AUTOPILOT_MASTER,
       1,                              // 1 = ON
       GROUP_AUTOPILOT,
       SIMCONNECT_EVENT_FLAG_DEFAULT);
   ```

2. **Set Heading Hold**
   ```cpp
   // Set target heading first
   SimConnect_TransmitClientEvent(hSimConnect,
       SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
       EVENT_HEADING_BUG_SET,
       300,                            // Heading in degrees
       GROUP_AUTOPILOT,
       SIMCONNECT_EVENT_FLAG_DEFAULT);
   
   // Enable heading hold
   SimConnect_TransmitClientEvent(hSimConnect,
       SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
       EVENT_AUTOPILOT_HEADING_HOLD,
       1,
       GROUP_AUTOPILOT,
       SIMCONNECT_EVENT_FLAG_DEFAULT);
   ```

3. **Monitor Autopilot State**
   ```cpp
   // Request data on autopilot status
   SimConnect_RequestDataOnSimObject(hSimConnect,
       REQUEST_AUTOPILOT_STATE,
       DEFINITION_AUTOPILOT_STATE,
       SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
       SIMCONNECT_PERIOD_SIM_FRAME);
   ```

### 8.3 ATC Interaction Pattern

```cpp
// 1. Subscribe to ATC facilities
SimConnect_SubscribeToFacilities(hSimConnect,
    SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT,
    REQUEST_ATC_FACILITIES);

// 2. Request airport/frequency data
SimConnect_RequestFacilitiesList(hSimConnect,
    SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT,
    REQUEST_AIRPORT_LIST);

// 3. Parse received facilities and frequencies
// 4. Build context-aware ATC response
// 5. Select menu option via AI/ML scoring

// 6. Example: Approaching airport, request approach frequency
// Select "Contact Approach" from available menu options
```

### 8.4 Terrain Integration

**Use Ground Info API:**
```cpp
// Get elevation data in 64x64 grid
SimConnect_RequestGroundInfo(hSimConnect,
    REQUEST_GROUND_INFO,
    minLat, minLon, minAlt,
    maxLat, maxLon, maxAlt,
    64, 64,
    SIMCONNECT_GROUND_INFO_LATLON_FORMAT_DEGREES,
    SIMCONNECT_GROUND_INFO_ALT_FORMAT_FEET,
    SIMCONNECT_GROUND_INFO_SOURCE_FLAG_TERRAIN |
    SIMCONNECT_GROUND_INFO_SOURCE_FLAG_BATHYMETRY);

// Response provides elevation and surface normals for:
// - Terrain height maps
// - Landing surface analysis
// - Obstacle detection
// - Weather affects (mountains influence wind patterns)
```

---

## 9. SIMVAR DEFINITION EXAMPLES

### Complete Weather Data Definition

```cpp
// Define complete weather data structure
HRESULT DefineWeatherData(HANDLE hSimConnect) {
    HRESULT hr;
    
    // Wind Data
    hr = SimConnect_AddToDataDefinition(hSimConnect, 
        DEFINITION_WEATHER, "Wind Velocity X", "m/s", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Wind Velocity Y", "m/s", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Wind Velocity Z", "m/s", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Wind Direction", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Wind Gust Speed", "knots", SIMCONNECT_DATATYPE_FLOAT64);
    
    // Temperature & Pressure
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Ambient Temperature", "celsius", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Ambient Dew Point", "celsius", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Kohlsman Setting Hg", "inHg", SIMCONNECT_DATATYPE_FLOAT64);
    
    // Precipitation & Icing
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Precipitation State", "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Ambient Precip Rate", "in/hr", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Structural Ice Accretion", "0-1", SIMCONNECT_DATATYPE_FLOAT64);
    
    // Turbulence & Visibility
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Ambient Turbulence", "0-1", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Ambient Visibility", "meters", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Wind Shear Speed", "knots", SIMCONNECT_DATATYPE_FLOAT64);
    
    // Clouds
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Ambient Cloud Density", "0-1", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Ambient Cloud Base", "feet", SIMCONNECT_DATATYPE_FLOAT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect,
        DEFINITION_WEATHER, "Ambient Cloud Top", "feet", SIMCONNECT_DATATYPE_FLOAT64);
    
    return hr;
}
```

---

## 10. SUMMARY TABLE

| Category | MSFS 2024 | Prepar3D v6 | Status |
|----------|-----------|------------|--------|
| **SimVars** | 100+ available | Equivalent set | Current |
| **Weather API** | Global + stations | Weather system + stations | Modern |
| **ATC System** | Menu-based | Menu-based | Stable |
| **AI Traffic** | Full AI + EX1 functions | Full AI | Current |
| **Terrain Data** | Ground Info API | Environment forces | Different approach |
| **Autopilot** | 70+ events | Equivalent | Stable |
| **Lighting** | 20+ systems | Equivalent | Stable |
| **Facility Data** | Full support | Full support | Modern |
| **Collision** | Weapon/damage system | Environment forces | Different |
| **Jetway Support** | Native | Native | Modern (P3D v6) |

---

## References

- **MSFS 2024 SDK**: SimConnect.h (official header)
- **Prepar3D v6 SDK**: IWeatherSystem.h, IEnvironmentForces.h, PDK interface headers
- **Aviation Standards**: METAR format, ATC procedures, runway naming conventions
- **SimConnect Documentation**: Both simulators maintain similar documentation patterns

---

**Document Generated**: October 27, 2025  
**Analysis Based On**: MSFS 2024 SDK Headers + Prepar3D v6 PDK Headers  
**Status**: Complete and Current

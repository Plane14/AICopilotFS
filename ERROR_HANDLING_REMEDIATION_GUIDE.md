# AICopilotFS - Error Handling Remediation Guide

**Companion Document to ERROR_HANDLING_GAP_REPORT.md**  
**Provides implementation-ready code fixes and refactoring patterns**

---

## Part 1: Core Infrastructure

### 1.1 Global Configuration Manager

Create `aicopilot/include/config_manager.h`:

```cpp
#pragma once
#include <string>
#include <map>
#include <memory>

namespace AICopilot {

class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    bool loadConfig(const std::string& configPath);
    
    // Getters with validation
    double getDouble(const std::string& key, double defaultValue, 
                    double minValue, double maxValue);
    int getInt(const std::string& key, int defaultValue, 
              int minValue, int maxValue);
    bool getBool(const std::string& key, bool defaultValue);
    std::string getString(const std::string& key, const std::string& defaultValue);
    
    // Setters with validation
    void setDouble(const std::string& key, double value, 
                  double minValue, double maxValue);
    void setInt(const std::string& key, int value, 
               int minValue, int maxValue);
    
private:
    ConfigManager() = default;
    std::map<std::string, std::string> config_;
};

} // namespace AICopilot
```

### 1.2 Structured Logger

Create `aicopilot/include/logger.h`:

```cpp
#pragma once
#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>

namespace AICopilot {

enum class LogLevel { DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3, CRITICAL = 4 };

class Logger {
public:
    static Logger& getInstance();
    
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filePath);
    
    void log(LogLevel level, const std::string& component, const std::string& message);
    void logWithContext(LogLevel level, const std::string& component,
                       const std::string& message,
                       const std::map<std::string, std::string>& context);
    
    // Convenience methods
    void debug(const std::string& component, const std::string& message) {
        log(LogLevel::DEBUG, component, message);
    }
    void info(const std::string& component, const std::string& message) {
        log(LogLevel::INFO, component, message);
    }
    void warning(const std::string& component, const std::string& message) {
        log(LogLevel::WARNING, component, message);
    }
    void error(const std::string& component, const std::string& message) {
        log(LogLevel::ERROR, component, message);
    }
    void critical(const std::string& component, const std::string& message) {
        log(LogLevel::CRITICAL, component, message);
    }
    
    // Get last N messages for debugging
    std::vector<std::string> getLastMessages(size_t count = 100);
    
private:
    Logger() = default;
    LogLevel currentLevel_ = LogLevel::INFO;
    std::ofstream logFile_;
    std::mutex logMutex_;
    std::vector<std::string> messageBuffer_;
    static constexpr size_t MAX_BUFFER_SIZE = 1000;
    
    std::string getTimestamp();
    std::string getLevelString(LogLevel level);
    void writeLog(const std::string& message);
};

} // namespace AICopilot
```

### 1.3 Error Codes and Exceptions

Create `aicopilot/include/error_codes.h`:

```cpp
#pragma once
#include <exception>
#include <string>

namespace AICopilot {

enum class ErrorCode {
    // SimConnect errors
    SIM_CONNECTION_FAILED = 1001,
    SIM_DISCONNECTED = 1002,
    SIM_INVALID_DATA = 1003,
    SIM_TIMEOUT = 1004,
    
    // Navigation errors
    NAV_INVALID_COORDINATES = 2001,
    NAV_INVALID_FLIGHT_PLAN = 2002,
    NAV_ANTIMERIDIAN_CROSSING = 2003,
    NAV_POLE_CROSSING = 2004,
    
    // Terrain errors
    TERRAIN_DATABASE_NOT_LOADED = 3001,
    TERRAIN_COLLISION = 3002,
    TERRAIN_INVALID_ELEVATION = 3003,
    
    // Weather errors
    WEATHER_INVALID_CONDITIONS = 4001,
    WEATHER_HAZARD_DETECTED = 4002,
    
    // Traffic errors
    TRAFFIC_CONFLICT = 5001,
    TRAFFIC_COLLISION_IMMINENT = 5002,
    
    // Configuration errors
    CONFIG_FILE_NOT_FOUND = 6001,
    CONFIG_INVALID_VALUE = 6002,
    CONFIG_MISSING_REQUIRED_KEY = 6003,
};

class AICopilotException : public std::exception {
public:
    AICopilotException(ErrorCode code, const std::string& message, 
                       const std::string& component = "UNKNOWN")
        : code_(code), message_(message), component_(component) {}
    
    virtual const char* what() const noexcept override {
        return message_.c_str();
    }
    
    ErrorCode getCode() const { return code_; }
    const std::string& getComponent() const { return component_; }
    
private:
    ErrorCode code_;
    std::string message_;
    std::string component_;
};

} // namespace AICopilot
```

---

## Part 2: SimConnect Wrapper Fixes

### 2.1 Connection State Machine

Add to `simconnect_wrapper.cpp`:

```cpp
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    ERROR
};

class SimConnectWrapper::Impl {
public:
    // ... existing code ...
    
    ConnectionState connectionState = ConnectionState::DISCONNECTED;
    std::chrono::steady_clock::time_point lastMessageTime;
    int reconnectionAttempts = 0;
    static constexpr int MAX_RECONNECTION_ATTEMPTS = 3;
    static constexpr int DISCONNECTION_TIMEOUT_MS = 5000;
};

// Enhanced connection monitoring
bool SimConnectWrapper::monitorConnection() {
    if (pImpl->connectionState == ConnectionState::DISCONNECTED) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastMessage = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - pImpl->lastMessageTime).count();
    
    if (timeSinceLastMessage > DISCONNECTION_TIMEOUT_MS) {
        // Connection timeout detected
        Logger::getInstance().warning("SIMCONNECT", 
            "Connection timeout after " + std::to_string(timeSinceLastMessage) + "ms");
        
        pImpl->connectionState = ConnectionState::ERROR;
        pImpl->connected = false;
        return false;
    }
    
    return true;
}
```

### 2.2 Data Validation Before Processing

Add to `simconnect_wrapper.cpp`:

```cpp
bool SimConnectWrapper::Impl::validateAircraftState(const SimConnectAircraftState& state) {
    auto& config = ConfigManager::getInstance();
    
    // Validate latitude
    double maxLat = config.getDouble("SIMCONNECT:max_latitude", 90.0, -90.0, 90.0);
    double minLat = config.getDouble("SIMCONNECT:min_latitude", -90.0, -90.0, 90.0);
    if (state.latitude < minLat || state.latitude > maxLat) {
        Logger::getInstance().warning("SIMCONNECT", 
            "Invalid latitude: " + std::to_string(state.latitude));
        return false;
    }
    
    // Validate longitude - detect antimeridian issues
    if (state.longitude < -180.0 || state.longitude > 180.0) {
        Logger::getInstance().warning("SIMCONNECT",
            "Invalid longitude: " + std::to_string(state.longitude));
        return false;
    }
    
    // Validate altitude
    double maxAlt = config.getDouble("SIMCONNECT:max_altitude_ft", 65000.0, 0.0, 100000.0);
    double minAlt = config.getDouble("SIMCONNECT:min_altitude_ft", -1000.0, -10000.0, 0.0);
    if (state.altitude < minAlt || state.altitude > maxAlt) {
        Logger::getInstance().warning("SIMCONNECT",
            "Invalid altitude: " + std::to_string(state.altitude) + " ft");
        return false;
    }
    
    // Validate electrical system
    if (state.batteryVoltage < 0.0) {
        Logger::getInstance().error("SIMCONNECT",
            "Negative battery voltage: " + std::to_string(state.batteryVoltage) + "V");
        return false;
    }
    
    return true;
}

void SimConnectWrapper::Impl::updateAircraftState(const SimConnectAircraftState& scState) {
    if (!validateAircraftState(scState)) {
        Logger::getInstance().error("SIMCONNECT", "Aircraft state validation failed");
        // Could implement recovery or use previous valid state
        return;
    }
    
    // ... rest of update logic ...
}
```

### 2.3 Coordinate Wrapping at Antimeridian

Add to `simconnect_wrapper.cpp`:

```cpp
Position SimConnectWrapper::Impl::normalizePosition(const Position& pos) {
    Position normalized = pos;
    
    // Normalize latitude to [-90, 90]
    if (normalized.latitude > 90.0 || normalized.latitude < -90.0) {
        Logger::getInstance().warning("SIMCONNECT",
            "Latitude out of range: " + std::to_string(normalized.latitude));
        normalized.latitude = std::max(-90.0, std::min(90.0, normalized.latitude));
    }
    
    // Normalize longitude to [-180, 180]
    while (normalized.longitude > 180.0) {
        normalized.longitude -= 360.0;
    }
    while (normalized.longitude < -180.0) {
        normalized.longitude += 360.0;
    }
    
    return normalized;
}

// Detect position jumps (teleports)
bool SimConnectWrapper::Impl::detectTeleport(const Position& oldPos, const Position& newPos) {
    auto& config = ConfigManager::getInstance();
    double threshold = config.getDouble("SIMCONNECT:teleport_threshold_nm", 10.0, 1.0, 100.0);
    
    // Simple distance calculation (for demo; use great circle in production)
    double latDiff = newPos.latitude - oldPos.latitude;
    double lonDiff = newPos.longitude - oldPos.longitude;
    double simpleDistance = std::sqrt(latDiff * latDiff + lonDiff * lonDiff) * 69.0;  // rough NM
    
    if (simpleDistance > threshold) {
        Logger::getInstance().warning("SIMCONNECT",
            "Position jump detected: " + std::to_string(simpleDistance) + " NM");
        return true;
    }
    
    return false;
}
```

---

## Part 3: Navigation Fixes

### 3.1 Antimeridian-Safe Calculations

Add to `navigation.cpp`:

```cpp
// Helper to normalize longitude difference
static double normalizeLongitudeDifference(double lon1, double lon2) {
    double dLon = lon2 - lon1;
    
    // Use shortest path across antimeridian
    if (dLon > 180.0) {
        dLon -= 360.0;
    } else if (dLon < -180.0) {
        dLon += 360.0;
    }
    
    return dLon;
}

double Navigation::greatCircleDistance(const Position& p1, const Position& p2) const {
    // Validate inputs
    if (p1.latitude < -90.0 || p1.latitude > 90.0 ||
        p2.latitude < -90.0 || p2.latitude > 90.0) {
        Logger::getInstance().error("NAVIGATION", "Invalid latitude in distance calculation");
        throw AICopilotException(ErrorCode::NAV_INVALID_COORDINATES,
                                "Invalid latitude", "NAVIGATION");
    }
    
    double lat1 = p1.latitude * PI / 180.0;
    double lat2 = p2.latitude * PI / 180.0;
    double dLat = (p2.latitude - p1.latitude) * PI / 180.0;
    
    // CRITICAL: Use normalized longitude difference
    double dLon = normalizeLongitudeDifference(p1.longitude, p2.longitude) * PI / 180.0;
    
    double a = sin(dLat / 2.0) * sin(dLat / 2.0) +
               cos(lat1) * cos(lat2) *
               sin(dLon / 2.0) * sin(dLon / 2.0);
    
    // Guard against NaN from sqrt of negative
    a = std::max(0.0, std::min(1.0, a));
    
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    
    return EARTH_RADIUS_NM * c;
}

double Navigation::greatCircleBearing(const Position& p1, const Position& p2) const {
    double lat1 = p1.latitude * PI / 180.0;
    double lat2 = p2.latitude * PI / 180.0;
    double dLon = normalizeLongitudeDifference(p1.longitude, p2.longitude) * PI / 180.0;
    
    double y = sin(dLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
    
    double bearing = atan2(y, x) * 180.0 / PI;
    
    // Normalize to 0-360
    bearing = fmod(bearing + 360.0, 360.0);
    return bearing;
}
```

### 3.2 Flight Plan Validation

Add to `navigation.cpp`:

```cpp
bool Navigation::validateFlightPlan() const {
    if (flightPlan_.waypoints.size() < 2) {
        Logger::getInstance().error("NAVIGATION", "Flight plan has < 2 waypoints");
        throw AICopilotException(ErrorCode::NAV_INVALID_FLIGHT_PLAN,
                                "Flight plan must have at least 2 waypoints", "NAVIGATION");
    }
    
    auto& config = ConfigManager::getInstance();
    double minLegNM = config.getDouble("NAVIGATION:min_leg_distance_nm", 0.1, 0.01, 1.0);
    double maxLegNM = config.getDouble("NAVIGATION:max_leg_distance_nm", 2000.0, 100.0, 10000.0);
    
    for (size_t i = 0; i < flightPlan_.waypoints.size(); ++i) {
        const auto& wp = flightPlan_.waypoints[i];
        
        // Validate coordinates
        if (wp.position.latitude < -90.0 || wp.position.latitude > 90.0) {
            Logger::getInstance().error("NAVIGATION",
                "Waypoint " + std::to_string(i) + " invalid latitude: " + 
                std::to_string(wp.position.latitude));
            throw AICopilotException(ErrorCode::NAV_INVALID_COORDINATES,
                                    "Invalid waypoint latitude", "NAVIGATION");
        }
        
        if (wp.position.altitude < -1000.0 || wp.position.altitude > 60000.0) {
            Logger::getInstance().warning("NAVIGATION",
                "Waypoint " + std::to_string(i) + " altitude out of typical range");
        }
        
        // Check leg distances
        if (i > 0) {
            double distance = greatCircleDistance(
                flightPlan_.waypoints[i-1].position,
                wp.position);
            
            if (distance < minLegNM || distance > maxLegNM) {
                Logger::getInstance().warning("NAVIGATION",
                    "Leg " + std::to_string(i) + " distance " + std::to_string(distance) +
                    " NM outside typical range");
            }
            
            // Detect heading reversals
            double bearing1 = (i > 1) ? 
                greatCircleBearing(flightPlan_.waypoints[i-2].position,
                                  flightPlan_.waypoints[i-1].position) : 0.0;
            double bearing2 = greatCircleBearing(flightPlan_.waypoints[i-1].position,
                                               wp.position);
            
            double headingChange = std::abs(bearing2 - bearing1);
            if (headingChange > 180.0) {
                headingChange = 360.0 - headingChange;
            }
            
            if (headingChange > 150.0) {
                Logger::getInstance().warning("NAVIGATION",
                    "Leg " + std::to_string(i) + " heading change: " +
                    std::to_string(headingChange) + "°");
            }
        }
    }
    
    return true;
}
```

### 3.3 Cross-Track Error with Protection

Add to `navigation.cpp`:

```cpp
double Navigation::crossTrackError(const Position& current) const {
    if (flightPlan_.waypoints.size() < 2 || activeWaypointIndex_ == 0) {
        return 0.0;
    }
    
    if (activeWaypointIndex_ >= flightPlan_.waypoints.size()) {
        return 0.0;
    }
    
    Waypoint prev = flightPlan_.waypoints[activeWaypointIndex_ - 1];
    Waypoint next = getActiveWaypoint();
    
    double d13 = greatCircleDistance(prev.position, current);
    
    if (d13 < 0.01) {  // Less than 10 meters
        return 0.0;
    }
    
    double brg13 = greatCircleBearing(prev.position, current);
    double brg12 = greatCircleBearing(prev.position, next.position);
    
    double angularDiff = brg13 - brg12;
    
    // Normalize angle to [-180, 180]
    while (angularDiff > 180.0) angularDiff -= 360.0;
    while (angularDiff < -180.0) angularDiff += 360.0;
    
    double sinValue = sin(d13 / EARTH_RADIUS_NM) * sin(angularDiff * PI / 180.0);
    
    // CRITICAL: Clamp sinValue to [-1, 1] to prevent NaN from asin
    sinValue = std::max(-1.0, std::min(1.0, sinValue));
    
    double xte = asin(sinValue) * EARTH_RADIUS_NM;
    
    return xte;
}
```

---

## Part 4: Terrain Awareness Fixes

### 4.1 Basic Terrain Database Implementation

Add to `terrain_awareness.cpp`:

```cpp
struct TerrainGridPoint {
    double latitude;
    double longitude;
    double elevation;
};

bool TerrainAwareness::loadTerrainDatabase(const std::string& databasePath) {
    Logger::getInstance().info("TERRAIN", "Loading terrain database from: " + databasePath);
    
    try {
        std::ifstream file(databasePath, std::ios::binary);
        if (!file.is_open()) {
            Logger::getInstance().error("TERRAIN",
                "Failed to open terrain database file: " + databasePath);
            return false;
        }
        
        terrainDatabase_.clear();
        
        // Read grid points (format: lat, lon, elevation)
        TerrainGridPoint point;
        while (file.read((char*)&point, sizeof(TerrainGridPoint))) {
            // Validate point
            if (point.latitude < -90.0 || point.latitude > 90.0 ||
                point.longitude < -180.0 || point.longitude > 180.0 ||
                point.elevation < -1000.0 || point.elevation > 30000.0) {
                Logger::getInstance().warning("TERRAIN",
                    "Skipping invalid terrain point");
                continue;
            }
            terrainDatabase_.push_back(point);
        }
        
        file.close();
        
        Logger::getInstance().info("TERRAIN",
            "Loaded " + std::to_string(terrainDatabase_.size()) + " terrain points");
        
        return !terrainDatabase_.empty();
    } catch (const std::exception& e) {
        Logger::getInstance().error("TERRAIN",
            "Exception loading terrain database: " + std::string(e.what()));
        return false;
    }
}

double TerrainAwareness::getTerrainElevation(const Position& pos) const {
    auto& config = ConfigManager::getInstance();
    double defaultElevation = config.getDouble("TERRAIN:default_elevation_ft", 0.0, -1000.0, 10000.0);
    
    if (terrainDatabase_.empty()) {
        Logger::getInstance().debug("TERRAIN",
            "Terrain database empty, using default elevation: " + std::to_string(defaultElevation));
        return defaultElevation;
    }
    
    // Find nearest terrain points (simple linear search; use spatial index in production)
    double minDistance = std::numeric_limits<double>::max();
    double elevation = defaultElevation;
    
    for (const auto& point : terrainDatabase_) {
        double latDiff = point.latitude - pos.latitude;
        double lonDiff = point.longitude - pos.longitude;
        double distance = latDiff * latDiff + lonDiff * lonDiff;  // Squared distance
        
        if (distance < minDistance) {
            minDistance = distance;
            elevation = point.elevation;
        }
    }
    
    return elevation;
}
```

### 4.2 Terrain Alert Generation with Validation

Add to `terrain_awareness.cpp`:

```cpp
TerrainAlert TerrainAwareness::checkTerrainClearance(const Position& pos) const {
    TerrainAlert alert;
    
    // Validate input position
    if (pos.latitude < -90.0 || pos.latitude > 90.0) {
        Logger::getInstance().error("TERRAIN", "Invalid latitude in terrain check");
        alert.level = TerrainWarningLevel::WARNING;
        alert.message = "TERRAIN: INVALID POSITION";
        return alert;
    }
    
    double elevation = getTerrainElevation(pos);
    
    // Validate elevation
    if (elevation < -1000.0 || elevation > 30000.0 || std::isnan(elevation)) {
        Logger::getInstance().warning("TERRAIN",
            "Invalid terrain elevation: " + std::to_string(elevation));
        alert.level = TerrainWarningLevel::WARNING;
        alert.message = "TERRAIN: ELEVATION DATA INVALID";
        return alert;
    }
    
    // Calculate AGL (Above Ground Level)
    double agl = pos.altitude - elevation;
    
    // Clamp clearance to reasonable values
    if (agl < 0.0) {
        Logger::getInstance().critical("TERRAIN",
            "Aircraft below terrain! AGL: " + std::to_string(agl) + " ft");
        alert.level = TerrainWarningLevel::PULL_UP;
        alert.message = "TERRAIN TERRAIN TERRAIN - PULL UP";
        alert.clearance = 0.0;
        return alert;
    }
    
    bool climbing = currentState_.verticalSpeed > 0;
    alert.level = determineWarningLevel(agl, climbing);
    alert.position = pos;
    alert.clearance = agl;
    alert.requiredAltitude = elevation + 
        ConfigManager::getInstance().getDouble("TERRAIN:min_clearance_ft", 1000.0, 500.0, 5000.0);
    
    return alert;
}
```

---

## Part 5: Weather System Fixes

### 5.1 Comprehensive Hazard Detection

Add to `weather_system.cpp`:

```cpp
void WeatherSystem::updateWeatherConditions(const WeatherConditions& conditions) {
    // Validate input conditions
    auto& config = ConfigManager::getInstance();
    
    if (conditions.visibility < 0.0 || conditions.visibility > 10.0) {
        Logger::getInstance().warning("WEATHER",
            "Invalid visibility: " + std::to_string(conditions.visibility) + " SM");
    }
    
    if (conditions.windSpeed < 0.0 || conditions.windSpeed > 100.0) {
        Logger::getInstance().warning("WEATHER",
            "Invalid wind speed: " + std::to_string(conditions.windSpeed) + " knots");
    }
    
    if (conditions.temperature < -70.0 || conditions.temperature > 60.0) {
        Logger::getInstance().warning("WEATHER",
            "Invalid temperature: " + std::to_string(conditions.temperature) + "°C");
    }
    
    currentWeather_ = conditions;
    activeHazards_.clear();
    
    // Detect thunderstorms
    if (conditions.precipitation && conditions.cloudBase < 5000.0 && conditions.turbulence) {
        WeatherHazard hazard;
        hazard.type = WeatherHazardType::THUNDERSTORM;
        hazard.severity = HazardSeverity::SEVERE;
        hazard.description = "Thunderstorm activity detected";
        activeHazards_.push_back(hazard);
        Logger::getInstance().warning("WEATHER", "Thunderstorm hazard detected");
    }
    
    // Detect icing conditions
    if (conditions.icing) {
        WeatherHazard hazard;
        hazard.type = WeatherHazardType::SEVERE_ICING;
        hazard.severity = HazardSeverity::SEVERE;
        hazard.description = "Icing conditions detected";
        activeHazards_.push_back(hazard);
        Logger::getInstance().warning("WEATHER", "Icing hazard detected");
    }
    
    // Detect severe turbulence
    if (conditions.turbulence && conditions.windSpeed > 30.0) {
        WeatherHazard hazard;
        hazard.type = WeatherHazardType::SEVERE_TURBULENCE;
        hazard.severity = HazardSeverity::SEVERE;
        hazard.description = "Severe turbulence detected";
        activeHazards_.push_back(hazard);
        Logger::getInstance().warning("WEATHER", "Severe turbulence hazard detected");
    }
    
    // Detect crosswind hazard (for landing/takeoff)
    double maxCrosswind = config.getDouble("WEATHER:max_crosswind_transport_kt", 30.0, 15.0, 50.0);
    if (conditions.windSpeed > maxCrosswind && conditions.windDirection != 0.0) {
        // Simple crosswind estimation (in production, calculate actual crosswind component)
        double estimatedCrosswind = conditions.windSpeed * 0.707;  // ~45° average
        if (estimatedCrosswind > maxCrosswind) {
            WeatherHazard hazard;
            hazard.type = WeatherHazardType::CROSSWIND;
            hazard.severity = HazardSeverity::MODERATE;
            hazard.description = "Crosswind exceeds aircraft limit";
            activeHazards_.push_back(hazard);
            Logger::getInstance().warning("WEATHER", "Crosswind hazard detected");
        }
    }
    
    // Detect windshear potential (high winds with low cloud base)
    if (conditions.windSpeed > 25.0 && conditions.cloudBase < 2000.0) {
        WeatherHazard hazard;
        hazard.type = WeatherHazardType::WINDSHEAR;
        hazard.severity = HazardSeverity::MODERATE;
        hazard.description = "Windshear potential detected";
        activeHazards_.push_back(hazard);
        Logger::getInstance().warning("WEATHER", "Windshear hazard detected");
    }
}
```

---

## Part 6: Configuration File Template

Create `config/aicopilot_defaults.ini`:

```ini
# AICopilot Configuration File
# Comprehensive configuration for all error handling parameters

[SIMULATOR]
# SimConnect connection settings
connection_timeout_ms=5000
message_retry_attempts=3
data_request_interval_ms=100
max_reconnection_attempts=3
disconnection_timeout_ms=5000

# Position validation ranges
[SIMCONNECT]
max_latitude=90.0
min_latitude=-90.0
max_longitude=180.0
min_longitude=-180.0
max_altitude_ft=65000.0
min_altitude_ft=-1000.0
max_battery_voltage=50.0
min_battery_voltage=0.0
max_airspeed_knots=600.0
max_vertical_speed_fpm=10000.0
teleport_threshold_nm=10.0

# Navigation settings
[NAVIGATION]
min_ground_speed_knots=0.0
max_ground_speed_knots=600.0
min_leg_distance_nm=0.1
max_leg_distance_nm=2000.0
waypoint_tolerance_nm=1.0
antimeridian_buffer_degrees=5
course_reversal_threshold_deg=150.0
max_heading_change_deg=90.0

# Terrain awareness
[TERRAIN]
min_clearance_pull_up_ft=300.0
min_clearance_warning_ft=500.0
min_clearance_caution_ft=1000.0
default_elevation_ft=0.0
max_terrain_elevation_ft=30000.0
terrain_database_path=./data/terrain.dem
terrain_update_interval_sec=5.0

# Weather detection
[WEATHER]
min_visibility_sm=0.0
max_visibility_sm=10.0
min_cloud_base_ft=0.0
max_cloud_base_ft=50000.0
min_temperature_c=-70.0
max_temperature_c=60.0
min_wind_speed_kt=0.0
max_wind_speed_kt=100.0
max_crosswind_light_aircraft_kt=20.0
max_crosswind_transport_kt=30.0
radar_intensity_min=0.0
radar_intensity_max=100.0
icing_detection_enabled=true
windshear_detection_enabled=true

# Logging configuration
[LOGGING]
log_level=INFO
log_file=./logs/aicopilot.log
log_max_file_size_mb=100
log_max_backup_files=10
enable_structured_logging=true
enable_performance_metrics=true
enable_console_output=true

# Aircraft performance
[AIRCRAFT]
min_speed_knots=30.0
max_speed_knots=250.0
max_climb_rate_fpm=2000.0
max_descent_rate_fpm=2000.0
max_bank_angle_deg=25.0
```

---

## Implementation Checklist

- [ ] Create ConfigManager class
- [ ] Create Logger class  
- [ ] Create error code enums and exceptions
- [ ] Implement SimConnect connection monitoring
- [ ] Add data validation before processing
- [ ] Implement antimeridian-safe calculations
- [ ] Add flight plan validation
- [ ] Implement terrain database loading
- [ ] Add comprehensive hazard detection
- [ ] Create configuration file
- [ ] Add structured logging throughout
- [ ] Implement cross-component error propagation
- [ ] Add performance monitoring
- [ ] Create unit tests for edge cases
- [ ] Integration testing with actual SimConnect


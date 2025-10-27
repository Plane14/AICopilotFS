# Database Integration - Quick Reference Guide

**Purpose:** Fast lookup for specific database integration tasks  
**Date:** October 27, 2025

---

## 🗂️ File Location Reference

### Terrain System Files
```
Header:    aicopilot/include/terrain_awareness.h (124 lines)
Implementation: aicopilot/src/terrain/terrain_awareness.cpp (250+ lines)
Stub Method: lines 170-189 (loadTerrainDatabase)
Integration Point: lines 109, 238 (getTerrainElevation, interpolateElevation)
```

### Weather System Files
```
Header:    aicopilot/include/weather_system.h (100 lines)
Implementation: aicopilot/src/weather/weather_system.cpp (150+ lines)
Current Data: Manual WeatherConditions input only
Integration Point: updateWeatherConditions() (line 13-50)
Missing: METAR parser, API integration
```

### Runway System Files
```
Header:    aicopilot/include/approach_system.h (120 lines)
Implementation: aicopilot/src/approach/approach_system.cpp (140+ lines)
Airport Data: aicopilot/include/airport_data.hpp (543 lines)
Navdata Provider: aicopilot/src/navdata/navdata_providers.cpp (431 lines)
Stub Method: lines 405-410 (getAirportLayout returns empty)
```

### Navigation System Files
```
Header:    aicopilot/include/navdata_provider.h (complete)
Implementation: aicopilot/src/navdata/navdata_providers.cpp (431 lines)
Cached Navaids: lines 268-285 (only 5 VORs hardcoded)
Cached Airports: lines 250-280 (25 airports only)
```

---

## 🔍 Code Snippets to Find

### Find Terrain Stub
```cpp
// Location: terrain_awareness.cpp line 170-189
// Search for:
bool TerrainAwareness::loadTerrainDatabase(const std::string& databasePath)
```

### Find Weather Update Method
```cpp
// Location: weather_system.cpp line 13-50
// Search for:
void WeatherSystem::updateWeatherConditions(const WeatherConditions& conditions)
```

### Find Runway Layout (Empty Stub)
```cpp
// Location: navdata_providers.cpp line 405-410
// Search for:
bool CachedNavdataProvider::getAirportLayout(const std::string& icao, AirportLayout& layout)
// Returns: empty layout with comment "stub implementation"
```

### Find Navaid Cache Initialization
```cpp
// Location: navdata_providers.cpp line 268-285
// Search for:
void loadDefaultNavaids() {
    addNavaid("LAX", ...);  // Only 5 navaids
}
```

---

## 📥 Data Download Commands

### Download SRTM Tiles (Example: New York)

```bash
# Using wget (SRTM from USGS mirror)
wget https://e4ftl01.cr.usgs.gov/SRTM_GL1/SRTM_GL1_srtm/N40W074.SRTM_GL1.4326.v003.h5
# or HGT format:
wget https://srtm.csi.cgiar.org/wp-content/uploads/files/Raster/srtm/tif/N40W074.zip

# Or using Python with gdal tools
gdal_translate -of HGT /vsicurl/https://srtm.csi.cgiar.org/N40W074.tif N40W074.hgt
```

### Download FAA NavAID Data

```bash
# From FAA NFDC FTP
wget ftp://ftp.fcm.nesdis.noaa.gov/nesdis/aviation/navaids.txt

# Alternative from X-Plane repositories
curl -O https://data.x-plane.com/nav.txt
```

### Get METAR for Specific Airport

```bash
# Single airport METAR
curl "https://aviationweather.gov/adds/dataserver?request=retrieve&dataSource=metars&stationString=KJFK&hoursBeforeNow=3&format=csv"

# Multiple airports
curl "https://aviationweather.gov/adds/dataserver?request=retrieve&dataSource=metars&stationString=KJFK,KLAX,KORD&hoursBeforeNow=1&format=csv"
```

---

## 💾 Data Storage Structure

### Terrain Directory Structure
```
data/
  srtm/
    N40W074.hgt      (1.4 MB, 1201x1201 samples)
    N40W073.hgt
    N41W074.hgt
    N41W073.hgt
    ... (400+ tiles for US coverage)
```

### Weather Cache Structure
```
data/
  weather/
    metar_cache.db   (SQLite)
      TABLE metars:
        stationID, observationTime, temperature, windSpeed, ...
    taf_cache.db     (SQLite)
      TABLE tafs:
        stationID, forecastTime, conditions, ...
```

### Navigation Database Structure
```
data/
  navdata/
    navaids.db       (SQLite or binary)
      TABLE vors:
        id, name, latitude, longitude, frequency, ...
      TABLE ndbs:
        id, name, latitude, longitude, frequency, ...
      TABLE waypoints:
        id, name, latitude, longitude, usage, ...
    airports.db
      TABLE airports:
        icao, name, elevation, latitude, longitude, ...
      TABLE runways:
        airport_icao, runway_id, length, width, heading, ...
      TABLE procedures:
        airport_icao, procedure_name, type, segments, ...
```

---

## 🧪 Testing Checklist

### Terrain Database Testing
- [ ] Load single HGT tile
- [ ] Verify 1201x1201 grid reading
- [ ] Test bilinear interpolation
- [ ] Compare elevation values against known locations
- [ ] Benchmark tile loading time
- [ ] Test LRU cache eviction

### Weather Database Testing
- [ ] Parse METAR string format
- [ ] Verify wind component extraction
- [ ] Test cloud layer parsing
- [ ] Validate temperature/dewpoint
- [ ] Test API response parsing
- [ ] Verify cache expiration (24 hours)

### Runway Database Testing
- [ ] Load CIFP procedure file
- [ ] Parse runway threshold data
- [ ] Verify ILS parameters
- [ ] Test approach procedure sequencing
- [ ] Compare with known procedures

### Navigation Database Testing
- [ ] Load FAA navaid file
- [ ] Parse coordinate format (DMS to decimal)
- [ ] Verify frequency values
- [ ] Test spatial queries (navaids within radius)
- [ ] Benchmark 50k navaid searches

---

## 🚨 Common Errors & Solutions

### HGT File Errors

| Error | Cause | Solution |
|-------|-------|----------|
| "File size mismatch" | Not 1,442,401 bytes | Verify download completeness |
| "Invalid coordinates" | Wrong filename format | Use format: N##W###.hgt |
| "All elevations -32768" | Water/void data | Expected over oceans, use fallback |
| "Slow tile loading" | No caching | Implement LRU cache (16 tiles) |

### METAR Parse Errors

| Error | Cause | Solution |
|-------|-------|----------|
| "Missing station ID" | Malformed METAR | Check source API |
| "Invalid wind direction" | 360° instead of 0° | Normalize: 360 → 0 |
| "Temperature NaN" | Negative temp parsing | Parse 'M' prefix correctly |
| "No cloud layers" | SKC format | Handle "Sky Clear" variant |

### Database Query Errors

| Error | Cause | Solution |
|-------|-------|----------|
| "No navaids found" | Database empty | Verify file loading |
| "Slow radius search" | Linear scan (O(n)) | Add spatial index (R-tree) |
| "Memory exhaustion" | Too many tiles cached | Implement tile LRU (16 max) |
| "Stale data" | No update mechanism | Add AIRAC cycle checker |

---

## 📊 Performance Targets

### Terrain System
- **Tile load time:** < 500 ms per tile
- **Elevation query:** < 1 ms (cached)
- **Interpolation accuracy:** ±30 meters
- **Memory per tile:** 1.4 MB
- **Max cached tiles:** 16 tiles = 22.4 MB

### Weather System
- **METAR parse time:** < 10 ms
- **API response time:** < 2 seconds
- **Cache update interval:** Every 30 minutes
- **Cache size:** < 10 MB (100+ METAR records)

### Navigation System
- **Navaid load time:** < 5 seconds (5500 records)
- **Radius search:** < 50 ms (with spatial index)
- **Memory footprint:** ~50 MB for all data
- **Startup time:** < 10 seconds

### Runway System
- **CIFP parse time:** < 100 ms per airport
- **Layout load time:** < 50 ms
- **Procedure sequencing:** Real-time

---

## 🔗 API Integration Points

### Aviation Weather Center (NOAA)

```
Endpoint: https://aviationweather.gov/adds/dataserver
Request:  ?request=retrieve&dataSource=metars&stationString=KJFK&hoursBeforeNow=3&format=csv
Rate Limit: 200 requests/hour (unauthenticated)
Update Frequency: METAR every 30-60 minutes
Response Format: CSV or XML
```

### FAA CIFP Data

```
FTP: ftp://ftp.fcm.nesdis.noaa.gov/nesdis/aviation/
Files: CIFP data (~20 MB compressed per cycle)
Update Cycle: Every 28 days (AIRAC)
License: Public Domain (US Government)
```

### SRTM Elevation Data

```
Primary: https://e4ftl01.cr.usgs.gov/SRTM_GL1/SRTM_GL1_srtm/
Mirror: https://srtm.csi.cgiar.org/
Format: HGT (binary) or GeoTIFF
Tile Size: ~1.4 MB (HGT), ~3 MB (GeoTIFF)
Coverage: 60°N to 54°S
Resolution: 1 arcsecond (~30 meters)
```

---

## 🔧 Build Integration

### CMakeLists.txt Changes Needed

```cmake
# Add to find_package section
find_package(GDAL REQUIRED)  # For elevation data
find_package(CURL REQUIRED)   # For HTTP APIs
find_package(SQLite3 REQUIRED) # For caching

# Link libraries
target_link_libraries(aicopilot_lib
    PRIVATE GDAL::GDAL
    PRIVATE CURL::CURL
    PRIVATE SQLite3::SQLite3
)

# Data directory
set(AICOPILOT_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/aicopilot/data")
add_definitions(-DAICOPILOT_DATA_DIR="${AICOPILOT_DATA_DIR}")
```

### Configuration File Example

```ini
[terrain]
data_dir=./data/srtm
max_cached_tiles=16
enable_preload=true
preload_radius=2.0

[weather]
api_endpoint=https://aviationweather.gov/adds/dataserver
cache_dir=./data/weather
cache_duration_hours=24
update_interval_minutes=30

[navigation]
navaid_database=./data/navdata/navaids.db
airport_database=./data/navdata/airports.db
enable_spatial_index=true

[runways]
cifp_data_dir=./data/cifp
airac_cycle=2025-10
```

---

## 📈 Memory Usage Estimates

| Component | Size | Notes |
|-----------|------|-------|
| 1 SRTM Tile (HGT) | 1.4 MB | 1201×1201 int16 samples |
| 16 Cached Tiles | 22.4 MB | Typical cache size |
| 5,500 Navaids (Index) | 2 MB | With spatial index |
| 50,000 Waypoints | 5 MB | Fix definitions |
| Weather Cache (100 METAR) | 5 MB | 30-minute data |
| Runways (5000 airports) | 20 MB | Runway/procedure data |
| **TOTAL (Full System)** | **~60 MB** | All data in memory |

---

## 🎯 Priority Implementation Order

1. **HGT Terrain Parser** (Week 1-2)
   - Highest safety impact
   - Relatively simple binary format
   - Can be tested standalone

2. **METAR Weather Parser** (Week 2-3)
   - High operational impact
   - Text format is easier
   - API integration straightforward

3. **Navaid/Waypoint Database** (Week 3-4)
   - Navigation-critical
   - CSV format easy to parse
   - Spatial indexing adds value

4. **CIFP Procedure Parser** (Week 4-5)
   - Complex binary format
   - Most difficult integration
   - Reference implementation available

---

## 📞 Support Resources

### Available in Project
- `navdatareader-master/` - Reference CIFP parser (Qt application)
- `aicopilot/include/airport_data.hpp` - Full airport structures (543 lines)
- Existing parsers - CSV/text parsing patterns

### External References
- GDAL Documentation - Raster file handling
- cURL Documentation - HTTP requests
- SQLite3 Documentation - Database caching
- GeographicLib - Geodetic calculations

### FAA Documentation
- NFDC Format Specification
- CIFP Record Type Codes
- Navaid Database Format
- Airport Reference Information

---

## ✅ Validation Checklist

### Data Format Validation
- [ ] SRTM HGT: 1,442,401 bytes exactly
- [ ] METAR: Station ID in first token
- [ ] CIFP: Correct record type codes
- [ ] FAX: 8 fields minimum per line

### Integration Validation
- [ ] Terrain elevation queries work
- [ ] Weather hazards detected
- [ ] Runway procedures available
- [ ] Navaid distances calculated correctly

### Performance Validation
- [ ] Tile load < 500 ms
- [ ] Elevation query < 1 ms
- [ ] METAR parse < 10 ms
- [ ] Navaid search < 50 ms

---

**Last Updated:** October 27, 2025  
**Version:** 1.0  
**Status:** Ready for Implementation

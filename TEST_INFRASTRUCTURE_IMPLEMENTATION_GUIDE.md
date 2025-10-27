# Test Infrastructure Implementation Guide

## Quick Start: Phase 1 Priority (First 2 Weeks)

### Step 1: Enable Tests in Build System (1 hour)

**File: `CMakeLists.txt` - Line ~18**

Change:
```cmake
option(BUILD_TESTS "Build unit tests" OFF)
```

To:
```cmake
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_COVERAGE "Generate code coverage" OFF)
option(ENABLE_GMOCK "Enable Google Mock for advanced mocking" ON)
```

Add before the `if(BUILD_TESTS)` block (around line 230):
```cmake
# Google Test Configuration
include(GoogleTest)

# Ensure gmock is available
if(ENABLE_GMOCK)
    message(STATUS "Google Mock enabled for advanced mocking")
endif()
```

Update the `if(BUILD_TESTS)` section to:
```cmake
if(BUILD_TESTS)
    enable_testing()
    
    # Find GTest package
    find_package(GTest CONFIG REQUIRED)
    
    # Add test subdirectory
    add_subdirectory(aicopilot/tests/unit)
    
    message(STATUS "Unit tests enabled - run with: ctest --output-on-failure")
endif()
```

---

### Step 2: Create Mock Infrastructure Files (10-12 hours)

#### 2.1 Create Mock SimConnect Header

**File: `aicopilot/tests/unit/mocks/mock_simconnect_wrapper.h`**

```cpp
#pragma once

#include <gmock/gmock.h>
#include "aicopilot_types.h"
#include "simconnect_wrapper.h"

namespace AICopilot {
namespace Tests {

class MockSimConnectWrapper : public SimConnectWrapper {
public:
    // Connection Management
    MOCK_METHOD(bool, connect, 
        (SimulatorType simType, const std::string& appName), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    
    // Process messages
    MOCK_METHOD(void, processMessages, (), (override));
    
    // Aircraft state queries
    MOCK_METHOD(AircraftState, getAircraftState, (), (override));
    MOCK_METHOD(AutopilotState, getAutopilotState, (), (override));
    MOCK_METHOD(Position, getPosition, (), (override));
    
    // Autopilot control
    MOCK_METHOD(void, setAutopilotMaster, (bool enabled), (override));
    MOCK_METHOD(void, setAutopilotHeading, (double heading), (override));
    MOCK_METHOD(void, setAutopilotAltitude, (double altitude), (override));
    MOCK_METHOD(void, setAutopilotSpeed, (double speed), (override));
    MOCK_METHOD(void, setAutopilotVerticalSpeed, (double verticalSpeed), (override));
    MOCK_METHOD(void, setAutopilotNav, (bool enabled), (override));
    MOCK_METHOD(void, setAutopilotApproach, (bool enabled), (override));
    
    // Flight controls
    MOCK_METHOD(void, setThrottle, (double value), (override));
    MOCK_METHOD(void, setElevator, (double value), (override));
    MOCK_METHOD(void, setAileron, (double value), (override));
    MOCK_METHOD(void, setRudder, (double value), (override));
    MOCK_METHOD(void, setFlaps, (int position), (override));
    MOCK_METHOD(void, setGear, (bool down), (override));
    MOCK_METHOD(void, setSpoilers, (bool deployed), (override));
    MOCK_METHOD(void, setParkingBrake, (bool set), (override));
    MOCK_METHOD(void, setBrakes, (double value), (override));
    
    // Engine controls
    MOCK_METHOD(void, setMixture, (double value), (override));
    MOCK_METHOD(void, setPropellerPitch, (double value), (override));
    MOCK_METHOD(void, setMagnetos, (int position), (override));
    MOCK_METHOD(void, toggleEngineStarter, (int engineIndex), (override));
    MOCK_METHOD(void, setEngineState, (int engineIndex, bool running), (override));
    
    // Lighting
    MOCK_METHOD(void, setLight, (const std::string& lightName, bool on), (override));
    
    // ATC
    MOCK_METHOD(void, sendATCMenuSelection, (int menuIndex), (override));
    MOCK_METHOD(void, requestATCMenu, (), (override));
    MOCK_METHOD(std::vector<std::string>, getATCMenuOptions, (), (override));
    
    // Subscriptions
    MOCK_METHOD(void, subscribeToAircraftState, 
        (StateCallback callback), (override));
    MOCK_METHOD(void, subscribeToATCMessages, 
        (ATCCallback callback), (override));
};

} // namespace Tests
} // namespace AICopilot
```

#### 2.2 Create Mock Ollama Client Header

**File: `aicopilot/tests/unit/mocks/mock_ollama_client.h`**

```cpp
#pragma once

#include <gmock/gmock.h>
#include "ollama_client.h"

namespace AICopilot {
namespace Tests {

class MockOllamaClient : public OllamaClient {
public:
    MOCK_METHOD(bool, connect, (const std::string& host), (override));
    MOCK_METHOD(void, setApiKey, (const std::string& apiKey), (override));
    MOCK_METHOD(bool, isAvailable, (), (const, override));
    MOCK_METHOD(void, setModel, (const std::string& model), (override));
    MOCK_METHOD(std::string, getModel, (), (const, override));
    
    MOCK_METHOD(int, selectATCMenuOption,
        (const std::string& atcMessage,
         const std::vector<std::string>& menuOptions,
         const std::string& flightPhase,
         const std::string& context), (override));
    
    MOCK_METHOD(std::string, generatePrompt,
        (const std::string& atcMessage,
         const std::vector<std::string>& menuOptions,
         const std::string& flightPhase,
         const std::string& context), (const, override));
};

} // namespace Tests
} // namespace AICopilot
```

#### 2.3 Create Test Builders

**File: `aicopilot/tests/unit/fixtures/builders.h`**

```cpp
#pragma once

#include "aicopilot_types.h"

namespace AICopilot {
namespace Tests {

class AircraftStateBuilder {
private:
    AircraftState state_;

public:
    AircraftStateBuilder() {
        state_.aircraft_id = 0;
        state_.position = {0.0, 0.0, 0.0};
        state_.velocity_knots = 0.0;
        state_.heading_degrees = 0.0;
        state_.pitch_degrees = 0.0;
        state_.roll_degrees = 0.0;
        state_.altitude_feet = 0.0;
        state_.vertical_speed_fpm = 0.0;
    }

    AircraftStateBuilder& withPosition(double lat, double lon, double alt) {
        state_.position = {lat, lon, alt};
        return *this;
    }

    AircraftStateBuilder& withVelocity(double knots, double heading) {
        state_.velocity_knots = knots;
        state_.heading_degrees = heading;
        return *this;
    }

    AircraftStateBuilder& withAltitude(double altFeet) {
        state_.altitude_feet = altFeet;
        return *this;
    }

    AircraftStateBuilder& withVerticalSpeed(double fpm) {
        state_.vertical_speed_fpm = fpm;
        return *this;
    }

    AircraftStateBuilder& withAttitude(double pitch, double roll) {
        state_.pitch_degrees = pitch;
        state_.roll_degrees = roll;
        return *this;
    }

    AircraftState build() const {
        return state_;
    }
};

class ATCMessageBuilder {
private:
    ATCMessage msg_;

public:
    ATCMessageBuilder() {
        msg_.type = ATCMessage::MessageType::Instruction;
    }

    ATCMessageBuilder& withText(const std::string& text) {
        msg_.text = text;
        return *this;
    }

    ATCMessageBuilder& withType(ATCMessage::MessageType type) {
        msg_.type = type;
        return *this;
    }

    ATCMessageBuilder& withFrequency(double freq) {
        msg_.frequency = freq;
        return *this;
    }

    ATCMessage build() const {
        return msg_;
    }
};

class FlightPlanBuilder {
private:
    FlightPlan plan_;

public:
    FlightPlanBuilder() {
        plan_.title = "Test Flight Plan";
    }

    FlightPlanBuilder& withTitle(const std::string& title) {
        plan_.title = title;
        return *this;
    }

    FlightPlanBuilder& withDeparture(const std::string& icao) {
        plan_.departure_airport = icao;
        return *this;
    }

    FlightPlanBuilder& withArrival(const std::string& icao) {
        plan_.arrival_airport = icao;
        return *this;
    }

    FlightPlanBuilder& withCruiseAltitude(double altFeet) {
        plan_.cruise_altitude = altFeet;
        return *this;
    }

    FlightPlanBuilder& addWaypoint(const Waypoint& wp) {
        plan_.waypoints.push_back(wp);
        return *this;
    }

    FlightPlan build() const {
        return plan_;
    }
};

} // namespace Tests
} // namespace AICopilot
```

---

### Step 3: Create Priority 1 Test Files (8-10 hours)

#### 3.1 Config Parser Tests

**File: `aicopilot/tests/unit/config_parser_test.cpp`**

```cpp
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "../../include/config_parser.h"

namespace fs = std::filesystem;

namespace AICopilot {
namespace Tests {

class ConfigParserTest : public ::testing::Test {
protected:
    std::string testDataDir_ = "test_data/config_parser";

    void SetUp() override {
        // Create test data directory
        if (!fs::exists(testDataDir_)) {
            fs::create_directories(testDataDir_);
        }
    }

    void TearDown() override {
        // Clean up test files
        fs::remove_all(testDataDir_);
    }

    std::string createTestConfigFile(const std::string& content) {
        std::string filePath = testDataDir_ + "/test.cfg";
        std::ofstream file(filePath);
        file << content;
        file.close();
        return filePath;
    }
};

TEST_F(ConfigParserTest, ParsesValidIniFile) {
    std::string content = R"(
[General]
Title=Test Aircraft
Description=Test Description

[Systems]
EngineType=Reciprocating
EngineCount=1

[Performance]
MaxAltitude=14000
MaxSpeed=120.5
)";
    std::string filePath = createTestConfigFile(content);

    ConfigParser parser;
    ASSERT_TRUE(parser.parse(filePath));
    
    EXPECT_EQ(parser.getValue("General", "Title"), "Test Aircraft");
    EXPECT_EQ(parser.getValue("Systems", "EngineType"), "Reciprocating");
    EXPECT_EQ(parser.getIntValue("Systems", "EngineCount"), 1);
}

TEST_F(ConfigParserTest, HandlesCommentsCorrectly) {
    std::string content = R"(
[General]
Title=Test Aircraft  ; This is a comment
Description=Test    # Another comment style
)";
    std::string filePath = createTestConfigFile(content);

    ConfigParser parser;
    ASSERT_TRUE(parser.parse(filePath));
    
    EXPECT_EQ(parser.getValue("General", "Title"), "Test Aircraft");
    EXPECT_EQ(parser.getValue("General", "Description"), "Test");
}

TEST_F(ConfigParserTest, ParsesQuotedValues) {
    std::string content = R"(
[General]
Title="Test Aircraft With Spaces"
Path="C:\Program Files\Test"
)";
    std::string filePath = createTestConfigFile(content);

    ConfigParser parser;
    ASSERT_TRUE(parser.parse(filePath));
    
    EXPECT_EQ(parser.getValue("General", "Title"), "Test Aircraft With Spaces");
}

TEST_F(ConfigParserTest, ReturnsDefaultForMissingKeys) {
    std::string content = R"([General]
Title=Test
)";
    std::string filePath = createTestConfigFile(content);

    ConfigParser parser;
    ASSERT_TRUE(parser.parse(filePath));
    
    EXPECT_EQ(parser.getValue("General", "Missing", "DEFAULT"), "DEFAULT");
    EXPECT_EQ(parser.getIntValue("General", "Missing", 42), 42);
}

TEST_F(ConfigParserTest, HandlesNumericValues) {
    std::string content = R"(
[Performance]
MaxAltitude=14000
MaxSpeed=120.5
Multiplayer=1
Disabled=0
)";
    std::string filePath = createTestConfigFile(content);

    ConfigParser parser;
    ASSERT_TRUE(parser.parse(filePath));
    
    EXPECT_EQ(parser.getIntValue("Performance", "MaxAltitude"), 14000);
    EXPECT_DOUBLE_EQ(parser.getDoubleValue("Performance", "MaxSpeed"), 120.5);
    EXPECT_TRUE(parser.getBoolValue("Performance", "Multiplayer"));
    EXPECT_FALSE(parser.getBoolValue("Performance", "Disabled"));
}

TEST_F(ConfigParserTest, ListsSectionsAndKeys) {
    std::string content = R"(
[General]
Title=Test
Description=Test

[Systems]
Engine=1
Avionics=1
)";
    std::string filePath = createTestConfigFile(content);

    ConfigParser parser;
    ASSERT_TRUE(parser.parse(filePath));
    
    auto sections = parser.getSections();
    EXPECT_EQ(sections.size(), 2);
    EXPECT_TRUE(parser.hasSection("General"));
    EXPECT_TRUE(parser.hasSection("Systems"));
    
    auto keys = parser.getKeys("General");
    EXPECT_EQ(keys.size(), 2);
    EXPECT_TRUE(parser.hasKey("General", "Title"));
}

TEST_F(ConfigParserTest, HandlesMissingFile) {
    ConfigParser parser;
    ASSERT_FALSE(parser.parse("nonexistent_file.cfg"));
}

TEST_F(ConfigParserTest, HandlesEmptyFile) {
    std::string filePath = createTestConfigFile("");
    ConfigParser parser;
    ASSERT_TRUE(parser.parse(filePath));
    EXPECT_EQ(parser.getSections().size(), 0);
}

TEST_F(ConfigParserTest, ClearsDataOnParse) {
    std::string content1 = "[Section1]\nKey=Value";
    std::string filePath1 = createTestConfigFile(content1);
    
    ConfigParser parser;
    ASSERT_TRUE(parser.parse(filePath1));
    EXPECT_TRUE(parser.hasSection("Section1"));
    
    std::string content2 = "[Section2]\nDifferent=Data";
    std::string filePath2 = createTestConfigFile(content2);
    
    ASSERT_TRUE(parser.parse(filePath2));
    EXPECT_FALSE(parser.hasSection("Section1"));
    EXPECT_TRUE(parser.hasSection("Section2"));
}

} // namespace Tests
} // namespace AICopilot
```

#### 3.2 Navigation Tests

**File: `aicopilot/tests/unit/navigation_test.cpp`**

```cpp
#include <gtest/gtest.h>
#include <cmath>
#include "../../include/navigation.h"

namespace AICopilot {
namespace Tests {

class NavigationTest : public ::testing::Test {
protected:
    Navigation nav_;

    // Reference coordinates for testing
    static constexpr double KORD_LAT = 41.8819;
    static constexpr double KORD_LON = -87.6278;
    static constexpr double KJFK_LAT = 40.6413;
    static constexpr double KJFK_LON = -73.7781;
    
    // Known distances (nautical miles)
    static constexpr double KORD_KJFK_DISTANCE = 740.0;
    static constexpr double DISTANCE_TOLERANCE = 5.0;  // ±5 nm
    
    // Known bearing
    static constexpr double KORD_KJFK_BEARING = 89.0;
    static constexpr double BEARING_TOLERANCE = 2.0;  // ±2 degrees
};

TEST_F(NavigationTest, CalculatesGreatCircleDistance) {
    Position pos1{KORD_LAT, KORD_LON, 0.0};
    Position pos2{KJFK_LAT, KJFK_LON, 0.0};
    
    Waypoint wp1, wp2;
    wp1.position = pos1;
    wp2.position = pos2;
    
    // Create simple flight plan
    FlightPlan plan;
    plan.waypoints.push_back(wp1);
    plan.waypoints.push_back(wp2);
    
    // Test distance calculation indirectly through navigation
    double distance = nav_.distanceToWaypoint(pos1, wp2);
    
    EXPECT_NEAR(distance, KORD_KJFK_DISTANCE, DISTANCE_TOLERANCE)
        << "Distance KORD→KJFK should be ~740 nm";
}

TEST_F(NavigationTest, CalculatesBearing) {
    Position pos1{KORD_LAT, KORD_LON, 0.0};
    Position pos2{KJFK_LAT, KJFK_LON, 0.0};
    
    Waypoint wp;
    wp.position = pos2;
    
    double bearing = nav_.bearingToWaypoint(pos1, wp);
    
    // Should be roughly east (90 degrees)
    EXPECT_NEAR(bearing, KORD_KJFK_BEARING, BEARING_TOLERANCE)
        << "Bearing KORD→KJFK should be ~89° (east)";
}

TEST_F(NavigationTest, AdvancesWaypoint) {
    FlightPlan plan;
    Waypoint wp1, wp2, wp3;
    wp1.name = "KORD";
    wp2.name = "WAYPOINT1";
    wp3.name = "KJFK";
    
    plan.waypoints = {wp1, wp2, wp3};
    nav_.loadFlightPlan("dummy_path");  // This will fail, but we set up plan manually
    
    // Verify waypoint advancement logic
    // (This would need actual method exposure in Navigation class)
}

TEST_F(NavigationTest, ChecksWaypointReached) {
    Position current{KORD_LAT, KORD_LON, 0.0};
    Waypoint wp;
    wp.position = current;
    
    bool reached = nav_.isWaypointReached(current, 1.0);
    EXPECT_TRUE(reached) << "Current position at waypoint should be reached";
}

TEST_F(NavigationTest, CalculatesCrossTrackError) {
    // Create flight plan with waypoints
    FlightPlan plan;
    
    // This test requires CTEcalculation exposure
    // Expected to test cross-track deviation from planned track
}

TEST_F(NavigationTest, ValidatesFlightPlan) {
    FlightPlan plan;
    plan.departure_airport = "KORD";
    plan.arrival_airport = "KJFK";
    
    Waypoint wp1, wp2;
    wp1.position = {KORD_LAT, KORD_LON, 0.0};
    wp2.position = {KJFK_LAT, KJFK_LON, 0.0};
    
    plan.waypoints = {wp1, wp2};
    
    bool valid = nav_.validateFlightPlan();
    EXPECT_TRUE(valid) << "Flight plan with valid coordinates should be valid";
}

TEST_F(NavigationTest, CalculatesTotalDistance) {
    FlightPlan plan;
    
    // Add multiple waypoints
    std::vector<Position> positions = {
        {41.0, -87.0, 0.0},  // KORD area
        {40.5, -83.0, 0.0},  // Intermediate
        {40.6, -73.7, 0.0}   // KJFK area
    };
    
    for (const auto& pos : positions) {
        Waypoint wp;
        wp.position = pos;
        plan.waypoints.push_back(wp);
    }
    
    double totalDistance = nav_.getTotalDistance();
    EXPECT_GT(totalDistance, 0.0) << "Total distance should be positive";
}

TEST_F(NavigationTest, HandlesAntipodal Points) {
    // Test distance calculation at opposite sides of Earth
    Position north{90.0, 0.0, 0.0};      // North Pole
    Position south{-90.0, 0.0, 0.0};     // South Pole
    
    Waypoint wp;
    wp.position = south;
    
    double distance = nav_.distanceToWaypoint(north, wp);
    EXPECT_NEAR(distance, 10800.0, 100.0)  // ~10,800 nm (half Earth circumference)
        << "Pole-to-pole distance should be ~10,800 nm";
}

TEST_F(NavigationTest, HandlesEquatorCrossing) {
    // Test crossing from Northern to Southern hemisphere
    Position start{10.0, 0.0, 0.0};
    Position end{-10.0, 0.0, 0.0};
    
    Waypoint wp;
    wp.position = end;
    
    double distance = nav_.distanceToWaypoint(start, wp);
    EXPECT_GT(distance, 0.0) << "Cross-equator distance should be positive";
}

} // namespace Tests
} // namespace AICopilot
```

---

### Step 4: Update CMakeLists.txt for Tests (2 hours)

**File: `aicopilot/tests/unit/CMakeLists.txt`**

```cmake
# Add test source files
set(TEST_SOURCES
    clearance_state_machine_test.cpp
    config_parser_test.cpp
    navigation_test.cpp
)

# Add mock include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fixtures)

# Create test executable
add_executable(aicopilot_unit_tests ${TEST_SOURCES})

# Link required libraries
target_link_libraries(aicopilot_unit_tests PRIVATE 
    aicopilot
    GTest::gtest 
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
)

# Discover and register tests
gtest_discover_tests(aicopilot_unit_tests)

# Print test info
add_custom_target(info_tests
    COMMAND ${CMAKE_COMMAND} -E echo "Tests configured. Run with: ctest --test-dir ${CMAKE_BINARY_DIR} --output-on-failure"
)
```

---

### Step 5: Build and Run (1 hour)

```bash
# Navigate to project
cd c:\Users\marti\source\repos\Plane14\AICopilotFS

# Create build directory
mkdir build
cd build

# Configure with tests enabled
cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build . --config Debug

# Run tests
ctest --output-on-failure

# Or run specific tests
ctest -R ConfigParser --output-on-failure
```

---

## Phase 2: Expanding Test Coverage (Weeks 3-4)

### Create Additional Test Files

Once Phase 1 is complete, create tests for:

1. **SimConnect Wrapper** (`simconnect_wrapper_test.cpp`)
2. **ATC Controller** (`atc_controller_test.cpp`)
3. **Weather System** (`weather_system_test.cpp`)
4. **Terrain Awareness** (`terrain_awareness_test.cpp`)
5. **Traffic System** (`traffic_system_test.cpp`)

### Template for Each:

```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../include/component.h"
#include "mocks/mock_*.h"
#include "fixtures/builders.h"

namespace AICopilot {
namespace Tests {

class ComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mocks
        // Create builders
    }
    
    std::shared_ptr<Mock*> mock_;
    // Other fixtures
};

// Test cases following same pattern as above

} // namespace Tests
} // namespace AICopilot
```

---

## Continuous Integration Setup

### GitHub Actions Workflow

**File: `.github/workflows/tests.yml`**

```yaml
name: C++ Unit Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest
    
    strategy:
      matrix:
        cmake-build-type: [Debug, Release]
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Configure CMake
      run: |
        cmake -B build -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=${{ matrix.cmake-build-type }}
    
    - name: Build Tests
      run: cmake --build build --config ${{ matrix.cmake-build-type }}
    
    - name: Run Tests
      run: ctest --test-dir build --output-on-failure -C ${{ matrix.cmake-build-type }}
```

---

## Success Checklist

- [ ] CMakeLists.txt updated with test options
- [ ] Mock infrastructure created and compiles
- [ ] Test fixtures/builders implemented
- [ ] Priority 1 test files created (Config Parser, Navigation)
- [ ] All Priority 1 tests passing locally
- [ ] GitHub Actions workflow created
- [ ] Tests run successfully in CI/CD
- [ ] Code coverage analysis setup (optional but recommended)

---

## Testing Best Practices Going Forward

1. **Write tests BEFORE implementation** when possible (TDD)
2. **Keep mocks close to real implementation** - update when interfaces change
3. **Use descriptive test names** - `TEST(ComponentTest, DescribesScenarioAndExpectation)`
4. **One assertion per test** when possible
5. **Use fixtures** for common setup/teardown
6. **Test edge cases** - zero, negative, max values
7. **Keep tests fast** - unit tests should run in < 1 minute total
8. **Separate unit and integration tests** - run frequently vs. nightly
9. **Document test data** - why specific values matter
10. **Refactor tests** - keep test code clean and maintainable

---

**Next Actions:**
1. Apply CMakeLists.txt changes (30 min)
2. Create mock directory structure (15 min)
3. Implement mock headers (2 hours)
4. Implement builders (2 hours)
5. Create Priority 1 test files (4-5 hours)
6. Build and debug (1-2 hours)
7. Verify all tests pass locally (30 min)
8. Commit to repository (15 min)

**Total: ~11-13 hours for Phase 1 foundation** ✓


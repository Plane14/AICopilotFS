/*****************************************************************************
* Copyright 2025 AI Copilot FS Project - Phase 1 Test Infrastructure
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include <gtest/gtest.h>
#include "../include/aicopilot_types.h"
#include "phase1/mock_simconnect.h"
#include "phase1/mock_databases.h"

using namespace AICopilot;
using namespace AICopilot::Testing;

/**
 * Global test environment setup
 */
class AICopilotTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Initialize any global resources needed for all tests
        // E.g., logging systems, global configuration
    }

    void TearDown() override {
        // Clean up any global resources
    }
};

/**
 * Base test fixture for Phase 1 tests
 * Provides common setup and mock infrastructure
 */
class AICopilotPhase1TestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock instances for each test
        mockSimConnect_ = MockSimConnectFactory::createMock();
        terrainDatabase_ = std::make_unique<MockTerrainDatabase>(
            MockDatabaseFactory::createTerrainDatabase());
        weatherDatabase_ = std::make_unique<MockWeatherDatabase>(
            MockDatabaseFactory::createWeatherDatabase());
        runwayDatabase_ = std::make_unique<MockRunwayDatabase>(
            MockDatabaseFactory::createRunwayDatabase());
        navDatabase_ = std::make_unique<MockNavDatabase>(
            MockDatabaseFactory::createNavDatabase());

        // Set up default mock aircraft state
        defaultAircraftState_ = MockSimConnectFactory::createDefaultAircraftState();
        defaultAutopilotState_ = MockSimConnectFactory::createDefaultAutopilotState();
    }

    void TearDown() override {
        // Clean up test resources
        mockSimConnect_.reset();
        terrainDatabase_.reset();
        weatherDatabase_.reset();
        runwayDatabase_.reset();
        navDatabase_.reset();
    }

    // Helper methods for tests
    Position createPosition(double lat, double lon, double alt, double hdg = 0.0) {
        return {lat, lon, alt, hdg};
    }

    AircraftState createAircraftState(
        const Position& pos,
        double ias = 100.0,
        bool onGround = false,
        double fuel = 100.0) {
        
        AircraftState state = defaultAircraftState_;
        state.position = pos;
        state.indicatedAirspeed = ias;
        state.trueAirspeed = ias;
        state.onGround = onGround;
        state.fuelQuantity = fuel;
        return state;
    }

    WeatherConditions createWeatherConditions(
        double temp = 15.0,
        double windSpeed = 5.0,
        double windDir = 270.0,
        double visibility = 10.0) {
        
        WeatherConditions wx{};
        wx.temperature = temp;
        wx.dewpoint = temp - 5.0;
        wx.windSpeed = windSpeed;
        wx.windDirection = windDir;
        wx.visibility = visibility;
        wx.ceiling = 5000.0;
        return wx;
    }

    // Helper to verify mock calls
    template<typename T>
    void verifyMockCalled(const std::shared_ptr<T>& mock) {
        // Mock verification helpers can be added here
    }

protected:
    std::shared_ptr<MockSimConnect> mockSimConnect_;
    std::unique_ptr<MockTerrainDatabase> terrainDatabase_;
    std::unique_ptr<MockWeatherDatabase> weatherDatabase_;
    std::unique_ptr<MockRunwayDatabase> runwayDatabase_;
    std::unique_ptr<MockNavDatabase> navDatabase_;

    AircraftState defaultAircraftState_;
    AutopilotState defaultAutopilotState_;
};

/**
 * Initialize the test environment
 * This runs once before all tests
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new AICopilotTestEnvironment());
    return RUN_ALL_TESTS();
}

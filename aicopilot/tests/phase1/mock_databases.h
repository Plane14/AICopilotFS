/*****************************************************************************
* Copyright 2025 AI Copilot FS Project - Phase 1 Test Infrastructure
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef MOCK_DATABASES_H
#define MOCK_DATABASES_H

#include "../include/aicopilot_types.h"
#include "../include/terrain_awareness.h"
#include "../include/weather_system.h"
#include "../include/approach_system.h"
#include <gmock/gmock.h>
#include <vector>
#include <map>

namespace AICopilot {
namespace Testing {

/**
 * Mock Terrain Elevation Database
 * Provides elevation data without external dependencies
 */
class MockTerrainDatabase {
public:
    MockTerrainDatabase() : elevationOffset_(0.0) {}

    // Store terrain points for lookup
    std::map<std::pair<double, double>, double> elevationMap_;
    double elevationOffset_;

    double getElevation(double latitude, double longitude) {
        auto key = std::make_pair(latitude, longitude);
        if (elevationMap_.find(key) != elevationMap_.end()) {
            return elevationMap_[key];
        }
        // Return default elevation based on latitude
        return 500.0 + elevationOffset_;
    }

    void setElevation(double latitude, double longitude, double elevation) {
        auto key = std::make_pair(latitude, longitude);
        elevationMap_[key] = elevation;
    }

    std::vector<TerrainPoint> getTerrainProfile(
        const Position& start,
        double heading,
        double distance,
        int samples = 10) {
        
        std::vector<TerrainPoint> profile;
        const double DEG_TO_RAD = 3.14159265359 / 180.0;
        
        for (int i = 0; i <= samples; ++i) {
            double progress = i / static_cast<double>(samples);
            double segDistance = progress * distance;
            
            // Simple Cartesian approximation for short distances
            double dLat = (segDistance / 60.0) * std::cos(heading * DEG_TO_RAD);
            double dLon = (segDistance / 60.0) * std::sin(heading * DEG_TO_RAD) / 
                         std::cos(start.latitude * DEG_TO_RAD);
            
            Position pos{start.latitude + dLat, start.longitude + dLon, start.altitude, heading};
            double elevation = getElevation(pos.latitude, pos.longitude);
            
            TerrainPoint tp{pos, elevation};
            profile.push_back(tp);
        }
        return profile;
    }

    void addMountain(double latitude, double longitude, double peakElevation, double radius) {
        for (double dlat = -radius; dlat <= radius; dlat += 0.1) {
            for (double dlon = -radius; dlon <= radius; dlon += 0.1) {
                double distance = std::sqrt(dlat * dlat + dlon * dlon);
                if (distance <= radius) {
                    double elevation = peakElevation * (1.0 - distance / radius);
                    setElevation(latitude + dlat, longitude + dlon, elevation);
                }
            }
        }
    }
};

/**
 * Mock Weather Provider
 * Simulates weather conditions without external data sources
 */
class MockWeatherDatabase {
public:
    MockWeatherDatabase() = default;

    WeatherConditions currentWeather_;
    std::vector<WeatherHazard> hazards_;

    WeatherConditions getWeatherAt(const Position& pos) {
        return currentWeather_;
    }

    void setCurrentWeather(const WeatherConditions& weather) {
        currentWeather_ = weather;
    }

    void addWeatherHazard(const WeatherHazard& hazard) {
        hazards_.push_back(hazard);
    }

    std::vector<WeatherHazard> getHazardsInArea(
        const Position& center,
        double radius) {
        
        std::vector<WeatherHazard> result;
        for (const auto& hazard : hazards_) {
            double distance = calculateDistance(center, hazard.position);
            if (distance <= radius) {
                result.push_back(hazard);
            }
        }
        return result;
    }

    void clearHazards() {
        hazards_.clear();
    }

private:
    double calculateDistance(const Position& p1, const Position& p2) {
        const double R = 3440.065; // Earth radius in nautical miles
        const double DEG_TO_RAD = 3.14159265359 / 180.0;
        
        double dLat = (p2.latitude - p1.latitude) * DEG_TO_RAD;
        double dLon = (p2.longitude - p1.longitude) * DEG_TO_RAD;
        
        double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                   std::cos(p1.latitude * DEG_TO_RAD) * std::cos(p2.latitude * DEG_TO_RAD) *
                   std::sin(dLon / 2) * std::sin(dLon / 2);
        
        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return R * c;
    }
};

/**
 * Mock Runway Database
 * Provides runway and airport information
 */
class MockRunwayDatabase {
public:
    MockRunwayDatabase() = default;

    struct Runway {
        std::string icao;
        std::string runwayId;
        Position thresholdPosition;
        double heading;
        double length;  // feet
        double width;   // feet
        bool hasILS;
    };

    std::vector<Runway> runways_;

    void addRunway(const std::string& icao, const std::string& runwayId,
                   const Position& threshold, double heading, 
                   double length, double width, bool hasILS = false) {
        Runway rwy{icao, runwayId, threshold, heading, length, width, hasILS};
        runways_.push_back(rwy);
    }

    std::vector<Runway> getRunwaysForAirport(const std::string& icao) {
        std::vector<Runway> result;
        for (const auto& rwy : runways_) {
            if (rwy.icao == icao) {
                result.push_back(rwy);
            }
        }
        return result;
    }

    Runway getRunway(const std::string& icao, const std::string& runwayId) {
        for (const auto& rwy : runways_) {
            if (rwy.icao == icao && rwy.runwayId == runwayId) {
                return rwy;
            }
        }
        return Runway{"", "", {0.0, 0.0, 0.0, 0.0}, 0.0, 0.0, 0.0, false};
    }

    void clearRunways() {
        runways_.clear();
    }
};

/**
 * Mock Navigation Database
 * Provides waypoints, navaids, and approach procedures
 */
class MockNavDatabase {
public:
    MockNavDatabase() = default;

    struct Navaid {
        std::string id;
        std::string name;
        Position position;
        std::string type;  // VOR, NDB, TACAN
        double frequency;
    };

    std::vector<Navaid> navaids_;
    std::map<std::string, Waypoint> waypoints_;

    void addNavaid(const std::string& id, const std::string& name,
                   const Position& pos, const std::string& type, double freq) {
        Navaid nav{id, name, pos, type, freq};
        navaids_.push_back(nav);
    }

    void addWaypoint(const std::string& id, const Waypoint& waypoint) {
        waypoints_[id] = waypoint;
    }

    Waypoint getWaypoint(const std::string& id) {
        if (waypoints_.find(id) != waypoints_.end()) {
            return waypoints_[id];
        }
        return Waypoint{{0.0, 0.0, 0.0, 0.0}, ""};
    }

    Navaid getNavaid(const std::string& id) {
        for (const auto& nav : navaids_) {
            if (nav.id == id) {
                return nav;
            }
        }
        return Navaid{"", "", {0.0, 0.0, 0.0, 0.0}, "", 0.0};
    }

    std::vector<Navaid> getNavaidsInRange(const Position& pos, double range) {
        std::vector<Navaid> result;
        for (const auto& nav : navaids_) {
            double distance = calculateDistance(pos, nav.position);
            if (distance <= range) {
                result.push_back(nav);
            }
        }
        return result;
    }

    void clearDatabase() {
        navaids_.clear();
        waypoints_.clear();
    }

private:
    double calculateDistance(const Position& p1, const Position& p2) {
        const double R = 3440.065; // Earth radius in nautical miles
        const double DEG_TO_RAD = 3.14159265359 / 180.0;
        
        double dLat = (p2.latitude - p1.latitude) * DEG_TO_RAD;
        double dLon = (p2.longitude - p1.longitude) * DEG_TO_RAD;
        
        double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                   std::cos(p1.latitude * DEG_TO_RAD) * std::cos(p2.latitude * DEG_TO_RAD) *
                   std::sin(dLon / 2) * std::sin(dLon / 2);
        
        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return R * c;
    }
};

/**
 * Mock Database Factory for consistent test setup
 */
class MockDatabaseFactory {
public:
    static MockTerrainDatabase createTerrainDatabase() {
        MockTerrainDatabase db;
        // Add some default terrain features
        db.setElevation(37.7749, -122.4194, 100.0);   // San Francisco
        db.setElevation(34.0522, -118.2437, 400.0);   // Los Angeles
        db.setElevation(40.7128, -74.0060, 50.0);     // New York
        db.addMountain(39.0, -105.0, 14000.0, 0.5);   // Rocky Mountains
        return db;
    }

    static MockWeatherDatabase createWeatherDatabase() {
        MockWeatherDatabase db;
        WeatherConditions wx{};
        wx.temperature = 15.0;
        wx.dewpoint = 10.0;
        wx.windSpeed = 5.0;
        wx.windDirection = 270.0;
        wx.visibility = 10.0;
        wx.ceiling = 5000.0;
        db.setCurrentWeather(wx);
        return db;
    }

    static MockRunwayDatabase createRunwayDatabase() {
        MockRunwayDatabase db;
        db.addRunway("KSFO", "28L", Position{37.7213, -122.2209, 0.0}, 280.0, 11000.0, 200.0, true);
        db.addRunway("KSFO", "28R", Position{37.7213, -122.2209, 0.0}, 280.0, 11000.0, 200.0, true);
        db.addRunway("KLAX", "25L", Position{33.9425, -118.4081, 0.0}, 250.0, 12923.0, 200.0, true);
        db.addRunway("KLAX", "25R", Position{33.9425, -118.4081, 0.0}, 250.0, 12923.0, 200.0, true);
        return db;
    }

    static MockNavDatabase createNavDatabase() {
        MockNavDatabase db;
        db.addNavaid("SFO", "San Francisco VOR", Position{37.7213, -122.2209, 0.0}, "VOR", 115.8);
        db.addNavaid("LAX", "Los Angeles VOR", Position{33.9425, -118.4081, 0.0}, "VOR", 117.9);
        db.addWaypoint("SFO01", Waypoint{Position{37.7213, -122.2209, 5000.0}, "SFO01"});
        db.addWaypoint("LAX01", Waypoint{Position{33.9425, -118.4081, 5000.0}, "LAX01"});
        return db;
    }
};

} // namespace Testing
} // namespace AICopilot

#endif // MOCK_DATABASES_H

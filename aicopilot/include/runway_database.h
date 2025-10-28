#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>

namespace AICopilot {

struct RunwayData {
    std::string icao_code;
    std::string runway_id;
    double latitude;
    double longitude;
    double elevation_feet;
    int heading_true;
    double length_feet;
    double width_feet;
    std::string surface_type;
    bool has_ils;
};

struct RunwayInfo {
    std::string runway_id;
    double latitude;
    double longitude;
    double elevation_feet;
    int heading_true;
    double length_feet;
    double width_feet;
    std::string surface_type;
    bool has_ils;
};

class RunwayDatabase {
public:
    RunwayDatabase() {
        initializeRunways();
    }

    std::vector<RunwayInfo> GetRunwayInfo(const std::string& icao_code) {
        std::vector<RunwayInfo> results;
        
        auto it = runways_.find(icao_code);
        if (it != runways_.end()) {
            for (const auto& rwy : it->second) {
                RunwayInfo info;
                info.runway_id = rwy.runway_id;
                info.latitude = rwy.latitude;
                info.longitude = rwy.longitude;
                info.elevation_feet = rwy.elevation_feet;
                info.heading_true = rwy.heading_true;
                info.length_feet = rwy.length_feet;
                info.width_feet = rwy.width_feet;
                info.surface_type = rwy.surface_type;
                info.has_ils = rwy.has_ils;
                results.push_back(info);
            }
        }
        
        return results;
    }

    RunwayInfo GetPrimaryRunway(const std::string& icao_code) {
        auto it = runways_.find(icao_code);
        if (it != runways_.end() && !it->second.empty()) {
            const auto& rwy = it->second[0];
            RunwayInfo info;
            info.runway_id = rwy.runway_id;
            info.latitude = rwy.latitude;
            info.longitude = rwy.longitude;
            info.elevation_feet = rwy.elevation_feet;
            info.heading_true = rwy.heading_true;
            info.length_feet = rwy.length_feet;
            info.width_feet = rwy.width_feet;
            info.surface_type = rwy.surface_type;
            info.has_ils = rwy.has_ils;
            return info;
        }
        
        RunwayInfo empty;
        empty.runway_id = "";
        empty.length_feet = 0.0;
        empty.has_ils = false;
        return empty;
    }

    int GetRunwayCount(const std::string& icao_code) {
        auto it = runways_.find(icao_code);
        if (it != runways_.end()) {
            return static_cast<int>(it->second.size());
        }
        return 0;
    }

    bool HasRunwayWithILS(const std::string& icao_code) {
        auto it = runways_.find(icao_code);
        if (it != runways_.end()) {
            for (const auto& rwy : it->second) {
                if (rwy.has_ils) {
                    return true;
                }
            }
        }
        return false;
    }

    std::string GetRunwayForHeading(const std::string& icao_code, int desired_heading) {
        auto it = runways_.find(icao_code);
        if (it == runways_.end() || it->second.empty()) {
            return "";
        }

        int best_diff = 180;
        std::string best_runway = "";

        for (const auto& rwy : it->second) {
            int diff = std::abs(rwy.heading_true - desired_heading);
            if (diff > 180) {
                diff = 360 - diff;
            }

            if (diff < best_diff) {
                best_diff = diff;
                best_runway = rwy.runway_id;
            }
        }

        return best_runway;
    }

private:
    std::unordered_map<std::string, std::vector<RunwayData>> runways_;

    void initializeRunways() {
        std::vector<RunwayData> kjfk_runways = {
            {"KJFK", "04L", 40.6413, -73.7781, 13.0, 40, 10000.0, 150.0, "ASPHALT", true},
            {"KJFK", "04R", 40.6413, -73.7625, 13.0, 40, 10000.0, 150.0, "ASPHALT", true},
            {"KJFK", "22L", 40.6413, -73.7781, 13.0, 220, 10000.0, 150.0, "ASPHALT", true},
            {"KJFK", "22R", 40.6413, -73.7625, 13.0, 220, 10000.0, 150.0, "ASPHALT", true},
        };
        runways_["KJFK"] = kjfk_runways;

        std::vector<RunwayData> klax_runways = {
            {"KLAX", "07L", 33.9425, -118.4081, 285.0, 70, 13000.0, 200.0, "ASPHALT", true},
            {"KLAX", "07R", 33.9425, -118.3667, 285.0, 70, 12950.0, 200.0, "ASPHALT", true},
            {"KLAX", "25L", 33.9425, -118.4081, 285.0, 250, 13000.0, 200.0, "ASPHALT", true},
            {"KLAX", "25R", 33.9425, -118.3667, 285.0, 250, 12950.0, 200.0, "ASPHALT", true},
        };
        runways_["KLAX"] = klax_runways;

        std::vector<RunwayData> kord_runways = {
            {"KORD", "10L", 41.9889, -87.9048, 682.0, 100, 11000.0, 150.0, "ASPHALT", true},
            {"KORD", "10R", 41.9889, -87.8933, 682.0, 100, 11000.0, 150.0, "ASPHALT", true},
            {"KORD", "28L", 41.9889, -87.9048, 682.0, 280, 11000.0, 150.0, "ASPHALT", true},
            {"KORD", "28R", 41.9889, -87.8933, 682.0, 280, 11000.0, 150.0, "ASPHALT", true},
        };
        runways_["KORD"] = kord_runways;

        std::vector<RunwayData> kdfw_runways = {
            {"KDFW", "18L", 32.8975, -97.0383, 607.0, 180, 13000.0, 200.0, "ASPHALT", true},
            {"KDFW", "18R", 32.8975, -97.0200, 607.0, 180, 13000.0, 200.0, "ASPHALT", true},
            {"KDFW", "36L", 32.8975, -97.0383, 607.0, 360, 13000.0, 200.0, "ASPHALT", true},
            {"KDFW", "36R", 32.8975, -97.0200, 607.0, 360, 13000.0, 200.0, "ASPHALT", true},
        };
        runways_["KDFW"] = kdfw_runways;

        std::vector<RunwayData> kden_runways = {
            {"KDEN", "17L", 39.8561, -104.6737, 5431.0, 170, 12000.0, 200.0, "ASPHALT", true},
            {"KDEN", "17R", 39.8561, -104.6500, 5431.0, 170, 12000.0, 200.0, "ASPHALT", true},
            {"KDEN", "35L", 39.8561, -104.6737, 5431.0, 350, 12000.0, 200.0, "ASPHALT", true},
            {"KDEN", "35R", 39.8561, -104.6500, 5431.0, 350, 12000.0, 200.0, "ASPHALT", true},
        };
        runways_["KDEN"] = kden_runways;

        std::vector<RunwayData> kbos_runways = {
            {"KBOS", "04R", 42.3656, -71.0096, 20.0, 40, 10000.0, 150.0, "ASPHALT", true},
            {"KBOS", "04L", 42.3656, -71.0200, 20.0, 40, 10000.0, 150.0, "ASPHALT", true},
            {"KBOS", "22L", 42.3656, -71.0096, 20.0, 220, 10000.0, 150.0, "ASPHALT", true},
            {"KBOS", "22R", 42.3656, -71.0200, 20.0, 220, 10000.0, 150.0, "ASPHALT", true},
        };
        runways_["KBOS"] = kbos_runways;

        std::vector<RunwayData> ksfo_runways = {
            {"KSFO", "10L", 37.6213, -122.3790, 8.0, 100, 12000.0, 200.0, "ASPHALT", true},
            {"KSFO", "10R", 37.6213, -122.3588, 8.0, 100, 12000.0, 200.0, "ASPHALT", true},
            {"KSFO", "28L", 37.6213, -122.3790, 8.0, 280, 12000.0, 200.0, "ASPHALT", true},
            {"KSFO", "28R", 37.6213, -122.3588, 8.0, 280, 12000.0, 200.0, "ASPHALT", true},
        };
        runways_["KSFO"] = ksfo_runways;

        std::vector<RunwayData> kmco_runways = {
            {"KMCO", "09L", 28.4294, -81.3089, 92.0, 90, 12000.0, 150.0, "ASPHALT", true},
            {"KMCO", "09R", 28.4294, -81.2850, 92.0, 90, 12000.0, 150.0, "ASPHALT", true},
            {"KMCO", "27L", 28.4294, -81.3089, 92.0, 270, 12000.0, 150.0, "ASPHALT", true},
            {"KMCO", "27R", 28.4294, -81.2850, 92.0, 270, 12000.0, 150.0, "ASPHALT", true},
        };
        runways_["KMCO"] = kmco_runways;

        std::vector<RunwayData> ksea_runways = {
            {"KSEA", "16L", 47.4502, -122.3088, 433.0, 160, 11901.0, 150.0, "ASPHALT", true},
            {"KSEA", "16C", 47.4502, -122.3000, 433.0, 160, 11901.0, 150.0, "ASPHALT", false},
            {"KSEA", "16R", 47.4502, -122.2912, 433.0, 160, 11901.0, 150.0, "ASPHALT", false},
            {"KSEA", "34L", 47.4502, -122.3088, 433.0, 340, 11901.0, 150.0, "ASPHALT", false},
        };
        runways_["KSEA"] = ksea_runways;
    }
};

}

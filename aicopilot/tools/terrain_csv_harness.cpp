#include "terrain_awareness.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace AICopilot;

int main() {
    TerrainAwareness ta;
    auto tmp = std::filesystem::temp_directory_path() / "ta_csv_harness.csv";
    std::ofstream ofs(tmp.string());
    if (!ofs.is_open()) {
        std::cerr << "Failed to open temp file: " << tmp << std::endl;
        return 1;
    }
    ofs << "34.000000,-118.000000,500.0\n";
    ofs << "35.000000,-119.000000,1500.0\n";
    ofs.close();

    if (!ta.loadTerrainDatabase(tmp.string())) {
        std::cerr << "loadTerrainDatabase failed" << std::endl;
        return 2;
    }

    Position q{34.0001, -118.0001, 0.0, 0.0};
    double elev = ta.getTerrainElevation(q);
    std::cout << "Queried elevation: " << elev << std::endl;
    if (elev != 500.0) {
        std::cerr << "Elevation mismatch (expected 500.0)" << std::endl;
        return 3;
    }

    // cleanup
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    std::cout << "Terrain CSV harness: PASS" << std::endl;
    return 0;
}

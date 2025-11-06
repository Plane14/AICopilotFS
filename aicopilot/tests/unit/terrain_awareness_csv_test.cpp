#include <gtest/gtest.h>
#include "terrain_awareness.h"
#include <fstream>
#include <filesystem>

using namespace AICopilot;

TEST(TerrainAwarenessCsvTest, LoadCsvAndExactElevation) {
    TerrainAwareness ta;

    auto tmp = std::filesystem::temp_directory_path() / "ta_csv_test.csv";
    std::ofstream ofs(tmp.string());
    ASSERT_TRUE(ofs.is_open());
    ofs << "34.000000,-118.000000,500.0\n";
    ofs.close();

    ASSERT_TRUE(ta.loadTerrainDatabase(tmp.string()));

    Position q{34.0, -118.0, 0.0, 0.0};
    double elev = ta.getTerrainElevation(q);
    EXPECT_DOUBLE_EQ(elev, 500.0);

    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

TEST(TerrainAwarenessCsvTest, NearestNeighborChoice) {
    TerrainAwareness ta;
    auto tmp = std::filesystem::temp_directory_path() / "ta_csv_test2.csv";
    std::ofstream ofs(tmp.string());
    ASSERT_TRUE(ofs.is_open());
    ofs << "34.000000,-118.000000,500.0\n";
    ofs << "35.000000,-119.000000,1500.0\n";
    ofs.close();

    ASSERT_TRUE(ta.loadTerrainDatabase(tmp.string()));

    Position q{34.0001, -118.0001, 0.0, 0.0};
    double elev = ta.getTerrainElevation(q);
    EXPECT_DOUBLE_EQ(elev, 500.0);

    std::error_code ec;
    std::filesystem::remove(tmp, ec);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#pragma once

#include "airport_data.hpp"
#include "navdata_provider.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace AICopilot {
namespace Integration {

/**
 * AirportManager is responsible for owning airport infrastructure data
 * (runways, taxiways, parking, procedures) and synchronising it with
 * optional navigation data providers.
 */
class AirportManager {
public:
    AirportManager();
    explicit AirportManager(std::shared_ptr<INavdataProvider> provider);

    void set_navdata_provider(std::shared_ptr<INavdataProvider> provider);

    bool initialize(const std::string& icao_code, const Airport::LatLonAlt& reference_point);

    bool is_initialized() const { return airport_.has_value(); }

    bool load_from_navdata();

    void set_name(const std::string& name);
    void set_elevation(double elevation_feet);
    void set_controlled(bool controlled);

    void set_runways(const std::vector<Airport::Runway>& runways);
    void add_runway(const Airport::Runway& runway);

    void set_taxiway_network(const Airport::TaxiwayNetwork& network);
    void set_parking_positions(const std::vector<Airport::ParkingPosition>& parking_positions);
    void set_procedures(const std::map<std::string, std::vector<Airport::SIDSTARProcedure>>& procedures);

    Airport::AirportMaster* get_airport();
    const Airport::AirportMaster* get_airport_const() const;

    const std::string& get_icao() const { return icao_code_; }
    const Airport::LatLonAlt& get_reference_point() const { return reference_point_; }
    double get_elevation() const { return elevation_feet_; }
    bool is_controlled() const { return controlled_; }

private:
    void rebuild_airport();

    std::shared_ptr<INavdataProvider> provider_;
    std::string icao_code_;
    std::string name_;
    Airport::LatLonAlt reference_point_;
    double elevation_feet_;
    bool controlled_;

    std::vector<Airport::Runway> runways_;
    Airport::TaxiwayNetwork taxiway_network_;
    std::vector<Airport::ParkingPosition> parking_positions_;
    std::map<std::string, std::vector<Airport::SIDSTARProcedure>> procedures_;

    std::optional<Airport::AirportMaster> airport_;
};

} // namespace Integration
} // namespace AICopilot

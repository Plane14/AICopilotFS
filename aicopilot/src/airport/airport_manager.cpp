#include "../../include/airport_manager.h"
#include "../../include/aicopilot_types.h"
#include <utility>

namespace AICopilot {
namespace Integration {

AirportManager::AirportManager()
    : provider_(nullptr)
    , elevation_feet_(0.0)
    , controlled_(false) {
}

AirportManager::AirportManager(std::shared_ptr<INavdataProvider> provider)
    : provider_(std::move(provider))
    , elevation_feet_(0.0)
    , controlled_(false) {
}

void AirportManager::set_navdata_provider(std::shared_ptr<INavdataProvider> provider) {
    provider_ = std::move(provider);
}

bool AirportManager::initialize(const std::string& icao_code, const Airport::LatLonAlt& reference_point) {
    icao_code_ = icao_code;
    reference_point_ = reference_point;
    airport_.reset();

    if (provider_ && provider_->isReady()) {
        load_from_navdata();
    }

    rebuild_airport();
    return airport_.has_value();
}

bool AirportManager::load_from_navdata() {
    if (!provider_ || !provider_->isReady()) {
        return false;
    }

    AirportInfo info;
    if (!provider_->getAirportByICAO(icao_code_, info)) {
        return false;
    }

    name_ = info.name;
    elevation_feet_ = info.elevation;
    controlled_ = info.towered;

    if (info.position.latitude != 0.0 || info.position.longitude != 0.0) {
        reference_point_ = Airport::LatLonAlt(info.position.latitude, info.position.longitude, info.elevation);
    }

    rebuild_airport();
    return true;
}

void AirportManager::set_name(const std::string& name) {
    name_ = name;
    rebuild_airport();
}

void AirportManager::set_elevation(double elevation_feet) {
    elevation_feet_ = elevation_feet;
    rebuild_airport();
}

void AirportManager::set_controlled(bool controlled) {
    controlled_ = controlled;
    rebuild_airport();
}

void AirportManager::set_runways(const std::vector<Airport::Runway>& runways) {
    runways_ = runways;
    rebuild_airport();
}

void AirportManager::add_runway(const Airport::Runway& runway) {
    runways_.push_back(runway);
    rebuild_airport();
}

void AirportManager::set_taxiway_network(const Airport::TaxiwayNetwork& network) {
    taxiway_network_ = network;
    rebuild_airport();
}

void AirportManager::set_parking_positions(const std::vector<Airport::ParkingPosition>& parking_positions) {
    parking_positions_ = parking_positions;
    rebuild_airport();
}

void AirportManager::set_procedures(const std::map<std::string, std::vector<Airport::SIDSTARProcedure>>& procedures) {
    procedures_ = procedures;
    rebuild_airport();
}

Airport::AirportMaster* AirportManager::get_airport() {
    return airport_ ? &(*airport_) : nullptr;
}

const Airport::AirportMaster* AirportManager::get_airport_const() const {
    return airport_ ? &(*airport_) : nullptr;
}

void AirportManager::rebuild_airport() {
    if (icao_code_.empty()) {
        airport_.reset();
        return;
    }

    Airport::AirportMaster master(0, icao_code_);
    master.set_reference_point(reference_point_);
    master.set_elevation(elevation_feet_);
    master.set_controlled(controlled_);

    for (const auto& runway : runways_) {
        master.add_runway(runway);
    }

    auto& network = master.get_taxiway_network();
    network = taxiway_network_;

    for (const auto& parking : parking_positions_) {
        master.add_parking(parking);
    }

    for (const auto& [runway_ident, proc_list] : procedures_) {
        for (const auto& proc : proc_list) {
            master.add_procedure(proc);
        }
    }

    airport_ = std::move(master);
}

} // namespace Integration
} // namespace AICopilot

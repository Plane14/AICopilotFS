#include "../include/elevation_data.h"
#include "../include/weather_data.h"
#include "../include/runway_database.h"
#include "../include/navdata.h"
#include "../include/input_validation.h"

int main() {
    AICopilot::ElevationData elevation;
    double elev = elevation.GetElevationAt(40.35, -105.54);

    AICopilot::WeatherData weather;
    auto wx = weather.GetWeatherAt("KJFK", 0);
    double temp = weather.GetTemperature("KJFK");

    AICopilot::RunwayDatabase runway_db;
    auto runways = runway_db.GetRunwayInfo("KJFK");
    auto primary = runway_db.GetPrimaryRunway("KJFK");

    AICopilot::NavData navdata;
    auto waypoint = navdata.GetWaypointInfo("KUJOE");
    auto airway = navdata.GetAirwayPoints("V1");

    AICopilot::ValidationResult coord_result = 
        AICopilot::InputValidation::ValidateCoordinates(40.0, -74.0);
    AICopilot::ValidationResult alt_result = 
        AICopilot::InputValidation::ValidateAltitude(5000.0);
    AICopilot::ValidationResult speed_result = 
        AICopilot::InputValidation::ValidateSpeed(250.0);

    std::vector<std::string> waypoints = {"KJFK", "KUJOE", "KLAX"};
    AICopilot::ValidationResult plan_result = 
        AICopilot::InputValidation::ValidateFlightPlan(waypoints);

    return 0;
}

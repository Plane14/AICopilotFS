#pragma once

#include <vector>
#include <map>
#include <memory>
#include <cmath>
#include <queue>
#include <limits>

// ============================================================================
// PART 1: AIRPORT DATA STRUCTURES
// ============================================================================

namespace AICopilot {
namespace Airport {

// Forward declarations
struct Runway;
struct Taxiway;
struct ParkingPosition;
struct SIDSTARProcedure;
class TaxiwayNetwork;
class AirportMaster;

// Constants
constexpr double FEET_TO_METERS = 0.3048;
constexpr double METERS_TO_FEET = 3.28084;
constexpr double EARTH_RADIUS_M = 6371000.0;
constexpr double DEG_TO_RAD = M_PI / 180.0;
constexpr double RAD_TO_DEG = 180.0 / M_PI;

// ============================================================================
// Geometric Types
// ============================================================================

struct Vector2D {
    double x, y;
    
    Vector2D(double x = 0.0, double y = 0.0) : x(x), y(y) {}
    
    Vector2D operator+(const Vector2D& v) const {
        return Vector2D(x + v.x, y + v.y);
    }
    
    Vector2D operator-(const Vector2D& v) const {
        return Vector2D(x - v.x, y - v.y);
    }
    
    Vector2D operator*(double scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }
    
    double dot(const Vector2D& v) const {
        return x * v.x + y * v.y;
    }
    
    double cross(const Vector2D& v) const {
        return x * v.y - y * v.x;
    }
    
    double magnitude() const {
        return std::sqrt(x * x + y * y);
    }
    
    Vector2D normalize() const {
        double mag = magnitude();
        if (mag < 1e-10) return Vector2D(0, 0);
        return Vector2D(x / mag, y / mag);
    }
    
    double distance_to(const Vector2D& v) const {
        return (*this - v).magnitude();
    }
};

struct LatLonAlt {
    double latitude;   // degrees
    double longitude;  // degrees
    double altitude;   // feet MSL
    
    LatLonAlt(double lat = 0.0, double lon = 0.0, double alt = 0.0)
        : latitude(lat), longitude(lon), altitude(alt) {}
    
    // Convert to ECEF (Earth-Centered, Earth-Fixed) cartesian coordinates
    Vector2D to_local_xy(const LatLonAlt& reference) const {
        double lat_rad = latitude * DEG_TO_RAD;
        double lon_rad = longitude * DEG_TO_RAD;
        double ref_lat_rad = reference.latitude * DEG_TO_RAD;
        double ref_lon_rad = reference.longitude * DEG_TO_RAD;
        
        double dlat = (latitude - reference.latitude) * DEG_TO_RAD;
        double dlon = (longitude - reference.longitude) * DEG_TO_RAD;
        
        double x = dlon * EARTH_RADIUS_M * std::cos(ref_lat_rad);
        double y = dlat * EARTH_RADIUS_M;
        
        return Vector2D(x, y);
    }
    
    double distance_to(const LatLonAlt& other) const {
        double lat_rad = latitude * DEG_TO_RAD;
        double lon_rad = longitude * DEG_TO_RAD;
        double other_lat_rad = other.latitude * DEG_TO_RAD;
        double other_lon_rad = other.longitude * DEG_TO_RAD;
        
        double dlat = other_lat_rad - lat_rad;
        double dlon = other_lon_rad - lon_rad;
        
        double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) +
                   std::cos(lat_rad) * std::cos(other_lat_rad) *
                   std::sin(dlon / 2.0) * std::sin(dlon / 2.0);
        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
        
        return EARTH_RADIUS_M * c * METERS_TO_FEET;
    }
};

// ============================================================================
// PART 1.1: Runway Structure
// ============================================================================

struct Runway {
    enum class Surface {
        Asphalt,
        Concrete,
        Grass,
        Water,
        Unknown
    };
    
    enum class Lighting {
        None,
        PartialMediumIntensity,
        FullMediumIntensity,
        FullHighIntensity
    };
    
    // Basic properties
    int runway_number;                  // 1-36, 18L = 180°, 09R = 090°
    std::string runway_ident;           // e.g., "18L", "09R"
    
    // Position and orientation
    LatLonAlt threshold_lat_lon;        // Runway threshold coordinates
    double heading_true;                // Magnetic heading in degrees (0-360)
    
    // Dimensions
    double length_feet;
    double width_feet;
    
    // Surface and lighting
    Surface surface_type;
    Lighting lighting_type;
    bool has_ils;                       // Instrument Landing System
    bool has_glideslope;
    
    // Operational data
    int displaced_threshold_feet;       // For landing only
    bool is_active;
    
    Runway(int num = 0, const std::string& ident = "")
        : runway_number(num), runway_ident(ident), heading_true(0.0),
          length_feet(0.0), width_feet(0.0), surface_type(Surface::Asphalt),
          lighting_type(Lighting::None), has_ils(false), has_glideslope(false),
          displaced_threshold_feet(0), is_active(true) {}
    
    // Get the reciprocal runway heading (opposite direction)
    double get_reciprocal_heading() const {
        double reciprocal = heading_true + 180.0;
        if (reciprocal >= 360.0) reciprocal -= 360.0;
        return reciprocal;
    }
    
    // Calculate wind component aligned with runway
    // positive = tailwind, negative = headwind
    double calculate_wind_component(double wind_direction, double wind_speed) const {
        double angle_diff = wind_direction - heading_true;
        while (angle_diff > 180.0) angle_diff -= 360.0;
        while (angle_diff < -180.0) angle_diff += 360.0;
        return wind_speed * std::sin(angle_diff * DEG_TO_RAD);
    }
    
    // Calculate crosswind component
    double calculate_crosswind_component(double wind_direction, double wind_speed) const {
        double angle_diff = wind_direction - heading_true;
        while (angle_diff > 180.0) angle_diff -= 360.0;
        while (angle_diff < -180.0) angle_diff += 360.0;
        return wind_speed * std::cos(angle_diff * DEG_TO_RAD);
    }
};

// ============================================================================
// PART 1.2: Taxiway Network (Nodes and Edges)
// ============================================================================

// Node: Intersection or junction on the taxiway network
struct TaxiwayNode {
    int node_id;
    LatLonAlt position;
    std::string name;                   // e.g., "A1", "B", "Runway 18L Hold"
    
    enum class NodeType {
        Intersection,
        RunwayHold,
        ParkingArea,
        ServiceArea,
        DeicingPad,
        StartupArea
    };
    
    NodeType type;
    
    TaxiwayNode(int id = 0, const LatLonAlt& pos = LatLonAlt())
        : node_id(id), position(pos), type(NodeType::Intersection) {}
};

// Edge: Taxiway segment connecting two nodes
struct TaxiwayEdge {
    int edge_id;
    int from_node_id;
    int to_node_id;
    double length_feet;                 // Distance along taxiway
    double max_speed_knots;             // Maximum safe speed (typically 15-25 knots)
    bool is_bidirectional;              // Most taxiways are bidirectional
    
    enum class Surface {
        Asphalt,
        Concrete,
        Gravel,
        Grass
    };
    
    Surface surface;
    int width_feet;
    bool requires_caution;              // Tight turns, etc.
    
    TaxiwayEdge(int id = 0, int from = 0, int to = 0, double len = 0.0)
        : edge_id(id), from_node_id(from), to_node_id(to), length_feet(len),
          max_speed_knots(20.0), is_bidirectional(true), surface(Surface::Asphalt),
          width_feet(75), requires_caution(false) {}
    
    // Calculate traversal time in seconds
    double calculate_traversal_time_seconds(double max_aircraft_speed_knots = 15.0) const {
        double speed = std::min(max_speed_knots, max_aircraft_speed_knots);
        if (speed < 1.0) return std::numeric_limits<double>::infinity();
        double feet_per_second = speed * 1.6878;  // knots to ft/s
        return length_feet / feet_per_second;
    }
};

// ============================================================================
// PART 1.3: Taxiway Network Class
// ============================================================================

class TaxiwayNetwork {
private:
    std::map<int, TaxiwayNode> nodes;
    std::map<int, TaxiwayEdge> edges;
    std::map<int, std::vector<int>> adjacency_list;  // node_id -> [edge_ids]
    
public:
    TaxiwayNetwork() = default;
    
    // Node operations
    void add_node(const TaxiwayNode& node) {
        nodes[node.node_id] = node;
    }
    
    TaxiwayNode* get_node(int node_id) {
        auto it = nodes.find(node_id);
        return (it != nodes.end()) ? &it->second : nullptr;
    }
    
    const TaxiwayNode* get_node_const(int node_id) const {
        auto it = nodes.find(node_id);
        return (it != nodes.end()) ? &it->second : nullptr;
    }
    
    // Edge operations
    void add_edge(const TaxiwayEdge& edge) {
        edges[edge.edge_id] = edge;
        adjacency_list[edge.from_node_id].push_back(edge.edge_id);
        if (edge.is_bidirectional) {
            // Create reverse edge
            TaxiwayEdge reverse_edge = edge;
            reverse_edge.edge_id = edge.edge_id + 100000;  // Offset for reverse
            reverse_edge.from_node_id = edge.to_node_id;
            reverse_edge.to_node_id = edge.from_node_id;
            edges[reverse_edge.edge_id] = reverse_edge;
            adjacency_list[edge.to_node_id].push_back(reverse_edge.edge_id);
        }
    }
    
    TaxiwayEdge* get_edge(int edge_id) {
        auto it = edges.find(edge_id);
        return (it != edges.end()) ? &it->second : nullptr;
    }
    
    const std::vector<int>& get_adjacent_edges(int node_id) const {
        static const std::vector<int> empty;
        auto it = adjacency_list.find(node_id);
        return (it != adjacency_list.end()) ? it->second : empty;
    }
    
    // Query operations
    size_t get_node_count() const { return nodes.size(); }
    size_t get_edge_count() const { return edges.size(); }
    
    std::vector<int> get_all_node_ids() const {
        std::vector<int> ids;
        for (const auto& pair : nodes) {
            ids.push_back(pair.first);
        }
        return ids;
    }
};

// ============================================================================
// PART 1.4: Parking Position
// ============================================================================

struct ParkingPosition {
    int parking_id;
    LatLonAlt location;
    
    enum class Type {
        Ramp,
        Gate,
        MilitaryRamp,
        Cargo,
        Hangar,
        TieDown,
        Unknown
    };
    
    Type type;
    
    enum class Pushback {
        Left,
        Right,
        Both,
        None
    };
    
    Pushback pushback_direction;
    
    // Gate information
    std::string gate_name;              // e.g., "A1", "B42"
    bool has_jetway;
    double jetway_offset_feet;          // Lateral offset to parking spot
    
    // Parking spot constraints
    double aircraft_type_width_feet;    // Min width for aircraft
    double max_wingspan_feet;           // Max wingspan that fits
    int num_parking_spaces;             // For ramps
    
    // Power and services
    bool has_electrical;
    bool has_water;
    bool has_lavatory_service;
    bool has_catering;
    bool has_fueling;
    
    // Operational
    bool is_occupied;
    int occupied_by_aircraft_id;        // 0 if empty
    
    ParkingPosition(int id = 0, const LatLonAlt& loc = LatLonAlt())
        : parking_id(id), location(loc), type(Type::Ramp),
          pushback_direction(Pushback::Both), has_jetway(false),
          jetway_offset_feet(0.0), aircraft_type_width_feet(150.0),
          max_wingspan_feet(200.0), num_parking_spaces(1),
          has_electrical(false), has_water(false),
          has_lavatory_service(false), has_catering(false),
          has_fueling(false), is_occupied(false),
          occupied_by_aircraft_id(0) {}
};

// ============================================================================
// PART 1.5: SID/STAR Procedures
// ============================================================================

struct ProcedureWaypoint {
    std::string name;
    LatLonAlt position;
    double altitude_feet;               // Cleared altitude
    double speed_knots;                 // Recommended speed restriction
    enum class TurnDirection {
        Straight,
        LeftTurn,
        RightTurn
    };
    TurnDirection turn_at_waypoint;
    int sequence_number;                // Order in procedure
    
    ProcedureWaypoint(const std::string& n = "", const LatLonAlt& pos = LatLonAlt())
        : name(n), position(pos), altitude_feet(0.0), speed_knots(0.0),
          turn_at_waypoint(TurnDirection::Straight), sequence_number(0) {}
};

struct SIDSTARProcedure {
    std::string procedure_name;         // e.g., "DEPARTURE1", "ARRIVAL_NORTH"
    std::string runway_ident;           // Empty if applies to all runways
    
    enum class Type {
        SID,                            // Standard Instrument Departure
        STAR,                           // Standard Terminal Arrival Route
        Approach                        // Approach procedure
    };
    
    Type procedure_type;
    
    std::vector<ProcedureWaypoint> waypoints;
    
    enum class TransitionType {
        None,
        RwyTransition,                  // Runway transition
        EnrouteTransition,              // To enroute structure
        AppTransition                   // To approach
    };
    
    TransitionType transition_type;
    
    SIDSTARProcedure(const std::string& name = "")
        : procedure_name(name), procedure_type(Type::SID),
          transition_type(TransitionType::None) {}
    
    void add_waypoint(const ProcedureWaypoint& wp) {
        waypoints.push_back(wp);
    }
    
    size_t get_waypoint_count() const {
        return waypoints.size();
    }
};

// ============================================================================
// PART 1.6: Airport Master Class
// ============================================================================

class AirportMaster {
private:
    // Airport identification
    int airport_id;
    std::string icao_code;              // e.g., "KJFK"
    std::string iata_code;              // e.g., "JFK"
    std::string name;                   // Full name
    
    // Physical location
    LatLonAlt reference_point;          // Phase reference for local coordinates
    double elevation_feet;
    
    // Operational data
    std::vector<Runway> runways;
    TaxiwayNetwork taxiway_network;
    std::vector<ParkingPosition> parking_positions;
    std::map<std::string, std::vector<SIDSTARProcedure>> procedures;
    
    // Traffic management
    bool is_controlled;
    int number_of_towers;
    
public:
    AirportMaster(int id = 0, const std::string& icao = "")
        : airport_id(id), icao_code(icao), elevation_feet(0.0),
          is_controlled(false), number_of_towers(1) {}
    
    // Setters
    void set_reference_point(const LatLonAlt& ref) { reference_point = ref; }
    void set_elevation(double elev_ft) { elevation_feet = elev_ft; }
    void set_controlled(bool controlled) { is_controlled = controlled; }
    
    // Runway management
    void add_runway(const Runway& runway) {
        runways.push_back(runway);
    }
    
    const std::vector<Runway>& get_runways() const {
        return runways;
    }
    
    Runway* get_runway(const std::string& ident) {
        for (auto& rwy : runways) {
            if (rwy.runway_ident == ident) return &rwy;
        }
        return nullptr;
    }
    
    // Taxiway network
    TaxiwayNetwork& get_taxiway_network() {
        return taxiway_network;
    }
    
    const TaxiwayNetwork& get_taxiway_network_const() const {
        return taxiway_network;
    }
    
    // Parking positions
    void add_parking(const ParkingPosition& parking) {
        parking_positions.push_back(parking);
    }
    
    const std::vector<ParkingPosition>& get_parking_positions() const {
        return parking_positions;
    }
    
    ParkingPosition* get_parking(int parking_id) {
        for (auto& parking : parking_positions) {
            if (parking.parking_id == parking_id) return &parking;
        }
        return nullptr;
    }
    
    // SID/STAR procedures
    void add_procedure(const SIDSTARProcedure& proc) {
        procedures[proc.runway_ident].push_back(proc);
    }
    
    std::vector<SIDSTARProcedure>* get_procedures(const std::string& runway_ident) {
        auto it = procedures.find(runway_ident);
        return (it != procedures.end()) ? &it->second : nullptr;
    }
    
    // Accessors
    const std::string& get_icao() const { return icao_code; }
    const std::string& get_name() const { return name; }
    const LatLonAlt& get_reference_point() const { return reference_point; }
    double get_elevation() const { return elevation_feet; }
    bool is_atc_controlled() const { return is_controlled; }
};

} // namespace Airport
} // namespace AICopilot

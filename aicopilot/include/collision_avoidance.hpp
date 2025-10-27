#pragma once

#include "airport_data.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

// ============================================================================
// PART 3: COLLISION AVOIDANCE SYSTEMS
// ============================================================================

namespace AICopilot {
namespace Collision {

using namespace Airport;

// ============================================================================
// PART 3.1: Geometric Collision Detection
// ============================================================================

struct Circle {
    Vector2D center;
    double radius;
    
    Circle(const Vector2D& c = Vector2D(), double r = 0.0)
        : center(c), radius(r) {}
    
    bool contains_point(const Vector2D& point) const {
        return center.distance_to(point) <= radius;
    }
    
    bool intersects_circle(const Circle& other) const {
        double dist = center.distance_to(other.center);
        return dist < (radius + other.radius);
    }
};

struct Polygon {
    std::vector<Vector2D> vertices;
    
    Polygon() = default;
    
    void add_vertex(const Vector2D& v) {
        vertices.push_back(v);
    }
    
    bool contains_point(const Vector2D& point) const {
        if (vertices.size() < 3) return false;
        
        // Ray casting algorithm
        int intersections = 0;
        for (size_t i = 0; i < vertices.size(); ++i) {
            const Vector2D& v1 = vertices[i];
            const Vector2D& v2 = vertices[(i + 1) % vertices.size()];
            
            if ((v1.y <= point.y && point.y < v2.y) ||
                (v2.y <= point.y && point.y < v1.y))
            {
                double x_intersect = v1.x + (point.y - v1.y) / (v2.y - v1.y) * (v2.x - v1.x);
                if (point.x < x_intersect) {
                    intersections++;
                }
            }
        }
        
        return (intersections % 2) == 1;
    }
};

class CollisionDetector {
public:
    // Circle-circle collision (aircraft collision at same altitude)
    static bool check_circle_circle_collision(
        const Circle& circle1,
        const Circle& circle2)
    {
        return circle1.intersects_circle(circle2);
    }
    
    // Circle-polygon collision (aircraft vs airport infrastructure)
    static bool check_circle_polygon_collision(
        const Circle& circle,
        const Polygon& polygon)
    {
        // Check if circle center is in polygon
        if (polygon.contains_point(circle.center)) {
            return true;
        }
        
        // Check if any polygon vertex is in circle
        for (const auto& vertex : polygon.vertices) {
            if (circle.contains_point(vertex)) {
                return true;
            }
        }
        
        // Check if circle intersects any polygon edge
        for (size_t i = 0; i < polygon.vertices.size(); ++i) {
            const Vector2D& v1 = polygon.vertices[i];
            const Vector2D& v2 = polygon.vertices[(i + 1) % polygon.vertices.size()];
            
            double dist = distance_point_to_line_segment(circle.center, v1, v2);
            if (dist <= circle.radius) {
                return true;
            }
        }
        
        return false;
    }
    
    // Polygon-polygon collision
    static bool check_polygon_polygon_collision(
        const Polygon& poly1,
        const Polygon& poly2)
    {
        // Separating Axis Theorem (SAT)
        
        // Check all edges of poly1
        for (size_t i = 0; i < poly1.vertices.size(); ++i) {
            Vector2D edge = poly1.vertices[(i + 1) % poly1.vertices.size()] - poly1.vertices[i];
            Vector2D axis(-edge.y, edge.x);  // Perpendicular
            axis = axis.normalize();
            
            if (is_separating_axis(poly1, poly2, axis)) {
                return false;  // No collision
            }
        }
        
        // Check all edges of poly2
        for (size_t i = 0; i < poly2.vertices.size(); ++i) {
            Vector2D edge = poly2.vertices[(i + 1) % poly2.vertices.size()] - poly2.vertices[i];
            Vector2D axis(-edge.y, edge.x);
            axis = axis.normalize();
            
            if (is_separating_axis(poly1, poly2, axis)) {
                return false;
            }
        }
        
        return true;  // Collision detected
    }
    
private:
    static double distance_point_to_line_segment(
        const Vector2D& point,
        const Vector2D& line_start,
        const Vector2D& line_end)
    {
        Vector2D pa = point - line_start;
        Vector2D ba = line_end - line_start;
        
        double h = pa.dot(ba) / ba.dot(ba);
        h = std::max(0.0, std::min(1.0, h));
        
        return (pa - ba * h).magnitude();
    }
    
    static bool is_separating_axis(
        const Polygon& poly1,
        const Polygon& poly2,
        const Vector2D& axis)
    {
        // Project poly1 onto axis
        double min1 = poly1.vertices[0].dot(axis);
        double max1 = min1;
        for (const auto& v : poly1.vertices) {
            double proj = v.dot(axis);
            min1 = std::min(min1, proj);
            max1 = std::max(max1, proj);
        }
        
        // Project poly2 onto axis
        double min2 = poly2.vertices[0].dot(axis);
        double max2 = min2;
        for (const auto& v : poly2.vertices) {
            double proj = v.dot(axis);
            min2 = std::min(min2, proj);
            max2 = std::max(max2, proj);
        }
        
        // Check for gap
        return max1 < min2 || max2 < min1;
    }
};

// ============================================================================
// PART 3.2: Separation Standards
// ============================================================================

class SeparationStandards {
public:
    enum class ConflictType {
        None,
        HeadOn,                        // Aircraft on collision course
        Parallel,                      // Flying parallel paths too close
        Crossing,                      // Crossing paths
        SameAltitude,                  // At same altitude laterally close
        VerticalConflict,              // Vertically too close
        Converging,                    // Converging towards each other
        Overtaking                     // Aircraft behind catching up too fast
    };
    
    struct AircraftState {
        int aircraft_id;
        Vector2D position_local;        // Local X,Y feet
        Vector2D velocity;              // X,Y ft/sec
        double altitude_feet;
        double heading_true;
        double groundspeed_knots;
        double wingspan_feet;
        double length_feet;
        
        AircraftState()
            : aircraft_id(-1), altitude_feet(0.0), heading_true(0.0),
              groundspeed_knots(0.0), wingspan_feet(100.0), length_feet(100.0) {}
    };
    
private:
    // Minimum separation standards (feet)
    double lateral_separation_minimum;  // Side by side: 500 ft
    double vertical_separation_minimum; // Different altitudes: 1000 ft
    double longitudinal_separation;     // Head to tail: 1000 ft
    double crossing_separation;         // Crossing paths: 500 ft
    
    // Low altitude protection (within 5nm of airport)
    double low_altitude_lateral_sep;    // Often reduced to 300-500 ft
    double low_altitude_vertical_sep;   // Often reduced to 500 ft
    
public:
    SeparationStandards()
        : lateral_separation_minimum(500.0),
          vertical_separation_minimum(1000.0),
          longitudinal_separation(1000.0),
          crossing_separation(500.0),
          low_altitude_lateral_sep(300.0),
          low_altitude_vertical_sep(500.0) {}
    
    void set_separation_standards(
        double lateral,
        double vertical,
        double longitudinal,
        double crossing)
    {
        lateral_separation_minimum = lateral;
        vertical_separation_minimum = vertical;
        longitudinal_separation = longitudinal;
        crossing_separation = crossing;
    }
    
    void set_low_altitude_standards(double lateral, double vertical) {
        low_altitude_lateral_sep = lateral;
        low_altitude_vertical_sep = vertical;
    }
    
    // Check if two aircraft violate separation standards
    ConflictType check_separation_conflict(
        const AircraftState& aircraft1,
        const AircraftState& aircraft2,
        bool is_low_altitude_area = false) const
    {
        // Use low altitude standards if applicable
        double lat_sep = is_low_altitude_area ? low_altitude_lateral_sep : lateral_separation_minimum;
        double vert_sep = is_low_altitude_area ? low_altitude_vertical_sep : vertical_separation_minimum;
        
        // Calculate 3D distance
        Vector2D delta_pos = aircraft2.position_local - aircraft1.position_local;
        double altitude_diff = std::abs(aircraft2.altitude_feet - aircraft1.altitude_feet);
        double horizontal_distance = delta_pos.magnitude();
        
        // Check vertical separation first
        if (altitude_diff < vert_sep) {
            if (horizontal_distance < lat_sep) {
                return ConflictType::SameAltitude;
            }
        }
        
        // Determine conflict type based on relative motion
        double heading_diff = aircraft2.heading_true - aircraft1.heading_true;
        while (heading_diff > 180.0) heading_diff -= 360.0;
        while (heading_diff < -180.0) heading_diff += 360.0;
        
        // Head-on: opposite headings (within 30 degrees)
        if (std::abs(heading_diff) < 30.0 || std::abs(heading_diff) > 330.0) {
            if (horizontal_distance < crossing_separation) {
                return ConflictType::HeadOn;
            }
        }
        
        // Parallel: similar headings (within 15 degrees)
        if (std::abs(heading_diff) < 15.0 || std::abs(std::abs(heading_diff) - 360.0) < 15.0) {
            if (horizontal_distance < lat_sep) {
                return ConflictType::Parallel;
            }
        }
        
        // Crossing: perpendicular-ish
        if ((std::abs(heading_diff - 90.0) < 30.0) ||
            (std::abs(heading_diff - 270.0) < 30.0))
        {
            if (horizontal_distance < crossing_separation) {
                return ConflictType::Crossing;
            }
        }
        
        // Check convergence
        Vector2D rel_velocity = aircraft2.velocity - aircraft1.velocity;
        double range_rate = delta_pos.normalize().dot(rel_velocity);
        
        if (range_rate < 0.0 && horizontal_distance < 2000.0) {  // Converging
            if (horizontal_distance < lat_sep) {
                return ConflictType::Converging;
            }
        }
        
        return ConflictType::None;
    }
    
    // Calculate closest point of approach
    double calculate_closest_point_of_approach(
        const AircraftState& aircraft1,
        const AircraftState& aircraft2,
        double& time_to_cpa_seconds) const
    {
        Vector2D delta_pos = aircraft2.position_local - aircraft1.position_local;
        Vector2D delta_vel = aircraft2.velocity - aircraft1.velocity;
        
        // Solve for time when distance is minimum
        // d(t) = |P1 + V1*t - P2 - V2*t|^2
        // d(t) = |(dP + dV*t)|^2
        // dd/dt = 2*(dP + dV*t)*dV = 0
        // t = -dP·dV / dV·dV
        
        double a = delta_vel.dot(delta_vel);
        if (std::abs(a) < 1e-10) {
            time_to_cpa_seconds = 0.0;
            return delta_pos.magnitude();
        }
        
        double b = delta_pos.dot(delta_vel);
        time_to_cpa_seconds = -b / a;
        
        if (time_to_cpa_seconds < 0.0) {
            time_to_cpa_seconds = 0.0;  // Already passed
        }
        
        // Distance at CPA
        Vector2D pos_at_cpa = delta_pos + delta_vel * time_to_cpa_seconds;
        return pos_at_cpa.magnitude();
    }
    
    // Predict if collision will occur in next N seconds
    bool predict_collision(
        const AircraftState& aircraft1,
        const AircraftState& aircraft2,
        double prediction_horizon_seconds,
        double& time_to_conflict) const
    {
        double time_to_cpa;
        double distance_at_cpa = calculate_closest_point_of_approach(
            aircraft1, aircraft2, time_to_cpa);
        
        if (time_to_cpa > prediction_horizon_seconds) {
            return false;  // Outside prediction horizon
        }
        
        // Check if minimum separation is violated at CPA
        double lat_sep = lateral_separation_minimum;
        
        time_to_conflict = time_to_cpa;
        return distance_at_cpa < lat_sep;
    }
};

// ============================================================================
// PART 3.3: Conflict Prediction (30 second lookahead)
// ============================================================================

class ConflictPredictor {
private:
    std::vector<SeparationStandards::AircraftState> aircraft_states;
    double prediction_horizon_seconds;
    
public:
    struct ConflictAlert {
        int aircraft1_id;
        int aircraft2_id;
        double time_to_conflict_seconds;
        double minimum_separation_at_conflict_feet;
        SeparationStandards::ConflictType conflict_type;
        Vector2D predicted_conflict_position;
        
        ConflictAlert()
            : aircraft1_id(-1), aircraft2_id(-1), time_to_conflict_seconds(999.0),
              minimum_separation_at_conflict_feet(0.0),
              conflict_type(SeparationStandards::ConflictType::None) {}
    };
    
    ConflictPredictor(double horizon_seconds = 30.0)
        : prediction_horizon_seconds(horizon_seconds) {}
    
    void set_prediction_horizon(double seconds) {
        prediction_horizon_seconds = seconds;
    }
    
    void update_aircraft_state(const SeparationStandards::AircraftState& state) {
        // Update or add aircraft state
        auto it = std::find_if(
            aircraft_states.begin(),
            aircraft_states.end(),
            [&](const SeparationStandards::AircraftState& s) {
                return s.aircraft_id == state.aircraft_id;
            });
        
        if (it != aircraft_states.end()) {
            *it = state;
        } else {
            aircraft_states.push_back(state);
        }
    }
    
    void remove_aircraft(int aircraft_id) {
        aircraft_states.erase(
            std::remove_if(
                aircraft_states.begin(),
                aircraft_states.end(),
                [aircraft_id](const SeparationStandards::AircraftState& s) {
                    return s.aircraft_id == aircraft_id;
                }),
            aircraft_states.end());
    }
    
    // Predict conflicts for all aircraft pairs
    std::vector<ConflictAlert> predict_conflicts() const {
        std::vector<ConflictAlert> alerts;
        SeparationStandards sep_standards;
        
        for (size_t i = 0; i < aircraft_states.size(); ++i) {
            for (size_t j = i + 1; j < aircraft_states.size(); ++j) {
                const auto& ac1 = aircraft_states[i];
                const auto& ac2 = aircraft_states[j];
                
                double time_to_conflict;
                bool conflict = sep_standards.predict_collision(
                    ac1, ac2, prediction_horizon_seconds, time_to_conflict);
                
                if (conflict) {
                    ConflictAlert alert;
                    alert.aircraft1_id = ac1.aircraft_id;
                    alert.aircraft2_id = ac2.aircraft_id;
                    alert.time_to_conflict_seconds = time_to_conflict;
                    alert.conflict_type = sep_standards.check_separation_conflict(ac1, ac2);
                    
                    // Predict position at conflict
                    double t = time_to_conflict;
                    Vector2D pos1_pred = ac1.position_local + ac1.velocity * t;
                    Vector2D pos2_pred = ac2.position_local + ac2.velocity * t;
                    alert.predicted_conflict_position = (pos1_pred + pos2_pred) * 0.5;
                    
                    alerts.push_back(alert);
                }
            }
        }
        
        // Sort by time to conflict (most urgent first)
        std::sort(
            alerts.begin(),
            alerts.end(),
            [](const ConflictAlert& a, const ConflictAlert& b) {
                return a.time_to_conflict_seconds < b.time_to_conflict_seconds;
            });
        
        return alerts;
    }
    
    size_t get_tracked_aircraft_count() const {
        return aircraft_states.size();
    }
};

// ============================================================================
// PART 3.4: Avoidance Maneuver Selection
// ============================================================================

class AvoidanceManeuver {
public:
    enum class ManeuverType {
        None,
        TurnLeft,                      // Turn 15-30 degrees left
        TurnRight,                      // Turn 15-30 degrees right
        ClimbTo,                       // Climb to new altitude
        DescentTo,                     // Descend to new altitude
        SpeedUp,                       // Increase speed by 10-20 knots
        SlowDown,                      // Decrease speed by 10-20 knots
        GoAround,                      // If on approach, go around
        HoldingPattern                 // Enter holding pattern
    };
    
    ManeuverType maneuver_type;
    double new_heading_true;            // For turn maneuvers
    double new_altitude_feet;           // For climb/descent
    double new_speed_knots;             // For speed changes
    double duration_seconds;            // How long to maintain maneuver
    double pilot_workload;              // 0-100, higher is more demanding
    std::string description;
    
    AvoidanceManeuver(ManeuverType type = ManeuverType::None)
        : maneuver_type(type), new_heading_true(0.0), new_altitude_feet(0.0),
          new_speed_knots(0.0), duration_seconds(60.0), pilot_workload(50.0) {}
};

class ManeuverSelector {
private:
    const AirportMaster* airport;
    
    // Constraints
    double max_turn_rate_degrees_per_sec;   // Typically 3 degrees
    double max_climb_rate_fpm;              // Typically 1000-2000 fpm
    double max_descent_rate_fpm;            // Typically 500-1000 fpm
    double max_speed_change_knots_per_sec;  // Typically 1-2 knots/sec
    
public:
    ManeuverSelector()
        : airport(nullptr),
          max_turn_rate_degrees_per_sec(3.0),
          max_climb_rate_fpm(1500.0),
          max_descent_rate_fpm(1000.0),
          max_speed_change_knots_per_sec(1.5) {}
    
    void set_aircraft_limits(
        double turn_rate,
        double climb_rate,
        double descent_rate,
        double speed_change)
    {
        max_turn_rate_degrees_per_sec = turn_rate;
        max_climb_rate_fpm = climb_rate;
        max_descent_rate_fpm = descent_rate;
        max_speed_change_knots_per_sec = speed_change;
    }
    
    // Select best avoidance maneuver
    AvoidanceManeuver select_avoidance_maneuver(
        const SeparationStandards::AircraftState& ownship,
        const SeparationStandards::AircraftState& intruder,
        bool is_departure = false,
        bool is_arrival = false) const
    {
        AvoidanceManeuver best_maneuver(AvoidanceManeuver::ManeuverType::None);
        double best_effectiveness = 0.0;
        
        // Calculate relative bearing to intruder
        Vector2D to_intruder = intruder.position_local - ownship.position_local;
        double bearing_to_intruder = std::atan2(to_intruder.y, to_intruder.x) * RAD_TO_DEG;
        
        // Normalize to 0-360
        while (bearing_to_intruder < 0.0) bearing_to_intruder += 360.0;
        while (bearing_to_intruder >= 360.0) bearing_to_intruder -= 360.0;
        
        // Option 1: Turn maneuver
        {
            // Determine which way to turn
            double bearing_diff = bearing_to_intruder - ownship.heading_true;
            while (bearing_diff > 180.0) bearing_diff -= 360.0;
            while (bearing_diff < -180.0) bearing_diff += 360.0;
            
            bool turn_left = (bearing_diff > 0.0);
            
            for (double turn_angle : {15.0, 30.0, 45.0}) {
                AvoidanceManeuver maneuver(
                    turn_left ? AvoidanceManeuver::ManeuverType::TurnLeft
                              : AvoidanceManeuver::ManeuverType::TurnRight);
                
                maneuver.new_heading_true = ownship.heading_true + (turn_left ? turn_angle : -turn_angle);
                while (maneuver.new_heading_true < 0.0) maneuver.new_heading_true += 360.0;
                while (maneuver.new_heading_true >= 360.0) maneuver.new_heading_true -= 360.0;
                
                // Calculate turn duration
                maneuver.duration_seconds = turn_angle / max_turn_rate_degrees_per_sec;
                maneuver.pilot_workload = 40.0 + (turn_angle * 0.5);
                maneuver.description = (turn_left ? "Turn left " : "Turn right ") + 
                                     std::to_string(static_cast<int>(turn_angle)) + " degrees";
                
                // Evaluate effectiveness (higher is better)
                double effectiveness = evaluate_maneuver_effectiveness(maneuver, ownship, intruder);
                if (effectiveness > best_effectiveness) {
                    best_effectiveness = effectiveness;
                    best_maneuver = maneuver;
                }
            }
        }
        
        // Option 2: Altitude change (if not on ground)
        if (ownship.altitude_feet > 500.0) {
            // Try climbing
            {
                AvoidanceManeuver maneuver(AvoidanceManeuver::ManeuverType::ClimbTo);
                maneuver.new_altitude_feet = ownship.altitude_feet + 500.0;  // Climb 500 ft
                maneuver.duration_seconds = 500.0 / (max_climb_rate_fpm / 60.0);
                maneuver.pilot_workload = 35.0;
                maneuver.description = "Climb to " + std::to_string(static_cast<int>(maneuver.new_altitude_feet)) + " feet";
                
                double effectiveness = evaluate_maneuver_effectiveness(maneuver, ownship, intruder);
                if (effectiveness > best_effectiveness) {
                    best_effectiveness = effectiveness;
                    best_maneuver = maneuver;
                }
            }
            
            // Try descending
            {
                AvoidanceManeuver maneuver(AvoidanceManeuver::ManeuverType::DescentTo);
                maneuver.new_altitude_feet = ownship.altitude_feet - 500.0;  // Descend 500 ft
                maneuver.duration_seconds = 500.0 / (max_descent_rate_fpm / 60.0);
                maneuver.pilot_workload = 35.0;
                maneuver.description = "Descend to " + std::to_string(static_cast<int>(maneuver.new_altitude_feet)) + " feet";
                
                double effectiveness = evaluate_maneuver_effectiveness(maneuver, ownship, intruder);
                if (effectiveness > best_effectiveness) {
                    best_effectiveness = effectiveness;
                    best_maneuver = maneuver;
                }
            }
        }
        
        // Option 3: Speed change
        {
            // Speed up
            {
                AvoidanceManeuver maneuver(AvoidanceManeuver::ManeuverType::SpeedUp);
                maneuver.new_speed_knots = ownship.groundspeed_knots + 15.0;
                maneuver.duration_seconds = 15.0 / max_speed_change_knots_per_sec;
                maneuver.pilot_workload = 30.0;
                maneuver.description = "Speed up by 15 knots";
                
                double effectiveness = evaluate_maneuver_effectiveness(maneuver, ownship, intruder);
                if (effectiveness > best_effectiveness) {
                    best_effectiveness = effectiveness;
                    best_maneuver = maneuver;
                }
            }
            
            // Slow down
            if (ownship.groundspeed_knots > 5.0) {
                AvoidanceManeuver maneuver(AvoidanceManeuver::ManeuverType::SlowDown);
                maneuver.new_speed_knots = std::max(5.0, ownship.groundspeed_knots - 15.0);
                maneuver.duration_seconds = 15.0 / max_speed_change_knots_per_sec;
                maneuver.pilot_workload = 30.0;
                maneuver.description = "Slow down by 15 knots";
                
                double effectiveness = evaluate_maneuver_effectiveness(maneuver, ownship, intruder);
                if (effectiveness > best_effectiveness) {
                    best_effectiveness = effectiveness;
                    best_maneuver = maneuver;
                }
            }
        }
        
        // Option 4: Go around (if approaching)
        if (is_arrival && ownship.altitude_feet < 2000.0) {
            AvoidanceManeuver maneuver(AvoidanceManeuver::ManeuverType::GoAround);
            maneuver.new_altitude_feet = ownship.altitude_feet + 1000.0;
            maneuver.duration_seconds = 60.0;
            maneuver.pilot_workload = 80.0;
            maneuver.description = "Execute go-around";
            
            double effectiveness = evaluate_maneuver_effectiveness(maneuver, ownship, intruder);
            if (effectiveness > best_effectiveness) {
                best_effectiveness = effectiveness;
                best_maneuver = maneuver;
            }
        }
        
        return best_maneuver;
    }
    
private:
    double evaluate_maneuver_effectiveness(
        const AvoidanceManeuver& maneuver,
        const SeparationStandards::AircraftState& ownship,
        const SeparationStandards::AircraftState& intruder) const
    {
        // Higher score = better maneuver
        double score = 50.0;  // Base score
        
        // Reduce score based on pilot workload
        score -= maneuver.pilot_workload * 0.2;
        
        // Add score based on separation improvement
        // (This would require simulating the maneuver and checking new separation)
        score += 20.0;  // Simplified
        
        return score;
    }
};

// ============================================================================
// PART 3.5: Multi-Aircraft Conflict Resolution
// ============================================================================

class ConflictResolver {
private:
    std::map<int, AvoidanceManeuver> resolution_orders;
    const ManeuverSelector* maneuver_selector;
    
public:
    struct ResolutionPlan {
        std::map<int, AvoidanceManeuver> aircraft_maneuvers;
        double plan_effectiveness;      // 0-100
        bool resolves_all_conflicts;
        std::string resolution_strategy;
        
        ResolutionPlan()
            : plan_effectiveness(0.0), resolves_all_conflicts(false) {}
    };
    
    ConflictResolver() : maneuver_selector(nullptr) {}
    
    void set_maneuver_selector(const ManeuverSelector* selector) {
        maneuver_selector = selector;
    }
    
    // Resolve conflicts for multiple aircraft pairs
    ResolutionPlan resolve_multi_aircraft_conflicts(
        const std::vector<ConflictPredictor::ConflictAlert>& conflicts,
        const std::map<int, SeparationStandards::AircraftState>& aircraft_states) const
    {
        ResolutionPlan plan;
        
        if (!maneuver_selector || conflicts.empty()) {
            plan.resolves_all_conflicts = true;
            return plan;
        }
        
        // Process conflicts in priority order (highest priority = most urgent)
        for (const auto& conflict : conflicts) {
            auto ac1_it = aircraft_states.find(conflict.aircraft1_id);
            auto ac2_it = aircraft_states.find(conflict.aircraft2_id);
            
            if (ac1_it == aircraft_states.end() || ac2_it == aircraft_states.end()) {
                continue;
            }
            
            const auto& ac1 = ac1_it->second;
            const auto& ac2 = ac2_it->second;
            
            // Check if maneuvers already assigned
            bool ac1_has_maneuver = plan.aircraft_maneuvers.count(conflict.aircraft1_id) > 0;
            bool ac2_has_maneuver = plan.aircraft_maneuvers.count(conflict.aircraft2_id) > 0;
            
            // Assign maneuvers
            if (!ac1_has_maneuver) {
                AvoidanceManeuver maneuver = maneuver_selector->select_avoidance_maneuver(ac1, ac2);
                if (maneuver.maneuver_type != AvoidanceManeuver::ManeuverType::None) {
                    plan.aircraft_maneuvers[conflict.aircraft1_id] = maneuver;
                }
            }
            
            if (!ac2_has_maneuver) {
                AvoidanceManeuver maneuver = maneuver_selector->select_avoidance_maneuver(ac2, ac1);
                if (maneuver.maneuver_type != AvoidanceManeuver::ManeuverType::None) {
                    plan.aircraft_maneuvers[conflict.aircraft2_id] = maneuver;
                }
            }
        }
        
        plan.plan_effectiveness = 75.0;  // Simplified scoring
        plan.resolves_all_conflicts = (plan.aircraft_maneuvers.size() == conflicts.size() * 2);
        plan.resolution_strategy = "Standard conflict avoidance routing";
        
        return plan;
    }
};

} // namespace Collision
} // namespace AICopilot

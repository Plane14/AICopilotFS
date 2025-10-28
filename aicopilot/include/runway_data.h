/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Runway Data Structures - Airport runway and ILS information
* Provides data types for runway management, ILS systems, and landing support
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#pragma once

#include <string>
#include <vector>

namespace AICopilot {

/**
 * Surface Type Enumeration
 * Runway surface material affects landing distance and friction
 */
enum class SurfaceType {
    ASPHALT = 0,           // Asphalt concrete
    CONCRETE = 1,          // Portland cement concrete
    GRASS = 2,             // Natural grass
    GRAVEL = 3,            // Compacted gravel
    DIRT = 4,              // Compacted dirt
    WATER = 5,             // Water surface (seaplanes)
    MACADAM = 6,           // Macadam
    BITUMINOUS = 7,        // Bituminous
    BRICK = 8,             // Brick or tile
    ASPHALT_CONCRETE = 9,  // AC Mix
    UNKNOWN = 10           // Unknown surface
};

/**
 * ILS Category Enumeration
 * Instrument Landing System categories define landing minimums
 */
enum class ILSCategory {
    NONE = 0,              // No ILS
    CAT_I = 1,             // Category I (200 ft DH, 2400 RVR)
    CAT_II = 2,            // Category II (100 ft DH, 1200 RVR)
    CAT_IIIA = 3,          // Category IIIA (0 ft DH, 700 RVR)
    CAT_IIIB = 4,          // Category IIIB (0 ft DH, 300 RVR)
    CAT_IIIC = 5           // Category IIIC (0 ft DH, RVR not required)
};

/**
 * ILS System Information
 * Complete Instrument Landing System data for runway
 */
struct ILSData {
    bool hasILS = false;                        // Does runway have ILS
    ILSCategory category = ILSCategory::NONE;   // ILS category
    double localizerFrequency = 0.0;            // 108.1-111.95 MHz
    double glideslopeFrequency = 0.0;           // 329.15-335.0 MHz
    int localizerCourse = 0;                    // Magnetic heading 0-360°
    double glideslopeAngle = 3.0;               // Degrees (typically 3.0)
    int decisionHeight = 0;                     // Feet AGL
    int minimumRVR = 0;                         // Runway Visual Range - feet
    
    // ILS identifier
    std::string identifier;                     // e.g., "IJK"
};

/**
 * Runway Information
 * Complete runway and airport data
 */
struct RunwayInfo {
    // Airport information
    std::string icao;                           // ICAO code (e.g., "KJFK")
    std::string airportName;                    // Full airport name
    std::string runwayId;                       // Runway ID (e.g., "04L", "22R", "18")
    
    // Geographic information
    double latitude = 0.0;                      // Decimal degrees N
    double longitude = 0.0;                     // Decimal degrees W (negative)
    int elevation = 0;                          // Field elevation feet MSL
    double magneticVariation = 0.0;             // Magnetic variation degrees (W negative)
    
    // Runway heading
    int headingMagnetic = 0;                    // Magnetic heading 0-360°
    int headingTrue = 0;                        // True heading 0-360°
    
    // Runway dimensions (feet)
    int length = 0;                             // Runway length
    int width = 0;                              // Runway width
    
    // Surface information
    SurfaceType surface = SurfaceType::UNKNOWN; // Surface type
    double frictionCoefficient = 0.5;           // Typical 0.3-0.8 depending on surface
    bool grooved = false;                       // Grooved surface
    bool porous = false;                        // Porous friction course
    
    // Lighting and aids
    bool hasALS = false;                        // Approach Lighting System
    bool hasRunwayLights = false;               // Runway edge lights
    bool hasREIL = false;                       // Runway End Identifier Lights
    bool hasVGSI = false;                       // PAPI/VASI lights
    std::string vgsiType;                       // PAPI or VASI
    
    // Displacement and length restrictions
    bool displaceThreshold = false;             // Has displaced threshold
    int displacedDistance = 0;                  // Feet from threshold
    
    // Distance available (feet) - FAA ICAO codes
    int TORA = 0;                               // Takeoff Run Available
    int TODA = 0;                               // Takeoff Distance Available
    int ASDA = 0;                               // Accelerate-Stop Distance Available
    int LDA = 0;                                // Landing Distance Available
    
    // ILS information
    ILSData ilsData;
    
    // Remarks
    std::string remarks;                        // Additional notes
};

/**
 * Airport Information
 * Airport reference data
 */
struct AirportInfo {
    std::string icao;                           // ICAO code
    std::string iata;                           // IATA code (if available)
    std::string name;                           // Full airport name
    double latitude = 0.0;                      // Decimal degrees N
    double longitude = 0.0;                     // Decimal degrees W (negative)
    int elevation = 0;                          // Field elevation feet MSL
    double magneticVariation = 0.0;             // Magnetic variation degrees (W negative)
    int runwayCount = 0;                        // Number of runways
    bool hasMajorILS = false;                   // Has Cat I or better ILS
    std::string classification;                 // Military, Civil, etc.
    std::vector<std::string> runwayIds;         // List of runway IDs
};

/**
 * Aircraft runway performance data
 * Aircraft-specific performance for runway analysis
 */
struct AircraftPerformance {
    std::string aircraftType;                   // Aircraft type (e.g., "B737")
    double maxCrosswind = 20.0;                 // Maximum crosswind knots (typical 15-25)
    double maxTailwind = 10.0;                  // Maximum tailwind knots (typical 5-10)
    double landingDistance = 3000.0;            // Required landing distance feet (50% rule)
    double takeoffDistance = 3000.0;            // Required takeoff distance feet (15% safety margin)
    int maxAircraftLength = 200;                // Aircraft length feet
    int minRunwayWidth = 100;                   // Minimum acceptable runway width feet
};

/**
 * Runway selection criteria
 * Information for runway selection algorithm
 */
struct RunwaySelectionCriteria {
    // Wind information
    int windDirection = 0;                      // Wind direction 0-360°
    int windSpeed = 0;                          // Wind speed knots
    int maxAcceptableCrosswind = 20;            // Maximum crosswind knots
    double maxAcceptableTailwind = 10.0;        // Maximum tailwind knots
    
    // Aircraft information
    std::string aircraftType;                   // Aircraft type
    double requiredDistance = 3000.0;           // Required runway length
    
    // Selection preference
    bool preferILS = true;                      // Prefer ILS equipped runways
    bool preferLongerRunway = true;             // Prefer longer runways when available
};

/**
 * Runway wind components
 * Calculated wind components for runway
 */
struct RunwayWindComponents {
    double headwind = 0.0;                      // Headwind knots (positive is headwind)
    double crosswind = 0.0;                     // Crosswind knots (positive is from left)
    double tailwind = 0.0;                      // Tailwind knots (positive is tailwind)
    double magnitude = 0.0;                     // Total wind speed
    int direction = 0;                          // Wind direction 0-360°
    int runwayHeading = 0;                      // Runway magnetic heading
};

} // namespace AICopilot

/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Runway Selector - Advanced runway selection algorithm
* Analyzes wind conditions and aircraft capabilities to select best runway
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#pragma once

#include "runway_data.h"
#include <vector>
#include <string>

namespace AICopilot {

/**
 * Runway Selector
 * Advanced runway selection algorithm considering wind, aircraft type, and preferences
 */
class RunwaySelector {
public:
    RunwaySelector();
    ~RunwaySelector();
    
    /**
     * Calculate wind components for runway
     * @param runwayHeading Runway magnetic heading (degrees)
     * @param windDirection Wind direction (degrees)
     * @param windSpeed Wind speed (knots)
     * @return Wind components (headwind, crosswind, tailwind)
     */
    static RunwayWindComponents CalculateWindComponents(
        int runwayHeading,
        int windDirection,
        int windSpeed);
    
    /**
     * Calculate crosswind component
     * @param runwayHeading Runway magnetic heading
     * @param windDirection Wind direction
     * @param windSpeed Wind speed (knots)
     * @return Crosswind in knots (positive = from left, negative = from right)
     */
    static double CalculateCrosswind(int runwayHeading, int windDirection, int windSpeed);
    
    /**
     * Calculate headwind component
     * @param runwayHeading Runway magnetic heading
     * @param windDirection Wind direction
     * @param windSpeed Wind speed (knots)
     * @return Headwind in knots (positive = headwind, negative = tailwind)
     */
    static double CalculateHeadwind(int runwayHeading, int windDirection, int windSpeed);
    
    /**
     * Check if runway is acceptable for operation
     * @param runway Runway information
     * @param criteria Selection criteria
     * @param components Wind components
     * @return true if runway is acceptable
     */
    static bool IsRunwayAcceptable(
        const RunwayInfo& runway,
        const RunwaySelectionCriteria& criteria,
        const RunwayWindComponents& components);
    
    /**
     * Score runway for selection
     * Lower score is better
     * @param runway Runway information
     * @param criteria Selection criteria
     * @param components Wind components
     * @return Score (0-1000, lower is better)
     */
    static double ScoreRunway(
        const RunwayInfo& runway,
        const RunwaySelectionCriteria& criteria,
        const RunwayWindComponents& components);
    
    /**
     * Select best runway from list
     * @param runways Available runways
     * @param criteria Selection criteria
     * @param bestRunway Output best runway
     * @return true if suitable runway found
     */
    static bool SelectBestRunway(
        const std::vector<RunwayInfo>& runways,
        const RunwaySelectionCriteria& criteria,
        RunwayInfo& bestRunway);
    
    /**
     * Select runway for landing
     * Prefers headwind and ILS, minimizes crosswind
     * @param runways Available runways
     * @param windDirection Wind direction (degrees)
     * @param windSpeed Wind speed (knots)
     * @param maxCrosswind Maximum acceptable crosswind (knots)
     * @param preferILS Prefer ILS-equipped runways
     * @param selectedRunway Output selected runway
     * @return true if suitable runway found
     */
    static bool SelectForLanding(
        const std::vector<RunwayInfo>& runways,
        int windDirection,
        int windSpeed,
        double maxCrosswind,
        bool preferILS,
        RunwayInfo& selectedRunway);
    
    /**
     * Select runway for takeoff
     * Maximizes headwind, minimizes crosswind
     * @param runways Available runways
     * @param windDirection Wind direction (degrees)
     * @param windSpeed Wind speed (knots)
     * @param maxCrosswind Maximum acceptable crosswind (knots)
     * @param selectedRunway Output selected runway
     * @return true if suitable runway found
     */
    static bool SelectForTakeoff(
        const std::vector<RunwayInfo>& runways,
        int windDirection,
        int windSpeed,
        double maxCrosswind,
        RunwayInfo& selectedRunway);
    
    /**
     * Validate runway for aircraft
     * @param runway Runway information
     * @param aircraftType Aircraft type
     * @param requiredDistance Required landing/takeoff distance
     * @return true if runway is suitable for aircraft
     */
    static bool ValidateRunwayForAircraft(
        const RunwayInfo& runway,
        const std::string& aircraftType,
        double requiredDistance);
    
    /**
     * Get reciprocal runway ID (e.g., "04L" -> "22R")
     * @param runwayId Runway ID
     * @return Reciprocal runway ID
     */
    static std::string GetReciprocalRunway(const std::string& runwayId);
    
    /**
     * Parse runway heading from ID (e.g., "04L" -> 4, "22R" -> 22)
     * @param runwayId Runway ID
     * @return Heading (0-36)
     */
    static int ParseRunwayHeading(const std::string& runwayId);

private:
    // Weight factors for runway scoring
    static constexpr double CROSSWIND_WEIGHT = 100.0;        // Heavy weight to minimize crosswind
    static constexpr double HEADWIND_WEIGHT = 50.0;          // Prefer headwind
    static constexpr double TAILWIND_PENALTY = 200.0;        // Strong penalty for tailwind
    static constexpr double LENGTH_BONUS = 0.1;              // Small bonus for longer runways
    static constexpr double ILS_BONUS = -50.0;               // Bonus for ILS
    
    // Limits
    static constexpr double MIN_HEADWIND_THRESHOLD = -5.0;   // Max tailwind allowed (knots)
    static constexpr double MAX_ACCEPTABLE_CROSSWIND = 25.0; // Hard limit for crosswind
    
    static double NormalizeAngleDifference(double angle);
};

} // namespace AICopilot

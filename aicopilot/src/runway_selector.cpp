/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Runway Selector Implementation
* Advanced runway selection algorithm with wind component calculations
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#include "../include/runway_selector.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AICopilot {

RunwaySelector::RunwaySelector() = default;

RunwaySelector::~RunwaySelector() = default;

RunwayWindComponents RunwaySelector::CalculateWindComponents(
    int runwayHeading,
    int windDirection,
    int windSpeed) {
    
    RunwayWindComponents components;
    components.runwayHeading = runwayHeading;
    components.direction = windDirection;
    components.magnitude = windSpeed;
    
    // Convert degrees to radians
    double headingRad = runwayHeading * M_PI / 180.0;
    double windRad = windDirection * M_PI / 180.0;
    
    // Calculate relative angle between wind and runway
    double angleDiff = windRad - headingRad;
    
    // Normalize to [-180, 180]
    while (angleDiff > M_PI) angleDiff -= 2.0 * M_PI;
    while (angleDiff < -M_PI) angleDiff += 2.0 * M_PI;
    
    // Headwind component (positive = headwind, negative = tailwind)
    components.headwind = windSpeed * std::cos(angleDiff);
    
    // Crosswind component (positive = from left, negative = from right)
    components.crosswind = windSpeed * std::sin(angleDiff);
    
    // Tailwind for reporting
    components.tailwind = -components.headwind;
    if (components.tailwind < 0.0) components.tailwind = 0.0;
    
    return components;
}

double RunwaySelector::CalculateCrosswind(int runwayHeading, int windDirection, int windSpeed) {
    double headingRad = runwayHeading * M_PI / 180.0;
    double windRad = windDirection * M_PI / 180.0;
    double angleDiff = windRad - headingRad;
    
    while (angleDiff > M_PI) angleDiff -= 2.0 * M_PI;
    while (angleDiff < -M_PI) angleDiff += 2.0 * M_PI;
    
    return windSpeed * std::sin(angleDiff);
}

double RunwaySelector::CalculateHeadwind(int runwayHeading, int windDirection, int windSpeed) {
    double headingRad = runwayHeading * M_PI / 180.0;
    double windRad = windDirection * M_PI / 180.0;
    double angleDiff = windRad - headingRad;
    
    while (angleDiff > M_PI) angleDiff -= 2.0 * M_PI;
    while (angleDiff < -M_PI) angleDiff += 2.0 * M_PI;
    
    return windSpeed * std::cos(angleDiff);
}

bool RunwaySelector::IsRunwayAcceptable(
    const RunwayInfo& runway,
    const RunwaySelectionCriteria& criteria,
    const RunwayWindComponents& components) {
    
    // Check crosswind limit
    if (std::abs(components.crosswind) > criteria.maxAcceptableCrosswind) {
        return false;
    }
    
    // Check tailwind limit (allow some tailwind)
    if (components.headwind < -criteria.maxAcceptableTailwind) {
        return false;
    }
    
    // Check runway length
    if (runway.LDA < static_cast<int>(criteria.requiredDistance)) {
        return false;
    }
    
    // Check runway width (typical minimums)
    if (runway.width < criteria.aircraftType.empty() ? 75 : 100) {
        return false;
    }
    
    // Check runway is available
    if (runway.length == 0) {
        return false;
    }
    
    return true;
}

double RunwaySelector::ScoreRunway(
    const RunwayInfo& runway,
    const RunwaySelectionCriteria& criteria,
    const RunwayWindComponents& components) {
    
    double score = 0.0;
    
    // Penalize crosswind heavily (0-500 points based on crosswind)
    double absCrosswind = std::abs(components.crosswind);
    score += absCrosswind * CROSSWIND_WEIGHT;
    
    // Penalize tailwind heavily
    if (components.headwind < 0.0) {
        score += (-components.headwind) * TAILWIND_PENALTY;
    } else {
        // Reward headwind
        score -= components.headwind * HEADWIND_WEIGHT;
    }
    
    // Bonus for longer runways
    score -= (runway.length - criteria.requiredDistance) * LENGTH_BONUS;
    
    // Bonus for ILS
    if (criteria.preferILS && runway.ilsData.hasILS) {
        score += ILS_BONUS;
    }
    
    // Ensure score is non-negative
    if (score < 0.0) score = 0.0;
    
    return score;
}

bool RunwaySelector::SelectBestRunway(
    const std::vector<RunwayInfo>& runways,
    const RunwaySelectionCriteria& criteria,
    RunwayInfo& bestRunway) {
    
    double bestScore = 1000000.0;
    bool found = false;
    
    for (const auto& runway : runways) {
        // Calculate wind components
        auto components = CalculateWindComponents(
            runway.headingMagnetic,
            criteria.windDirection,
            criteria.windSpeed);
        
        // Check if acceptable
        if (!IsRunwayAcceptable(runway, criteria, components)) {
            continue;
        }
        
        // Calculate score
        double score = ScoreRunway(runway, criteria, components);
        
        if (score < bestScore) {
            bestScore = score;
            bestRunway = runway;
            found = true;
        }
    }
    
    return found;
}

bool RunwaySelector::SelectForLanding(
    const std::vector<RunwayInfo>& runways,
    int windDirection,
    int windSpeed,
    double maxCrosswind,
    bool preferILS,
    RunwayInfo& selectedRunway) {
    
    RunwaySelectionCriteria criteria;
    criteria.windDirection = windDirection;
    criteria.windSpeed = windSpeed;
    criteria.maxAcceptableCrosswind = maxCrosswind;
    criteria.maxAcceptableTailwind = 5.0;  // Conservative for landing
    criteria.preferILS = preferILS;
    criteria.requiredDistance = 5000.0;  // Conservative estimate
    
    return SelectBestRunway(runways, criteria, selectedRunway);
}

bool RunwaySelector::SelectForTakeoff(
    const std::vector<RunwayInfo>& runways,
    int windDirection,
    int windSpeed,
    double maxCrosswind,
    RunwayInfo& selectedRunway) {
    
    RunwaySelectionCriteria criteria;
    criteria.windDirection = windDirection;
    criteria.windSpeed = windSpeed;
    criteria.maxAcceptableCrosswind = maxCrosswind;
    criteria.maxAcceptableTailwind = 3.0;  // Very conservative for takeoff
    criteria.preferILS = false;  // ILS not needed for takeoff
    criteria.requiredDistance = 5000.0;
    
    return SelectBestRunway(runways, criteria, selectedRunway);
}

bool RunwaySelector::ValidateRunwayForAircraft(
    const RunwayInfo& runway,
    const std::string& aircraftType,
    double requiredDistance) {
    
    // Check runway length
    if (runway.LDA < requiredDistance) {
        return false;
    }
    
    // Check minimum width
    if (runway.width < 75) {  // Absolute minimum
        return false;
    }
    
    // Check runway is open
    if (runway.length == 0) {
        return false;
    }
    
    return true;
}

std::string RunwaySelector::GetReciprocalRunway(const std::string& runwayId) {
    if (runwayId.empty() || runwayId.length() < 2) {
        return "";
    }
    
    try {
        int heading = std::stoi(runwayId.substr(0, 2));
        int reciprocal = (heading + 18) % 36;
        
        std::stringstream ss;
        ss.width(2);
        ss.fill('0');
        ss << reciprocal;
        std::string recId = ss.str();
        
        // Add suffix
        if (runwayId.length() > 2) {
            char suffix = runwayId[2];
            if (suffix == 'L') recId += 'R';
            else if (suffix == 'R') recId += 'L';
            else if (suffix == 'C') recId += 'C';
        }
        
        return recId;
    } catch (...) {
        return "";
    }
}

int RunwaySelector::ParseRunwayHeading(const std::string& runwayId) {
    if (runwayId.empty() || runwayId.length() < 2) {
        return 0;
    }
    
    try {
        return std::stoi(runwayId.substr(0, 2));
    } catch (...) {
        return 0;
    }
}

double RunwaySelector::NormalizeAngleDifference(double angle) {
    while (angle > 180.0) angle -= 360.0;
    while (angle < -180.0) angle += 360.0;
    return angle;
}

} // namespace AICopilot

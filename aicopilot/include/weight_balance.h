/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Comprehensive Weight and Balance System
* Implements CG envelope validation and moment calculations
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef WEIGHT_BALANCE_H
#define WEIGHT_BALANCE_H

#include "aicopilot_types.h"
#include "aircraft_config.h"
#include <string>
#include <vector>

namespace AICopilot {

/**
 * CG Envelope limits at different gross weights
 */
struct CGEnvelopePoint {
    double grossWeight;      // lbs
    double cgForwardLimit;   // % of MAC
    double cgAftLimit;       // % of MAC
};

/**
 * CG Envelope definition
 */
struct CGEnvelope {
    std::string aircraftType;
    std::vector<CGEnvelopePoint> envelopePoints;
    double minimumWeight;    // lbs (typically empty weight)
    double maximumWeight;    // lbs (MTOW)
    double macLength;        // Mean Aerodynamic Chord length in feet
    double macLeadingEdgeLocation;  // distance from datum in feet
};

/**
 * Weight item (seat, cargo, fuel, etc.)
 */
struct WeightItem {
    std::string name;
    std::string category;    // crew, passenger, cargo, fuel, equipment
    double weight;           // lbs
    double armDistance;      // distance from datum in feet
    double moment;           // weight * armDistance
    bool adjustable;         // can be moved or redistributed
};

/**
 * Complete weight and balance calculation result
 */
struct WeightBalanceResult {
    double totalWeight;
    double totalMoment;
    double cgLocation;       // % of MAC
    double cgLocationFeet;   // distance from datum in feet
    bool withinEnvelope;
    std::string status;      // "OK", "TAIL_HEAVY", "NOSE_HEAVY", "OVERWEIGHT", etc.
    std::vector<std::string> warnings;
    std::string report;
};

/**
 * Comprehensive Weight and Balance System
 * Validates weight distribution and CG position
 */
class WeightBalanceSystem {
public:
    WeightBalanceSystem();
    ~WeightBalanceSystem() = default;
    
    /**
     * Initialize with aircraft configuration
     */
    bool initialize(const AircraftConfig& config);
    
    /**
     * Load CG envelope from aircraft data
     */
    bool loadCGEnvelope(const CGEnvelope& envelope);
    
    /**
     * Add weight item
     */
    void addWeightItem(const WeightItem& item);
    
    /**
     * Remove weight item by name
     */
    void removeWeightItem(const std::string& name);
    
    /**
     * Update weight item
     */
    void updateWeightItem(const std::string& name, double newWeight);
    
    /**
     * Get all weight items
     */
    const std::vector<WeightItem>& getWeightItems() const { return weightItems_; }
    
    /**
     * Calculate weight and balance
     */
    WeightBalanceResult calculateWeightBalance();
    
    /**
     * Check if within limits
     */
    bool isWithinLimits() const { return lastResult_.withinEnvelope; }
    
    /**
     * Get last calculation result
     */
    const WeightBalanceResult& getLastResult() const { return lastResult_; }
    
    /**
     * Clear all weight items
     */
    void clearWeights();
    
    /**
     * Get fuel required for range
     */
    double getFuelRequiredForRange(
        double distanceNauticalMiles,
        double cruiseSpeed,      // knots true airspeed
        double fuelBurnRate);    // gallons per hour
    
    /**
     * Adjust load to bring CG within envelope
     */
    bool adjustLoadToCG(
        const std::string& movableItemName,
        double targetCG);
    
    /**
     * Get center of gravity in different formats
     */
    double getCGPercentMAC() const { return lastResult_.cgLocation; }
    double getCGFeetFromDatum() const { return lastResult_.cgLocationFeet; }
    
    /**
     * Generate weight and balance report
     */
    std::string generateReport() const;
    
private:
    AircraftConfig aircraftConfig_;
    CGEnvelope cgEnvelope_;
    std::vector<WeightItem> weightItems_;
    WeightBalanceResult lastResult_;
    
    // Calculation helpers
    void calculateMoments();
    bool validateCGPosition(double totalWeight, double cgLocation);
    bool interpolateCGLimits(double weight, 
                            double& forwardLimit, 
                            double& aftLimit) const;
    
    // Aircraft database methods
    void loadStandardCGEnvelopes();
    CGEnvelope createCessna172Envelope();
    CGEnvelope createBeechcraft58Envelope();
    CGEnvelope createCessna208Envelope();
    CGEnvelope createDassaultFalcon7XEnvelope();
    CGEnvelope createB737Envelope();
    CGEnvelope createA320Envelope();
};

} // namespace AICopilot

#endif // WEIGHT_BALANCE_H

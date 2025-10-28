/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
* Comprehensive Weight and Balance Implementation
*****************************************************************************/

#include "../include/weight_balance.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace AICopilot {

WeightBalanceSystem::WeightBalanceSystem() = default;

bool WeightBalanceSystem::initialize(const AircraftConfig& config) {
    aircraftConfig_ = config;
    clearWeights();
    loadStandardCGEnvelopes();
    
    // Load envelope for this aircraft
    if (config.icaoModel.find("C172") != std::string::npos) {
        cgEnvelope_ = createCessna172Envelope();
    } else if (config.icaoModel.find("B58") != std::string::npos) {
        cgEnvelope_ = createBeechcraft58Envelope();
    } else if (config.icaoModel.find("C208") != std::string::npos) {
        cgEnvelope_ = createCessna208Envelope();
    } else if (config.icaoModel.find("FA7X") != std::string::npos || 
               config.icaoModel.find("7X") != std::string::npos) {
        cgEnvelope_ = createDassaultFalcon7XEnvelope();
    } else if (config.icaoModel.find("B737") != std::string::npos) {
        cgEnvelope_ = createB737Envelope();
    } else if (config.icaoModel.find("A320") != std::string::npos) {
        cgEnvelope_ = createA320Envelope();
    }
    
    return true;
}

bool WeightBalanceSystem::loadCGEnvelope(const CGEnvelope& envelope) {
    cgEnvelope_ = envelope;
    return true;
}

void WeightBalanceSystem::addWeightItem(const WeightItem& item) {
    weightItems_.push_back(item);
}

void WeightBalanceSystem::removeWeightItem(const std::string& name) {
    auto it = std::find_if(weightItems_.begin(), weightItems_.end(),
        [&name](const WeightItem& item) { return item.name == name; });
    
    if (it != weightItems_.end()) {
        weightItems_.erase(it);
    }
}

void WeightBalanceSystem::updateWeightItem(const std::string& name, double newWeight) {
    for (auto& item : weightItems_) {
        if (item.name == name) {
            item.weight = newWeight;
            item.moment = item.weight * item.armDistance;
            break;
        }
    }
}

WeightBalanceResult WeightBalanceSystem::calculateWeightBalance() {
    WeightBalanceResult result;
    result.totalWeight = 0.0;
    result.totalMoment = 0.0;
    
    // Calculate total weight and moment
    for (const auto& item : weightItems_) {
        result.totalWeight += item.weight;
        result.totalMoment += item.moment;
    }
    
    // Check weight limits
    if (result.totalWeight > cgEnvelope_.maximumWeight) {
        result.warnings.push_back("WEIGHT LIMIT EXCEEDED: Aircraft overweight");
        result.status = "OVERWEIGHT";
        result.withinEnvelope = false;
    }
    
    if (result.totalWeight < cgEnvelope_.minimumWeight) {
        result.warnings.push_back("WARNING: Aircraft weight below empty weight");
        result.status = "UNDERWEIGHT";
        result.withinEnvelope = false;
    }
    
    // Calculate CG location
    if (result.totalWeight > 0) {
        result.cgLocationFeet = result.totalMoment / result.totalWeight;
        
        // Convert to percentage of MAC
        double macStart = cgEnvelope_.macLeadingEdgeLocation;
        double cgOffset = result.cgLocationFeet - macStart;
        result.cgLocation = (cgOffset / cgEnvelope_.macLength) * 100.0;
    } else {
        result.cgLocationFeet = 0.0;
        result.cgLocation = 0.0;
    }
    
    // Check CG envelope
    double forwardLimit = 0.0, aftLimit = 0.0;
    if (interpolateCGLimits(result.totalWeight, forwardLimit, aftLimit)) {
        if (result.cgLocation < forwardLimit) {
            result.warnings.push_back("CG FORWARD OF LIMIT: Aircraft nose-heavy");
            result.status = "NOSE_HEAVY";
            result.withinEnvelope = false;
        } else if (result.cgLocation > aftLimit) {
            result.warnings.push_back("CG AFT OF LIMIT: Aircraft tail-heavy");
            result.status = "TAIL_HEAVY";
            result.withinEnvelope = false;
        } else {
            if (result.status.empty()) {
                result.status = "OK";
                result.withinEnvelope = true;
            }
        }
    }
    
    // Generate detailed report
    std::ostringstream reportStream;
    reportStream << "WEIGHT AND BALANCE REPORT\n";
    reportStream << "==========================\n\n";
    reportStream << "Aircraft: " << aircraftConfig_.title << "\n";
    reportStream << "Status: " << result.status << "\n\n";
    
    reportStream << "WEIGHT SUMMARY\n";
    reportStream << "==============\n";
    reportStream << "Total Weight: " << result.totalWeight << " lbs\n";
    reportStream << "Maximum Gross Weight: " << cgEnvelope_.maximumWeight << " lbs\n";
    reportStream << "Weight Margin: " << (cgEnvelope_.maximumWeight - result.totalWeight) << " lbs\n\n";
    
    reportStream << "CG POSITION\n";
    reportStream << "===========\n";
    reportStream << "CG Location: " << result.cgLocation << "% of MAC\n";
    reportStream << "CG From Datum: " << result.cgLocationFeet << " ft\n";
    reportStream << "Forward Limit: " << forwardLimit << "% of MAC\n";
    reportStream << "Aft Limit: " << aftLimit << "% of MAC\n\n";
    
    reportStream << "WEIGHT ITEMS\n";
    reportStream << "============\n";
    for (const auto& item : weightItems_) {
        reportStream << item.name << ": " << item.weight << " lbs (Arm: " 
                     << item.armDistance << " ft, Moment: " << item.moment << ")\n";
    }
    
    if (!result.warnings.empty()) {
        reportStream << "\nWARNINGS\n";
        reportStream << "========\n";
        for (const auto& warning : result.warnings) {
            reportStream << "- " << warning << "\n";
        }
    }
    
    result.report = reportStream.str();
    lastResult_ = result;
    
    return result;
}

bool WeightBalanceSystem::isWithinLimits() const {
    return lastResult_.withinEnvelope;
}

void WeightBalanceSystem::clearWeights() {
    weightItems_.clear();
}

double WeightBalanceSystem::getFuelRequiredForRange(
    double distanceNauticalMiles,
    double cruiseSpeed,
    double fuelBurnRate) {
    
    // Calculate flight time
    double flightTime = distanceNauticalMiles / cruiseSpeed;
    
    // Add 45 minutes reserve (FAA requirement)
    double totalTime = flightTime + 0.75;
    
    // Calculate fuel required
    double fuelRequired = totalTime * fuelBurnRate;
    
    return fuelRequired;
}

bool WeightBalanceSystem::adjustLoadToCG(
    const std::string& movableItemName,
    double targetCG) {
    
    // Find movable item
    auto it = std::find_if(weightItems_.begin(), weightItems_.end(),
        [&movableItemName](const WeightItem& item) {
            return item.name == movableItemName && item.adjustable;
        });
    
    if (it == weightItems_.end()) return false;
    
    // Calculate required moment shift
    WeightBalanceResult current = calculateWeightBalance();
    double momentNeeded = targetCG * current.totalWeight - current.totalMoment;
    
    // Adjust item weight to achieve target CG
    // New moment = old moment + moment shift
    double newMoment = it->moment + momentNeeded;
    double newWeight = newMoment / it->armDistance;
    
    // Check if adjustment is feasible
    if (newWeight < 0 || newWeight > 1000.0) return false;  // Sanity check
    
    it->weight = newWeight;
    it->moment = newMoment;
    
    return true;
}

std::string WeightBalanceSystem::generateReport() const {
    return lastResult_.report;
}

void WeightBalanceSystem::calculateMoments() {
    for (auto& item : weightItems_) {
        item.moment = item.weight * item.armDistance;
    }
}

bool WeightBalanceSystem::validateCGPosition(double totalWeight, double cgLocation) {
    double forwardLimit = 0.0, aftLimit = 0.0;
    
    if (!interpolateCGLimits(totalWeight, forwardLimit, aftLimit)) {
        return false;
    }
    
    return cgLocation >= forwardLimit && cgLocation <= aftLimit;
}

bool WeightBalanceSystem::interpolateCGLimits(double weight, 
                                             double& forwardLimit, 
                                             double& aftLimit) const {
    
    if (cgEnvelope_.envelopePoints.empty()) return false;
    
    // Find envelope points to interpolate between
    auto it = std::lower_bound(cgEnvelope_.envelopePoints.begin(),
                              cgEnvelope_.envelopePoints.end(),
                              weight,
                              [](const CGEnvelopePoint& p, double w) {
                                  return p.grossWeight < w;
                              });
    
    if (it == cgEnvelope_.envelopePoints.end()) {
        // Weight exceeds maximum in envelope
        return false;
    }
    
    if (it == cgEnvelope_.envelopePoints.begin()) {
        // Use first point
        forwardLimit = it->cgForwardLimit;
        aftLimit = it->cgAftLimit;
        return true;
    }
    
    // Interpolate between two points
    auto prev = it - 1;
    double w1 = prev->grossWeight;
    double w2 = it->grossWeight;
    
    if (w1 == w2) {
        forwardLimit = it->cgForwardLimit;
        aftLimit = it->cgAftLimit;
        return true;
    }
    
    double fraction = (weight - w1) / (w2 - w1);
    forwardLimit = prev->cgForwardLimit + fraction * (it->cgForwardLimit - prev->cgForwardLimit);
    aftLimit = prev->cgAftLimit + fraction * (it->cgAftLimit - prev->cgAftLimit);
    
    return true;
}

void WeightBalanceSystem::loadStandardCGEnvelopes() {
    // This would load from a database or configuration file
    // For now, envelopes are created on demand
}

CGEnvelope WeightBalanceSystem::createCessna172Envelope() {
    CGEnvelope envelope;
    envelope.aircraftType = "Cessna 172";
    envelope.minimumWeight = 1686.0;  // Empty weight
    envelope.maximumWeight = 2450.0;  // MTOW
    envelope.macLength = 59.7;        // Mean Aerodynamic Chord
    envelope.macLeadingEdgeLocation = 141.0;  // Distance from datum
    
    // CG Envelope points for Cessna 172
    envelope.envelopePoints.push_back({1686.0, 35.8, 40.5});  // At empty weight
    envelope.envelopePoints.push_back({2000.0, 35.5, 40.7});
    envelope.envelopePoints.push_back({2200.0, 35.2, 41.0});
    envelope.envelopePoints.push_back({2450.0, 34.8, 41.5});  // At MTOW
    
    return envelope;
}

CGEnvelope WeightBalanceSystem::createBeechcraft58Envelope() {
    CGEnvelope envelope;
    envelope.aircraftType = "Beechcraft 58";
    envelope.minimumWeight = 3393.0;  // Empty weight
    envelope.maximumWeight = 5500.0;  // MTOW
    envelope.macLength = 54.0;
    envelope.macLeadingEdgeLocation = 328.0;
    
    envelope.envelopePoints.push_back({3393.0, 20.0, 28.0});
    envelope.envelopePoints.push_back({4000.0, 19.5, 28.5});
    envelope.envelopePoints.push_back({4500.0, 19.0, 29.0});
    envelope.envelopePoints.push_back({5500.0, 18.5, 30.0});
    
    return envelope;
}

CGEnvelope WeightBalanceSystem::createCessna208Envelope() {
    CGEnvelope envelope;
    envelope.aircraftType = "Cessna 208 (Caravan)";
    envelope.minimumWeight = 3105.0;  // Empty weight
    envelope.maximumWeight = 5400.0;  // MTOW
    envelope.macLength = 60.0;
    envelope.macLeadingEdgeLocation = 265.0;
    
    envelope.envelopePoints.push_back({3105.0, 23.0, 34.0});
    envelope.envelopePoints.push_back({4000.0, 22.5, 34.5});
    envelope.envelopePoints.push_back({5000.0, 22.0, 35.0});
    envelope.envelopePoints.push_back({5400.0, 21.5, 35.5});
    
    return envelope;
}

CGEnvelope WeightBalanceSystem::createDassaultFalcon7XEnvelope() {
    CGEnvelope envelope;
    envelope.aircraftType = "Dassault Falcon 7X";
    envelope.minimumWeight = 36000.0;  // Empty weight (lbs)
    envelope.maximumWeight = 61000.0;  // MTOW (lbs)
    envelope.macLength = 37.7;
    envelope.macLeadingEdgeLocation = 650.0;
    
    envelope.envelopePoints.push_back({36000.0, 30.0, 38.0});
    envelope.envelopePoints.push_back({45000.0, 29.5, 38.5});
    envelope.envelopePoints.push_back({53000.0, 29.0, 39.0});
    envelope.envelopePoints.push_back({61000.0, 28.5, 39.5});
    
    return envelope;
}

CGEnvelope WeightBalanceSystem::createB737Envelope() {
    CGEnvelope envelope;
    envelope.aircraftType = "Boeing 737";
    envelope.minimumWeight = 154500.0;  // Typical OEW in lbs
    envelope.maximumWeight = 405000.0;  // MTOW in lbs
    envelope.macLength = 81.0;
    envelope.macLeadingEdgeLocation = 1100.0;
    
    // Simplified envelope
    envelope.envelopePoints.push_back({154500.0, 15.0, 35.0});
    envelope.envelopePoints.push_back({250000.0, 15.5, 35.5});
    envelope.envelopePoints.push_back({350000.0, 16.0, 36.0});
    envelope.envelopePoints.push_back({405000.0, 16.5, 36.5});
    
    return envelope;
}

CGEnvelope WeightBalanceSystem::createA320Envelope() {
    CGEnvelope envelope;
    envelope.aircraftType = "Airbus A320";
    envelope.minimumWeight = 142000.0;  // Typical OEW in lbs
    envelope.maximumWeight = 405000.0;  // MTOW in lbs
    envelope.macLength = 78.0;
    envelope.macLeadingEdgeLocation = 1050.0;
    
    envelope.envelopePoints.push_back({142000.0, 16.0, 34.0});
    envelope.envelopePoints.push_back({250000.0, 16.5, 34.5});
    envelope.envelopePoints.push_back({350000.0, 17.0, 35.0});
    envelope.envelopePoints.push_back({405000.0, 17.5, 35.5});
    
    return envelope;
}

} // namespace AICopilot

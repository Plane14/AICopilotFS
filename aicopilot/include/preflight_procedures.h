/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Comprehensive Preflight Procedures System
* Implements realistic, itemized preflight checklists with state machine
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*****************************************************************************/

#ifndef PREFLIGHT_PROCEDURES_H
#define PREFLIGHT_PROCEDURES_H

#include "aicopilot_types.h"
#include "aircraft_config.h"
#include <string>
#include <vector>
#include <memory>

namespace AICopilot {

/**
 * Preflight Checklist Item Status
 */
enum class ChecklistItemStatus {
    NOT_STARTED,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    NOT_APPLICABLE
};

/**
 * Preflight Checklist Phases
 */
enum class PreflightPhase {
    NOT_STARTED,
    EXTERIOR_INSPECTION,
    INTERIOR_INSPECTION,
    ENGINE_STARTUP,
    SYSTEM_CHECKS,
    TAXI_READINESS,
    COMPLETE,
    FAILED
};

/**
 * Individual preflight checklist item
 */
struct ChecklistItem {
    std::string id;
    std::string description;
    std::string action;
    std::string expected;
    ChecklistItemStatus status = ChecklistItemStatus::NOT_STARTED;
    double completionTime = 0.0;  // seconds
    std::string notes;
    bool critical = false;  // If fails, abort preflight
};

/**
 * Preflight procedure result
 */
struct PreflightResult {
    bool passed = false;
    std::vector<std::string> failedItems;
    std::vector<std::string> warnings;
    double totalTime = 0.0;  // seconds
    std::string report;
};

/**
 * Realistic Preflight Procedures System
 * Implements FAA and manufacturer standard preflight checklists
 */
class PreflightProcedures {
public:
    PreflightProcedures();
    ~PreflightProcedures() = default;
    
    /**
     * Initialize preflight for specific aircraft
     */
    bool initialize(const AircraftConfig& config);
    
    /**
     * Start preflight procedure
     */
    void startPreflight();
    
    /**
     * Execute next checklist item
     * @return true if all items processed, false if still in progress
     */
    bool executeNextItem(const AircraftState& currentState);
    
    /**
     * Get current preflight phase
     */
    PreflightPhase getCurrentPhase() const { return currentPhase_; }
    
    /**
     * Get current checklist item
     */
    const ChecklistItem& getCurrentItem() const;
    
    /**
     * Get all checklist items
     */
    const std::vector<ChecklistItem>& getAllItems() const { return checklistItems_; }
    
    /**
     * Get progress (0.0 to 1.0)
     */
    double getProgress() const;
    
    /**
     * Complete preflight and get results
     */
    PreflightResult completePreflight(const AircraftState& finalState);
    
    /**
     * Get status report
     */
    std::string getStatusReport() const;
    
    /**
     * Skip to next phase (for testing)
     */
    void advancePhase();
    
    /**
     * Manually mark item as completed
     */
    void completeItem(const std::string& itemId, bool passed = true, 
                     const std::string& notes = "");
    
    /**
     * Check if preflight is complete
     */
    bool isComplete() const { return currentPhase_ == PreflightPhase::COMPLETE; }
    
    /**
     * Check if preflight failed
     */
    bool hasFailed() const { return currentPhase_ == PreflightPhase::FAILED; }
    
private:
    AircraftConfig aircraftConfig_;
    PreflightPhase currentPhase_ = PreflightPhase::NOT_STARTED;
    std::vector<ChecklistItem> checklistItems_;
    size_t currentItemIndex_ = 0;
    double startTime_ = 0.0;
    
    // Initialize checklists for different aircraft types
    void initializeGenericChecklist();
    void initializeSingleEngineChecklist();
    void initializeMultiEngineChecklist();
    void initializeTurbopropChecklist();
    void initializeJetChecklist();
    
    // Checklist sections
    void addExteriorInspectionItems();
    void addInteriorInspectionItems();
    void addEngineStartupItems();
    void addSystemCheckItems();
    void addTaxiReadinessItems();
    
    // Validation methods
    bool validateExteriorCondition(const AircraftState& state);
    bool validateInteriorSystems(const AircraftState& state);
    bool validateEngineReadiness(const AircraftState& state);
    bool validateAllSystems(const AircraftState& state);
    bool validateTaxiReadiness(const AircraftState& state);
    
    // State machine transitions
    void transitionToNextPhase();
    void handlePhaseCompletion();
    
    // Phase execution
    bool executeExteriorInspection(const AircraftState& state);
    bool executeInteriorInspection(const AircraftState& state);
    bool executeEngineStartup(const AircraftState& state);
    bool executeSystemChecks(const AircraftState& state);
    bool executeTaxiReadiness(const AircraftState& state);
};

} // namespace AICopilot

#endif // PREFLIGHT_PROCEDURES_H

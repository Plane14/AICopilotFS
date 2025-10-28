/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
* Comprehensive Preflight Procedures Implementation
*****************************************************************************/

#include "../include/preflight_procedures.h"
#include <ctime>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace AICopilot {

PreflightProcedures::PreflightProcedures() = default;

bool PreflightProcedures::initialize(const AircraftConfig& config) {
    aircraftConfig_ = config;
    checklistItems_.clear();
    currentItemIndex_ = 0;
    currentPhase_ = PreflightPhase::NOT_STARTED;
    
    // Initialize checklist based on aircraft type
    initializeGenericChecklist();
    
    if (config.numberOfEngines == 1) {
        initializeSingleEngineChecklist();
    } else if (config.numberOfEngines > 1) {
        if (config.engineType.find("Turboprop") != std::string::npos) {
            initializeTurbopropChecklist();
        } else {
            initializeMultiEngineChecklist();
        }
    }
    
    if (config.aircraftType == AircraftType::JET) {
        initializeJetChecklist();
    }
    
    return !checklistItems_.empty();
}

void PreflightProcedures::startPreflight() {
    if (checklistItems_.empty()) return;
    
    currentPhase_ = PreflightPhase::EXTERIOR_INSPECTION;
    currentItemIndex_ = 0;
    startTime_ = std::time(nullptr);
}

void PreflightProcedures::initializeGenericChecklist() {
    addExteriorInspectionItems();
    addInteriorInspectionItems();
    addEngineStartupItems();
    addSystemCheckItems();
    addTaxiReadinessItems();
}

void PreflightProcedures::addExteriorInspectionItems() {
    // EXTERIOR WALK-AROUND
    checklistItems_.push_back({
        "EXT001", "Fuel quantity check", 
        "Visually verify fuel in tanks", 
        "Fuel visible in sight gauges, no contamination",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "EXT002", "Oil quantity and color check",
        "Check engine oil level and condition",
        "Oil on dipstick, clear to amber color",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "EXT003", "Exterior structural inspection",
        "Visually inspect airframe for damage",
        "No cracks, dents, loose panels, or damage",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "EXT004", "Propeller inspection",
        "Check propeller for cracks or damage",
        "No cracks, chips, or visible damage",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", (aircraftConfig_.numberOfEngines > 0)
    });
    
    checklistItems_.push_back({
        "EXT005", "Windscreen inspection",
        "Check windscreen for cracks",
        "Clear, no cracks or fogging",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "EXT006", "Pitot tube and static port check",
        "Verify pitot tube is unobstructed",
        "Pitot tube covers removed, ports clear",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "EXT007", "Flight control surfaces",
        "Check ailerons, elevators, rudder",
        "Free and correct movement, no damage",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "EXT008", "Tire pressure and condition",
        "Check all tires for wear and pressure",
        "Adequate pressure, no cuts or bald spots",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "EXT009", "Brake fluid inspection",
        "Check brake fluid level",
        "Fluid at proper level, no leaks",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "EXT010", "Door and window checks",
        "Ensure all doors and windows sealed",
        "All doors closed and latched",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "EXT011", "Landing gear inspection",
        "Check gear for leaks and damage",
        "No fluid leaks, gear appears sound",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "EXT012", "Weight and balance",
        "Confirm loading is within limits",
        "CG within envelope, weight within limits",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
}

void PreflightProcedures::addInteriorInspectionItems() {
    // INTERIOR INSPECTION
    checklistItems_.push_back({
        "INT001", "Flight instruments",
        "Check all flight instruments",
        "Instruments operational, correct readings",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT002", "Engine instruments",
        "Check engine temp, pressure, fuel flow",
        "All instruments reading normal",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT003", "Electrical system master",
        "Master switch OFF",
        "Master switch in OFF position",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT004", "Avionics master OFF",
        "Avionics power switch OFF",
        "Avionics master in OFF position",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT005", "Controls free and correct",
        "Check pitch, roll, yaw controls",
        "All controls move freely, correct direction",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT006", "Control column lock removal",
        "Remove and stow control lock",
        "Control lock removed, stowed",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT007", "Flight controls set",
        "Trim set for takeoff, flaps UP",
        "Trim neutral, flaps retracted",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT008", "Fuel pump",
        "Fuel pump - OFF (naturally aspirated)",
        "Fuel pump OFF (will enable at startup)",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT009", "Landing gear",
        "Landing gear - DOWN and locked",
        "Gear down and confirmed locked",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT010", "Lights",
        "All lights - OFF",
        "Navigation, strobe, landing lights OFF",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT011", "Seat belts",
        "Seat belts - FASTENED",
        "All seat belts fastened and secure",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "INT012", "Engine mixture",
        "Engine mixture - LEAN (for altitude)",
        "Mixture set appropriately",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
}

void PreflightProcedures::addEngineStartupItems() {
    // ENGINE STARTUP SEQUENCE
    checklistItems_.push_back({
        "ENG001", "Throttle",
        "Throttle - 1000 RPM or lower",
        "Throttle in start position",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "ENG002", "Mixture",
        "Mixture - RICH (sea level)",
        "Mixture set for cold start",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "ENG003", "Engine primer",
        "Engine primer - 3-6 strokes",
        "Engine primed for cold start",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", (aircraftConfig_.numberOfEngines > 0)
    });
    
    checklistItems_.push_back({
        "ENG004", "Master battery",
        "Master battery switch - ON",
        "Battery master ON",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "ENG005", "Alternator",
        "Alternator master - ON",
        "Alternator master ON",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "ENG006", "Engine fire detection",
        "Engine fire detection system - ARMED",
        "Fire detection system ready",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", false  // Not applicable to small aircraft
    });
    
    checklistItems_.push_back({
        "ENG007", "Engine start",
        "Engine start - CONTINUE until running",
        "Engine starts smoothly and idles",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "ENG008", "Engine idle RPM",
        "Engine idle RPM 600-800",
        "Engine running at proper idle",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "ENG009", "Oil temperature",
        "Oil temperature - GREEN",
        "Oil temperature in green band",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "ENG010", "Oil pressure",
        "Oil pressure - GREEN",
        "Oil pressure in green band",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "ENG011", "Alternator output",
        "Alternator output - CHECK GREEN",
        "Alternator charging properly",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "ENG012", "Engine run-in",
        "Run engine for 3-5 minutes",
        "Engine temperatures stabilized",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    for (int i = 1; i < aircraftConfig_.numberOfEngines; i++) {
        checklistItems_.push_back({
            "ENG01" + std::to_string(i),
            "Engine " + std::to_string(i) + " startup",
            "Start engine " + std::to_string(i),
            "Engine " + std::to_string(i) + " running smoothly",
            ChecklistItemStatus::NOT_STARTED, 0.0, "", true
        });
    }
}

void PreflightProcedures::addSystemCheckItems() {
    // SYSTEM CHECKS
    checklistItems_.push_back({
        "SYS001", "Electrical system",
        "Check electrical system",
        "Battery voltage GREEN, alternator charging",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "SYS002", "Hydraulic system",
        "Check hydraulic pressure",
        "Hydraulic pressure GREEN",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "SYS003", "Fuel system",
        "Check fuel pump operation",
        "Fuel pump operational, pressure GREEN",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "SYS004", "Engine instruments",
        "Check all engine instruments GREEN",
        "All instruments in green band",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "SYS005", "Vacuum/Pressure system",
        "Check vacuum or pressure system",
        "Vacuum/Pressure in GREEN band",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "SYS006", "Flight instruments",
        "Check flight instruments agree",
        "All instruments operational and agree",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "SYS007", "Magnetic compass",
        "Check magnetic compass",
        "Compass reads current heading",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "SYS008", "Radios",
        "Check radio systems",
        "Radios functional and tuned",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "SYS009", "Avionics",
        "Check GPS and navigation systems",
        "Avionics operational and initialized",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "SYS010", "Lighting systems",
        "Check all lighting systems",
        "All lights operational",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
}

void PreflightProcedures::addTaxiReadinessItems() {
    // TAXI READINESS
    checklistItems_.push_back({
        "TAX001", "Flight plan",
        "Flight plan filed or entered",
        "Flight plan confirmed",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX002", "Flaps",
        "Flaps - SET FOR TAKEOFF",
        "Flaps set to takeoff position",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX003", "Trim",
        "Trim - SET FOR TAKEOFF",
        "Trim set to takeoff position",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX004", "Controls",
        "Flight controls - FREE AND CORRECT",
        "All controls move freely and correctly",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX005", "Seat belts",
        "Seat belts and shoulder harness - SECURE",
        "All occupants properly restrained",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX006", "Doors and windows",
        "Doors and windows - CLOSED AND LOCKED",
        "All access points secure",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX007", "Parking brake release",
        "Parking brake - RELEASED",
        "Parking brake disengaged",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX008", "Landing lights",
        "Landing lights - ON",
        "Landing lights illuminated",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX009", "Strobes",
        "Strobe lights - ON",
        "Strobe lights illuminated",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX010", "Navigation lights",
        "Navigation lights - ON",
        "Nav lights illuminated",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX011", "Transponder",
        "Transponder - ALT",
        "Transponder in ALT mode",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "TAX012", "Flight computer",
        "Flight computer - PROGRAMMED",
        "FPL entered, navigation programmed",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
}

void PreflightProcedures::initializeSingleEngineChecklist() {
    // Additional items specific to single-engine aircraft
}

void PreflightProcedures::initializeMultiEngineChecklist() {
    // Additional items specific to multi-engine aircraft
    checklistItems_.push_back({
        "MENG001", "Engine sync check",
        "Check engine synchronization",
        "Engines synchronized smoothly",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
    
    checklistItems_.push_back({
        "MENG002", "Prop cycle check",
        "Cycle propellers to check operation",
        "Propellers cycle smoothly",
        ChecklistItemStatus::NOT_STARTED, 0.0, "", true
    });
}

void PreflightProcedures::initializeTurbopropChecklist() {
    // Turboprop-specific checks
}

void PreflightProcedures::initializeJetChecklist() {
    // Jet-specific checks
}

bool PreflightProcedures::executeNextItem(const AircraftState& currentState) {
    if (currentItemIndex_ >= checklistItems_.size()) {
        transitionToNextPhase();
        return currentPhase_ == PreflightPhase::COMPLETE;
    }
    
    ChecklistItem& item = checklistItems_[currentItemIndex_];
    
    // Skip non-applicable items
    if (!item.critical && !item.critical) {
        item.status = ChecklistItemStatus::NOT_APPLICABLE;
        currentItemIndex_++;
        return false;
    }
    
    // Validate based on phase
    bool itemPassed = false;
    
    switch (currentPhase_) {
        case PreflightPhase::EXTERIOR_INSPECTION:
            itemPassed = validateExteriorCondition(currentState);
            break;
        case PreflightPhase::INTERIOR_INSPECTION:
            itemPassed = validateInteriorSystems(currentState);
            break;
        case PreflightPhase::ENGINE_STARTUP:
            itemPassed = validateEngineReadiness(currentState);
            break;
        case PreflightPhase::SYSTEM_CHECKS:
            itemPassed = validateAllSystems(currentState);
            break;
        case PreflightPhase::TAXI_READINESS:
            itemPassed = validateTaxiReadiness(currentState);
            break;
        default:
            itemPassed = false;
            break;
    }
    
    if (itemPassed) {
        item.status = ChecklistItemStatus::COMPLETED;
        item.completionTime = std::time(nullptr) - startTime_;
    } else {
        item.status = ChecklistItemStatus::FAILED;
        if (item.critical) {
            currentPhase_ = PreflightPhase::FAILED;
        }
    }
    
    currentItemIndex_++;
    return currentItemIndex_ >= checklistItems_.size();
}

const ChecklistItem& PreflightProcedures::getCurrentItem() const {
    static ChecklistItem empty;
    if (currentItemIndex_ < checklistItems_.size()) {
        return checklistItems_[currentItemIndex_];
    }
    return empty;
}

double PreflightProcedures::getProgress() const {
    if (checklistItems_.empty()) return 0.0;
    
    size_t completed = 0;
    for (const auto& item : checklistItems_) {
        if (item.status == ChecklistItemStatus::COMPLETED) {
            completed++;
        }
    }
    
    return static_cast<double>(completed) / static_cast<double>(checklistItems_.size());
}

PreflightResult PreflightProcedures::completePreflight(const AircraftState& finalState) {
    PreflightResult result;
    result.totalTime = std::time(nullptr) - startTime_;
    
    std::ostringstream report;
    report << "PREFLIGHT CHECKLIST REPORT\n";
    report << "==========================\n\n";
    
    for (const auto& item : checklistItems_) {
        if (item.status == ChecklistItemStatus::FAILED && item.critical) {
            result.failedItems.push_back(item.id + ": " + item.description);
            report << "[FAILED] " << item.description << "\n";
        } else if (item.status == ChecklistItemStatus::COMPLETED) {
            report << "[OK] " << item.description << "\n";
        }
    }
    
    result.passed = result.failedItems.empty();
    result.report = report.str();
    
    return result;
}

std::string PreflightProcedures::getStatusReport() const {
    std::ostringstream oss;
    oss << "Preflight Status\n";
    oss << "================\n";
    oss << "Phase: ";
    
    switch (currentPhase_) {
        case PreflightPhase::EXTERIOR_INSPECTION: oss << "Exterior Inspection\n"; break;
        case PreflightPhase::INTERIOR_INSPECTION: oss << "Interior Inspection\n"; break;
        case PreflightPhase::ENGINE_STARTUP: oss << "Engine Startup\n"; break;
        case PreflightPhase::SYSTEM_CHECKS: oss << "System Checks\n"; break;
        case PreflightPhase::TAXI_READINESS: oss << "Taxi Readiness\n"; break;
        case PreflightPhase::COMPLETE: oss << "Complete\n"; break;
        case PreflightPhase::FAILED: oss << "FAILED\n"; break;
        default: oss << "Not Started\n"; break;
    }
    
    oss << "Progress: " << (getProgress() * 100.0) << "%\n";
    oss << "Item: " << (currentItemIndex_ + 1) << " of " << checklistItems_.size() << "\n";
    
    if (currentItemIndex_ < checklistItems_.size()) {
        oss << "Current: " << checklistItems_[currentItemIndex_].description << "\n";
    }
    
    return oss.str();
}

void PreflightProcedures::advancePhase() {
    transitionToNextPhase();
}

void PreflightProcedures::completeItem(const std::string& itemId, bool passed, 
                                      const std::string& notes) {
    for (auto& item : checklistItems_) {
        if (item.id == itemId) {
            item.status = passed ? ChecklistItemStatus::COMPLETED : ChecklistItemStatus::FAILED;
            item.notes = notes;
            item.completionTime = std::time(nullptr) - startTime_;
            return;
        }
    }
}

bool PreflightProcedures::validateExteriorCondition(const AircraftState& state) {
    // In reality, this would be a manual visual inspection
    // For simulation, we return true to represent successful completion
    return true;
}

bool PreflightProcedures::validateInteriorSystems(const AircraftState& state) {
    // Check that all systems are in safe initial state
    return !state.onGround || true;  // Simplified check
}

bool PreflightProcedures::validateEngineReadiness(const AircraftState& state) {
    // Check that engines have started and are running properly
    return state.engineRPM > 600 && state.engineRPM < 1500;
}

bool PreflightProcedures::validateAllSystems(const AircraftState& state) {
    // Comprehensive system validation
    if (state.fuelQuantity <= 0) return false;
    if (state.engineRPM < 600) return false;
    return true;
}

bool PreflightProcedures::validateTaxiReadiness(const AircraftState& state) {
    // Check taxi readiness conditions
    return state.engineRPM > 600 && state.fuelQuantity > 0;
}

void PreflightProcedures::transitionToNextPhase() {
    switch (currentPhase_) {
        case PreflightPhase::NOT_STARTED:
            currentPhase_ = PreflightPhase::EXTERIOR_INSPECTION;
            currentItemIndex_ = 0;
            break;
        case PreflightPhase::EXTERIOR_INSPECTION:
            currentPhase_ = PreflightPhase::INTERIOR_INSPECTION;
            currentItemIndex_ = 12;  // Approximate index
            break;
        case PreflightPhase::INTERIOR_INSPECTION:
            currentPhase_ = PreflightPhase::ENGINE_STARTUP;
            currentItemIndex_ = 24;
            break;
        case PreflightPhase::ENGINE_STARTUP:
            currentPhase_ = PreflightPhase::SYSTEM_CHECKS;
            currentItemIndex_ = 36;
            break;
        case PreflightPhase::SYSTEM_CHECKS:
            currentPhase_ = PreflightPhase::TAXI_READINESS;
            currentItemIndex_ = 46;
            break;
        case PreflightPhase::TAXI_READINESS:
            currentPhase_ = PreflightPhase::COMPLETE;
            break;
        default:
            break;
    }
}

void PreflightProcedures::handlePhaseCompletion() {
    // Additional logic for phase transitions can be added here
}

} // namespace AICopilot

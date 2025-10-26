/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Advanced example demonstrating full AI Copilot features including
* emergency handling, ATC interaction, and safety monitoring
*****************************************************************************/

#include "../include/ai_pilot.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

using namespace AICopilot;

// Global pilot instance for signal handler
AIPilot* g_pilot = nullptr;

void signalHandler(int signal) {
    if (signal == SIGINT && g_pilot) {
        std::cout << "\n\nShutdown signal received. Stopping AI Copilot..." << std::endl;
        g_pilot->stopAutonomousFlight();
    }
}

void printBanner() {
    std::cout << "╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   AI Copilot FS - Advanced Autonomous Flight System           ║" << std::endl;
    std::cout << "║   Microsoft Flight Simulator 2024 & Prepar3D V6               ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
}

void printHelp() {
    std::cout << "\nUsage: advanced_example [options] [aircraft.cfg] [flightplan.pln]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -s, --simulator TYPE Simulator type (msfs2024, p3dv6)\n";
    std::cout << "  -v, --verbose        Enable verbose logging\n";
    std::cout << "  --ollama             Enable Ollama AI for ATC menu selection\n";
    std::cout << "  --ollama-host HOST   Ollama server host (default: http://localhost:11434)\n";
    std::cout << "  --ollama-model MODEL Ollama model to use (default: llama2)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  advanced_example aircraft.cfg flightplan.pln\n";
    std::cout << "  advanced_example -s msfs2024 cessna172.cfg KSEA_KPDX.pln\n";
    std::cout << "  advanced_example --ollama --ollama-model llama3 aircraft.cfg plan.pln\n";
    std::cout << std::endl;
}

void demonstrateFeatures(AIPilot& pilot) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "AI COPILOT FEATURE DEMONSTRATION" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "\n✓ Autonomous Flight Management" << std::endl;
    std::cout << "  - 10 automated flight phases" << std::endl;
    std::cout << "  - Automatic phase transitions" << std::endl;
    std::cout << "  - Continuous safety monitoring" << std::endl;
    
    std::cout << "\n✓ Advanced Navigation" << std::endl;
    std::cout << "  - Flight plan parsing (PLN, FMS formats)" << std::endl;
    std::cout << "  - Great circle calculations" << std::endl;
    std::cout << "  - Cross-track error monitoring" << std::endl;
    std::cout << "  - Waypoint sequencing" << std::endl;
    
    std::cout << "\n✓ Intelligent ATC Interaction" << std::endl;
    std::cout << "  - Context-aware menu selection" << std::endl;
    std::cout << "  - AI-based scoring algorithm" << std::endl;
    std::cout << "  - Ollama LLM integration (optional)" << std::endl;
    std::cout << "  - Instruction parsing and execution" << std::endl;
    
    std::cout << "\n✓ Comprehensive Safety Systems" << std::endl;
    std::cout << "  - Fuel monitoring (20% and 10% warnings)" << std::endl;
    std::cout << "  - Terrain clearance checking" << std::endl;
    std::cout << "  - Weather assessment" << std::endl;
    std::cout << "  - Engine health monitoring" << std::endl;
    
    std::cout << "\n✓ Emergency Procedures" << std::endl;
    std::cout << "  - Engine failure recovery" << std::endl;
    std::cout << "  - Fire procedures" << std::endl;
    std::cout << "  - Loss of control recovery" << std::endl;
    std::cout << "  - Low fuel diversion" << std::endl;
    
    std::cout << "\n✓ Complete Systems Control" << std::endl;
    std::cout << "  - Autopilot (HDG, ALT, SPD, VS, NAV, APR)" << std::endl;
    std::cout << "  - Flight controls (pitch, roll, yaw)" << std::endl;
    std::cout << "  - Power management (throttle, mixture, prop)" << std::endl;
    std::cout << "  - Flight surfaces (flaps, gear, spoilers)" << std::endl;
    std::cout << "  - Ground operations (parking brake, brakes)" << std::endl;
    std::cout << "  - Lighting (nav, beacon, strobe, landing, taxi)" << std::endl;
    std::cout << "  - Engine management (start/stop, magnetos)" << std::endl;
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
}

int main(int argc, char* argv[]) {
    printBanner();
    
    // Parse command line arguments
    SimulatorType simType = SimulatorType::MSFS2024;
    std::string configPath;
    std::string planPath;
    bool verbose = false;
    bool enableOllama = false;
    std::string ollamaHost = "http://localhost:11434";
    std::string ollamaModel = "llama2";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        }
        else if (arg == "-s" || arg == "--simulator") {
            if (i + 1 < argc) {
                std::string simTypeStr = argv[++i];
                if (simTypeStr == "msfs2024") {
                    simType = SimulatorType::MSFS2024;
                } else if (simTypeStr == "p3dv6") {
                    simType = SimulatorType::P3D_V6;
                }
            }
        }
        else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
        else if (arg == "--ollama") {
            enableOllama = true;
        }
        else if (arg == "--ollama-host") {
            if (i + 1 < argc) {
                ollamaHost = argv[++i];
            }
        }
        else if (arg == "--ollama-model") {
            if (i + 1 < argc) {
                ollamaModel = argv[++i];
            }
        }
        else if (configPath.empty()) {
            configPath = arg;
        }
        else if (planPath.empty()) {
            planPath = arg;
        }
    }
    
    // Create AI Pilot instance
    AIPilot pilot;
    g_pilot = &pilot;
    
    // Setup signal handler for graceful shutdown
    std::signal(SIGINT, signalHandler);
    
    // Initialize connection to simulator
    std::cout << "\n[INIT] Connecting to simulator..." << std::endl;
    if (!pilot.initialize(simType)) {
        std::cerr << "[ERROR] Failed to initialize AI Pilot" << std::endl;
        return 1;
    }
    std::cout << "[OK] Connected successfully" << std::endl;
    
    // Enable Ollama if requested
    if (enableOllama) {
        std::cout << "\n[INIT] Enabling Ollama AI for ATC menu selection..." << std::endl;
        std::cout << "[INFO] Ollama host: " << ollamaHost << std::endl;
        std::cout << "[INFO] Ollama model: " << ollamaModel << std::endl;
        
        // Note: ATC controller initialization happens in startAutonomousFlight
        // We'll set it after that, so store the settings for now
    }
    
    // Load aircraft configuration (if provided)
    if (!configPath.empty()) {
        std::cout << "\n[INIT] Loading aircraft configuration: " << configPath << std::endl;
        if (!pilot.loadAircraftConfig(configPath)) {
            std::cerr << "[ERROR] Failed to load aircraft configuration" << std::endl;
            std::cerr << "[INFO] Continuing without aircraft-specific configuration" << std::endl;
        } else {
            std::cout << "[OK] Aircraft configuration loaded" << std::endl;
        }
    }
    
    // Load flight plan (if provided)
    if (!planPath.empty()) {
        std::cout << "\n[INIT] Loading flight plan: " << planPath << std::endl;
        if (!pilot.loadFlightPlan(planPath)) {
            std::cerr << "[ERROR] Failed to load flight plan" << std::endl;
            std::cerr << "[INFO] Will operate without predefined flight plan" << std::endl;
        } else {
            std::cout << "[OK] Flight plan loaded" << std::endl;
        }
    }
    
    // Demonstrate features
    demonstrateFeatures(pilot);
    
    // Start autonomous flight
    std::cout << "\n[START] Initiating autonomous flight operations..." << std::endl;
    std::cout << "[INFO] AI Copilot is now controlling the aircraft" << std::endl;
    
    pilot.startAutonomousFlight();
    
    // Enable Ollama after ATC controller is initialized
    if (enableOllama) {
        std::cout << "[OLLAMA] Enabling Ollama AI for ATC menu selection..." << std::endl;
        pilot.enableOllamaATC(true, ollamaHost);
        pilot.setOllamaModel(ollamaModel);
        
        if (pilot.isOllamaEnabled()) {
            std::cout << "[OLLAMA] ✓ Ollama AI enabled successfully" << std::endl;
        } else {
            std::cout << "[OLLAMA] ✗ Failed to enable Ollama, using rule-based selection" << std::endl;
        }
    }
    
    std::cout << "[INFO] Press Ctrl+C to stop and return control\n" << std::endl;
    
    // Main update loop
    int updateCount = 0;
    int statusInterval = verbose ? 20 : 40; // More frequent updates in verbose mode
    
    while (pilot.isActive()) {
        // Update at 20 Hz
        pilot.update();
        
        // Print status periodically
        if (updateCount % statusInterval == 0) {
            std::cout << "\n" << pilot.getStatusReport();
            std::cout << std::string(50, '-') << std::endl;
        }
        
        updateCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << "\n[STOP] AI Copilot has stopped autonomous operations" << std::endl;
    std::cout << "[INFO] Aircraft control returned to user" << std::endl;
    std::cout << "\nThank you for using AI Copilot FS!" << std::endl;
    
    return 0;
}

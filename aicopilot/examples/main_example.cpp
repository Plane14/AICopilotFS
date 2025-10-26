/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* Example application demonstrating the AI Copilot
*****************************************************************************/

#include "../include/ai_pilot.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace AICopilot;

int main(int argc, char* argv[]) {
    std::cout << "AI Copilot FS - Autonomous Pilot for MSFS2024 and Prepar3D V6" << std::endl;
    std::cout << "================================================================" << std::endl;
    
    // Create AI Pilot instance
    AIPilot pilot;
    
    // Initialize connection to simulator
    std::cout << "\nInitializing connection to simulator..." << std::endl;
    if (!pilot.initialize(SimulatorType::MSFS2024)) {
        std::cerr << "Failed to initialize AI Pilot" << std::endl;
        return 1;
    }
    
    // Load aircraft configuration (if provided)
    if (argc > 1) {
        std::cout << "\nLoading aircraft configuration: " << argv[1] << std::endl;
        if (!pilot.loadAircraftConfig(argv[1])) {
            std::cerr << "Failed to load aircraft configuration" << std::endl;
            return 1;
        }
    }
    
    // Load flight plan (if provided)
    if (argc > 2) {
        std::cout << "\nLoading flight plan: " << argv[2] << std::endl;
        if (!pilot.loadFlightPlan(argv[2])) {
            std::cerr << "Failed to load flight plan" << std::endl;
            return 1;
        }
    }
    
    // Start autonomous flight
    std::cout << "\nStarting autonomous flight..." << std::endl;
    pilot.startAutonomousFlight();
    
    // Main loop
    std::cout << "\nAI Pilot is active. Press Ctrl+C to stop." << std::endl;
    std::cout << "==========================================" << std::endl;
    
    int updateCount = 0;
    while (pilot.isActive()) {
        // Update at 20 Hz
        pilot.update();
        
        // Print status every 2 seconds
        if (updateCount % 40 == 0) {
            std::cout << "\n" << pilot.getStatusReport() << std::endl;
        }
        
        updateCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << "\nAI Pilot stopped." << std::endl;
    return 0;
}

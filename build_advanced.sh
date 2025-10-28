#!/bin/bash
# Script to build and test the new advanced procedures system
# Run from the build directory

echo "==================================================="
echo "Building Advanced Flight Procedures & Optimization"
echo "==================================================="

# Build only the new components
echo ""
echo "Building advanced_procedures..."
g++ -std=c++17 -O3 -c ../aicopilot/src/advanced_procedures.cpp -o advanced_procedures.o -I../aicopilot/include

echo "Building dynamic_flight_planning..."
g++ -std=c++17 -O3 -c ../aicopilot/src/dynamic_flight_planning.cpp -o dynamic_flight_planning.o -I../aicopilot/include

echo "Building performance_optimizer..."
g++ -std=c++17 -O3 -c ../aicopilot/src/performance_optimizer.cpp -o performance_optimizer.o -I../aicopilot/include

echo "Building system_monitor..."
g++ -std=c++17 -O3 -c ../aicopilot/src/system_monitor.cpp -o system_monitor.o -I../aicopilot/include

echo "Building test_advanced_procedures..."
g++ -std=c++17 -O2 -c ../aicopilot/tests/test_advanced_procedures.cpp -o test_advanced_procedures.o -I../aicopilot/include

echo ""
echo "==================================================="
echo "Build Complete!"
echo "==================================================="
echo ""
echo "Generated object files:"
ls -la *.o

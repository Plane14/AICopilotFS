cmake_minimum_required(VERSION 3.10)
project(AICopilotFS VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build unit tests" OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/aicopilot/include)

# Source files
set(AICOPILOT_SOURCES
    aicopilot/src/parsers/config_parser.cpp
    aicopilot/src/parsers/aircraft_config.cpp
    aicopilot/src/simconnect/simconnect_wrapper.cpp
    aicopilot/src/systems/aircraft_systems.cpp
    aicopilot/src/navigation/navigation.cpp
    aicopilot/src/atc/atc_controller.cpp
    aicopilot/src/ai/ai_pilot.cpp
)

# Header files
set(AICOPILOT_HEADERS
    aicopilot/include/aicopilot_types.h
    aicopilot/include/config_parser.h
    aicopilot/include/aircraft_config.h
    aicopilot/include/simconnect_wrapper.h
    aicopilot/include/aircraft_systems.h
    aicopilot/include/navigation.h
    aicopilot/include/atc_controller.h
    aicopilot/include/ai_pilot.h
)

# Create library
add_library(aicopilot STATIC ${AICOPILOT_SOURCES} ${AICOPILOT_HEADERS})

# SimConnect SDK paths (adjust these based on your installation)
# For MSFS2024
if(DEFINED ENV{MSFS_SDK_PATH})
    set(MSFS_SDK_PATH $ENV{MSFS_SDK_PATH})
    include_directories(${MSFS_SDK_PATH}/SimConnect SDK/include)
    link_directories(${MSFS_SDK_PATH}/SimConnect SDK/lib)
    # target_link_libraries(aicopilot SimConnect)
endif()

# For Prepar3D V6
if(DEFINED ENV{P3D_SDK_PATH})
    set(P3D_SDK_PATH $ENV{P3D_SDK_PATH})
    include_directories(${P3D_SDK_PATH}/inc)
    link_directories(${P3D_SDK_PATH}/lib)
    # target_link_libraries(aicopilot SimConnect)
endif()

# Build examples
if(BUILD_EXAMPLES)
    add_executable(aicopilot_example aicopilot/examples/main_example.cpp)
    target_link_libraries(aicopilot_example aicopilot)
endif()

# Build tests
if(BUILD_TESTS)
    enable_testing()
    # Add test files here when created
endif()

# Installation
install(TARGETS aicopilot
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${AICOPILOT_HEADERS}
    DESTINATION include/aicopilot
)

if(BUILD_EXAMPLES)
    install(TARGETS aicopilot_example
        RUNTIME DESTINATION bin
    )
endif()

# Print build configuration
message(STATUS "AI Copilot FS Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
if(DEFINED MSFS_SDK_PATH)
    message(STATUS "  MSFS SDK path: ${MSFS_SDK_PATH}")
endif()
if(DEFINED P3D_SDK_PATH)
    message(STATUS "  P3D SDK path: ${P3D_SDK_PATH}")
endif()

cmake_minimum_required(VERSION 3.10)
project(AICopilotFS VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build unit tests" OFF)
option(USE_MSFS_2024_SDK "Build with MSFS 2024 SDK" ON)
option(USE_P3D_V6_SDK "Build with Prepar3D v6 SDK" OFF)
option(SUPPORT_BOTH_SDKS "Build with support for both SDKs (runtime detection)" OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Windows-specific settings (SimConnect is Windows-only)
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 or later
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    
    # Detect architecture
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM_ARCH "x64")
        message(STATUS "Building for 64-bit Windows")
    else()
        set(PLATFORM_ARCH "x86")
        message(STATUS "Building for 32-bit Windows")
    endif()
else()
    message(FATAL_ERROR "This project requires Windows for SimConnect support")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/aicopilot/include)

# SimConnect SDK Configuration
set(SIMCONNECT_FOUND FALSE)
set(SIMCONNECT_LIBRARIES "")
set(SIMCONNECT_INCLUDE_DIRS "")

# SDK Base Paths
set(MSFS_2024_SDK_BASE "c:/MSFS 2024 SDK")
set(P3D_V6_SDK_BASE "d:/Lockheed Martin/Prepar3D v6 SDK")

# Determine which SDK(s) to use
if(SUPPORT_BOTH_SDKS)
    message(STATUS "Configuring support for both MSFS 2024 and Prepar3D v6 SDKs")
    set(USE_MSFS_2024_SDK ON)
    set(USE_P3D_V6_SDK ON)
endif()

# Configure MSFS 2024 SDK
if(USE_MSFS_2024_SDK)
    set(MSFS_SIMCONNECT_INCLUDE "${MSFS_2024_SDK_BASE}/SimConnect SDK/include")
    set(MSFS_SIMCONNECT_LIB_DIR "${MSFS_2024_SDK_BASE}/SimConnect SDK/lib")
    
    if(EXISTS "${MSFS_SIMCONNECT_INCLUDE}/SimConnect.h")
        message(STATUS "MSFS 2024 SDK found at: ${MSFS_2024_SDK_BASE}")
        message(STATUS "  Include: ${MSFS_SIMCONNECT_INCLUDE}")
        message(STATUS "  Lib Dir: ${MSFS_SIMCONNECT_LIB_DIR}")
        
        list(APPEND SIMCONNECT_INCLUDE_DIRS "${MSFS_SIMCONNECT_INCLUDE}")
        
        # Check for the appropriate library based on architecture
        if(PLATFORM_ARCH STREQUAL "x64")
            set(MSFS_SIMCONNECT_LIB "${MSFS_SIMCONNECT_LIB_DIR}/SimConnect.lib")
        else()
            set(MSFS_SIMCONNECT_LIB "${MSFS_SIMCONNECT_LIB_DIR}/SimConnect.lib")
        endif()
        
        if(EXISTS "${MSFS_SIMCONNECT_LIB}")
            list(APPEND SIMCONNECT_LIBRARIES "${MSFS_SIMCONNECT_LIB}")
            set(SIMCONNECT_FOUND TRUE)
            add_definitions(-DUSE_MSFS2024_SDK)
            message(STATUS "  Library: ${MSFS_SIMCONNECT_LIB}")
        else()
            message(WARNING "MSFS 2024 SimConnect library not found at: ${MSFS_SIMCONNECT_LIB}")
        endif()
    else()
        message(WARNING "MSFS 2024 SDK not found at expected location: ${MSFS_2024_SDK_BASE}")
        if(NOT USE_P3D_V6_SDK)
            message(WARNING "Consider setting USE_P3D_V6_SDK to ON or check the SDK installation")
        endif()
    endif()
endif()

# Configure Prepar3D v6 SDK
if(USE_P3D_V6_SDK)
    set(P3D_SIMCONNECT_INCLUDE "${P3D_V6_SDK_BASE}/inc/SimConnect")
    set(P3D_SIMCONNECT_LIB_DIR "${P3D_V6_SDK_BASE}/lib/SimConnect")
    
    if(EXISTS "${P3D_SIMCONNECT_INCLUDE}/SimConnect.h")
        message(STATUS "Prepar3D v6 SDK found at: ${P3D_V6_SDK_BASE}")
        message(STATUS "  Include: ${P3D_SIMCONNECT_INCLUDE}")
        message(STATUS "  Lib Dir: ${P3D_SIMCONNECT_LIB_DIR}")
        
        list(APPEND SIMCONNECT_INCLUDE_DIRS "${P3D_SIMCONNECT_INCLUDE}")
        
        # Check for the appropriate library based on architecture
        if(PLATFORM_ARCH STREQUAL "x64")
            set(P3D_SIMCONNECT_LIB "${P3D_SIMCONNECT_LIB_DIR}/SimConnect.lib")
        else()
            set(P3D_SIMCONNECT_LIB "${P3D_SIMCONNECT_LIB_DIR}/SimConnect.lib")
        endif()
        
        if(EXISTS "${P3D_SIMCONNECT_LIB}")
            list(APPEND SIMCONNECT_LIBRARIES "${P3D_SIMCONNECT_LIB}")
            set(SIMCONNECT_FOUND TRUE)
            add_definitions(-DUSE_P3D_SDK)
            message(STATUS "  Library: ${P3D_SIMCONNECT_LIB}")
        else()
            message(WARNING "Prepar3D v6 SimConnect library not found at: ${P3D_SIMCONNECT_LIB}")
        endif()
    else()
        message(WARNING "Prepar3D v6 SDK not found at expected location: ${P3D_V6_SDK_BASE}")
        if(NOT USE_MSFS_2024_SDK)
            message(WARNING "Consider setting USE_MSFS_2024_SDK to ON or check the SDK installation")
        endif()
    endif()
endif()

# Check if at least one SDK was found
if(NOT SIMCONNECT_FOUND)
    message(FATAL_ERROR "No SimConnect SDK found! Please:\n"
                        "  1. Install MSFS 2024 SDK to: ${MSFS_2024_SDK_BASE}\n"
                        "  2. OR Install Prepar3D v6 SDK to: ${P3D_V6_SDK_BASE}\n"
                        "  3. OR set USE_MSFS_2024_SDK or USE_P3D_V6_SDK options accordingly")
endif()

# Add SimConnect include directories
include_directories(${SIMCONNECT_INCLUDE_DIRS})

# Optional: Find packages for Ollama integration
option(ENABLE_OLLAMA "Enable Ollama AI integration (requires CURL and jsoncpp)" OFF)

if(ENABLE_OLLAMA)
    find_package(CURL)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(JSONCPP jsoncpp)
    endif()
    
    if(CURL_FOUND AND JSONCPP_FOUND)
        message(STATUS "Ollama integration enabled")
        add_definitions(-DENABLE_OLLAMA)
        set(OLLAMA_SOURCES aicopilot/src/atc/ollama_client_impl.cpp)
        set(OLLAMA_HEADERS aicopilot/include/ollama_client.h)
        set(OLLAMA_LIBRARIES ${CURL_LIBRARIES} ${JSONCPP_LIBRARIES})
        include_directories(${JSONCPP_INCLUDE_DIRS})
    else()
        message(WARNING "Ollama integration requires CURL and jsoncpp. Disabling Ollama support.")
        set(ENABLE_OLLAMA OFF)
        set(OLLAMA_SOURCES aicopilot/src/atc/ollama_client_stub.cpp)
    endif()
else()
    message(STATUS "Ollama integration disabled (use -DENABLE_OLLAMA=ON to enable)")
    set(OLLAMA_SOURCES aicopilot/src/atc/ollama_client_stub.cpp)
endif()

# Source files
set(AICOPILOT_SOURCES
    aicopilot/src/parsers/config_parser.cpp
    aicopilot/src/parsers/aircraft_config.cpp
    aicopilot/src/simconnect/simconnect_wrapper.cpp
    aicopilot/src/systems/aircraft_systems.cpp
    aicopilot/src/navigation/navigation.cpp
    aicopilot/src/navdata/navdata_providers.cpp
    aicopilot/src/atc/atc_controller.cpp
    aicopilot/src/ai/ai_pilot.cpp
    aicopilot/src/weather/weather_system.cpp
    aicopilot/src/terrain/terrain_awareness.cpp
    aicopilot/src/traffic/traffic_system.cpp
    aicopilot/src/approach/approach_system.cpp
    aicopilot/src/profiles/aircraft_profile.cpp
    aicopilot/src/voice/voice_interface.cpp
    aicopilot/src/ml/ml_decision_system.cpp
    aicopilot/src/helicopter/helicopter_operations.cpp
    aicopilot/src/airport/airport_manager.cpp
    aicopilot/src/airport/airport_integration.cpp
    ${OLLAMA_SOURCES}
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
    aicopilot/include/weather_system.h
    aicopilot/include/terrain_awareness.h
    aicopilot/include/traffic_system.h
    aicopilot/include/approach_system.h
    aicopilot/include/aircraft_profile.h
    aicopilot/include/voice_interface.h
    aicopilot/include/ml_decision_system.h
    aicopilot/include/helicopter_operations.h
    aicopilot/include/airport_manager.h
    aicopilot/include/airport_integration.hpp
    ${OLLAMA_HEADERS}
)

# Create library
add_library(aicopilot STATIC ${AICOPILOT_SOURCES} ${AICOPILOT_HEADERS})

# Link required libraries
target_link_libraries(aicopilot 
    ${OLLAMA_LIBRARIES}
    ${SIMCONNECT_LIBRARIES}
)

target_include_directories(aicopilot PRIVATE 
    ${SIMCONNECT_INCLUDE_DIRS}
)

# Add Ollama include directories if enabled
if(ENABLE_OLLAMA AND JSONCPP_FOUND)
    target_include_directories(aicopilot PRIVATE ${JSONCPP_INCLUDE_DIRS})
endif()

# Add Windows system libraries required by SimConnect
if(WIN32)
    target_link_libraries(aicopilot 
        ws2_32      # Winsock
        winmm       # Windows Multimedia
    )
endif()

# Build examples
if(BUILD_EXAMPLES)
    add_executable(aicopilot_example aicopilot/examples/main_example.cpp)
    target_link_libraries(aicopilot_example aicopilot)
    
    add_executable(aicopilot_advanced aicopilot/examples/advanced_example.cpp)
    target_link_libraries(aicopilot_advanced aicopilot)
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
    install(TARGETS aicopilot_example aicopilot_advanced
        RUNTIME DESTINATION bin
    )
endif()

# Print build configuration
message(STATUS "")
message(STATUS "==================================================")
message(STATUS "AI Copilot FS Configuration:")
message(STATUS "==================================================")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME} ${PLATFORM_ARCH}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "")
message(STATUS "SimConnect SDK Configuration:")
if(USE_MSFS_2024_SDK AND EXISTS "${MSFS_SIMCONNECT_INCLUDE}/SimConnect.h")
    message(STATUS "  ✓ MSFS 2024 SDK: ENABLED")
    message(STATUS "    Path: ${MSFS_2024_SDK_BASE}")
    message(STATUS "    Include: ${MSFS_SIMCONNECT_INCLUDE}")
    message(STATUS "    Library: ${MSFS_SIMCONNECT_LIB}")
endif()
if(USE_P3D_V6_SDK AND EXISTS "${P3D_SIMCONNECT_INCLUDE}/SimConnect.h")
    message(STATUS "  ✓ Prepar3D v6 SDK: ENABLED")
    message(STATUS "    Path: ${P3D_V6_SDK_BASE}")
    message(STATUS "    Include: ${P3D_SIMCONNECT_INCLUDE}")
    message(STATUS "    Library: ${P3D_SIMCONNECT_LIB}")
endif()
if(SUPPORT_BOTH_SDKS)
    message(STATUS "  Runtime SDK Detection: ENABLED")
endif()
message(STATUS "")
message(STATUS "Preprocessor Definitions:")
get_directory_property(COMPILE_DEFS COMPILE_DEFINITIONS)
foreach(DEF ${COMPILE_DEFS})
    message(STATUS "  - ${DEF}")
endforeach()
message(STATUS "==================================================")
message(STATUS "")

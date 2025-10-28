/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PHASE 1: ERROR HANDLING AND INPUT VALIDATION
* Test Suite: validation_and_error_handling_tests.hpp
* 
* PURPOSE:
* - Comprehensive test cases for validation framework
* - Error handling verification
* - Edge case detection
* - Coordinate system testing
* - Integration testing
*
* INCLUDES 30+ TEST CASES:
* - Coordinate validation (latitude, longitude bounds)
* - Altitude validation (min/max, vertical speed)
* - Airspeed validation (IAS, TAS, ground speed)
* - Heading validation (0-360 range, normalization)
* - Weight and balance validation
* - Waypoint validation
* - Flight plan validation
* - Edge cases (poles, antimeridian)
* - Distance calculations
* - Bearing calculations
* - Error handling and recovery
* - SimConnect error scenarios
* - Data validation
*****************************************************************************/

#ifndef VALIDATION_AND_ERROR_HANDLING_TESTS_HPP
#define VALIDATION_AND_ERROR_HANDLING_TESTS_HPP

#include "error_handling.hpp"
#include "validation_framework.hpp"
#include "coordinate_utils.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <iomanip>

namespace AICopilot {
namespace Tests {

// ============================================================================
// TEST RESULT TRACKING
// ============================================================================

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    
    TestResult(const std::string& n, bool p, const std::string& msg = "")
        : name(n), passed(p), message(msg) {}
};

class TestRunner {
private:
    std::vector<TestResult> results_;
    int passCount_ = 0;
    int failCount_ = 0;

public:
    void addResult(const TestResult& result) {
        results_.push_back(result);
        if (result.passed) {
            passCount_++;
        } else {
            failCount_++;
        }
    }

    void printSummary() const {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "TEST SUMMARY\n";
        std::cout << std::string(70, '=') << "\n";
        std::cout << "Total: " << (passCount_ + failCount_) << "\n";
        std::cout << "Passed: " << passCount_ << "\n";
        std::cout << "Failed: " << failCount_ << "\n";
        std::cout << std::string(70, '-') << "\n";

        for (const auto& result : results_) {
            std::cout << (result.passed ? "[PASS]" : "[FAIL]") << " " 
                      << std::setw(50) << std::left << result.name;
            if (!result.message.empty()) {
                std::cout << " - " << result.message;
            }
            std::cout << "\n";
        }
        std::cout << std::string(70, '=') << "\n";
    }
};

// ============================================================================
// COORDINATE VALIDATION TESTS (6 tests)
// ============================================================================

TestResult testLatitudeValidation() {
    auto valid = CoordinateValidator::validateLatitude(45.0);
    auto invalid_high = CoordinateValidator::validateLatitude(91.0);
    auto invalid_low = CoordinateValidator::validateLatitude(-91.0);
    
    bool pass = valid.isValid && !invalid_high.isValid && !invalid_low.isValid;
    return TestResult("Latitude Validation", pass);
}

TestResult testLongitudeValidation() {
    auto valid = CoordinateValidator::validateLongitude(135.0);
    auto invalid_high = CoordinateValidator::validateLongitude(181.0);
    auto invalid_low = CoordinateValidator::validateLongitude(-181.0);
    
    bool pass = valid.isValid && !invalid_high.isValid && !invalid_low.isValid;
    return TestResult("Longitude Validation", pass);
}

TestResult testCoordinatePairValidation() {
    auto valid = CoordinateValidator::validateCoordinatePair(45.0, 135.0);
    auto invalid_lat = CoordinateValidator::validateCoordinatePair(91.0, 135.0);
    auto invalid_lon = CoordinateValidator::validateCoordinatePair(45.0, 181.0);
    
    bool pass = valid.isValid && !invalid_lat.isValid && !invalid_lon.isValid;
    return TestResult("Coordinate Pair Validation", pass);
}

TestResult testPoleCrossingDetection() {
    auto nearNorthPole = CoordinateValidator::checkPoleCrossing(88.0);
    auto nearSouthPole = CoordinateValidator::checkPoleCrossing(-88.0);
    auto normalLat = CoordinateValidator::checkPoleCrossing(45.0);
    
    bool pass = !nearNorthPole.isValid && !nearSouthPole.isValid && normalLat.isValid;
    return TestResult("Pole Crossing Detection", pass);
}

TestResult testAntimeridianCrossingDetection() {
    auto crosses = CoordinateValidator::checkAntimeridianCrossing(170.0, -170.0);
    auto noCross = CoordinateValidator::checkAntimeridianCrossing(45.0, 135.0);
    
    bool pass = !crosses.isValid && noCross.isValid;
    return TestResult("Antimeridian Crossing Detection", pass);
}

TestResult testCoordinateNormalization() {
    double lon1 = CoordinateValidator::normalizeLongitude(185.0);
    double lon2 = CoordinateValidator::normalizeLongitude(-185.0);
    double lon3 = CoordinateValidator::normalizeLongitude(360.0);
    
    bool pass = (lon1 > -180.0 && lon1 <= 180.0) &&
                (lon2 > -180.0 && lon2 <= 180.0) &&
                (lon3 > -180.0 && lon3 <= 180.0);
    return TestResult("Coordinate Normalization", pass);
}

// ============================================================================
// ALTITUDE VALIDATION TESTS (4 tests)
// ============================================================================

TestResult testAltitudeValidation() {
    auto valid = AltitudeValidator::validateAltitude(10000.0);
    auto tooLow = AltitudeValidator::validateAltitude(-100.0);
    auto tooHigh = AltitudeValidator::validateAltitude(60000.0);
    
    bool pass = valid.isValid && !tooLow.isValid && !tooHigh.isValid;
    return TestResult("Altitude Range Validation", pass);
}

TestResult testVerticalSpeedValidation() {
    auto validClimb = AltitudeValidator::validateVerticalSpeed(1000.0);
    auto validDescent = AltitudeValidator::validateVerticalSpeed(-1000.0);
    auto tooHigh = AltitudeValidator::validateVerticalSpeed(5000.0);
    
    bool pass = validClimb.isValid && validDescent.isValid && !tooHigh.isValid;
    return TestResult("Vertical Speed Validation", pass);
}

TestResult testAltitudeAtSeaLevel() {
    auto result = AltitudeValidator::validateAltitude(0.0);
    bool pass = result.isValid;
    return TestResult("Altitude at Sea Level", pass);
}

TestResult testMaxAircraftAltitude() {
    auto result = AltitudeValidator::validateAltitude(50000.0);
    bool pass = result.isValid;
    return TestResult("Maximum Aircraft Altitude", pass);
}

// ============================================================================
// AIRSPEED VALIDATION TESTS (4 tests)
// ============================================================================

TestResult testIndicatedAirspeedValidation() {
    auto valid = AirspeedValidator::validateIndicatedAirspeed(100.0);
    auto tooHigh = AirspeedValidator::validateIndicatedAirspeed(250.0);
    auto negative = AirspeedValidator::validateIndicatedAirspeed(-50.0);
    
    bool pass = valid.isValid && !tooHigh.isValid && !negative.isValid;
    return TestResult("IAS Range Validation", pass);
}

TestResult testTrueAirspeedValidation() {
    auto valid = AirspeedValidator::validateTrueAirspeed(150.0);
    auto tooHigh = AirspeedValidator::validateTrueAirspeed(700.0);
    auto negative = AirspeedValidator::validateTrueAirspeed(-50.0);
    
    bool pass = valid.isValid && !tooHigh.isValid && !negative.isValid;
    return TestResult("TAS Range Validation", pass);
}

TestResult testGroundSpeedValidation() {
    auto valid = AirspeedValidator::validateGroundSpeed(120.0);
    auto zero = AirspeedValidator::validateGroundSpeed(0.0);
    auto negative = AirspeedValidator::validateGroundSpeed(-50.0);
    
    bool pass = valid.isValid && zero.isValid && !negative.isValid;
    return TestResult("Ground Speed Validation", pass);
}

TestResult testAirspeedUnderStall() {
    auto stalling = AirspeedValidator::validateIndicatedAirspeed(30.0);
    // Should be valid (stall is not a validation error, it's a flight dynamics issue)
    bool pass = stalling.isValid;
    return TestResult("Airspeed Below Stall Speed", pass);
}

// ============================================================================
// HEADING VALIDATION TESTS (3 tests)
// ============================================================================

TestResult testHeadingValidation() {
    auto north = HeadingValidator::validateHeading(0.0);
    auto south = HeadingValidator::validateHeading(180.0);
    auto invalid = HeadingValidator::validateHeading(400.0);
    
    bool pass = north.isValid && south.isValid && !invalid.isValid;
    return TestResult("Heading Range Validation", pass);
}

TestResult testHeadingNormalization() {
    double norm1 = HeadingValidator::normalizeHeading(-45.0);
    double norm2 = HeadingValidator::normalizeHeading(400.0);
    double norm3 = HeadingValidator::normalizeHeading(180.0);
    
    bool pass = (norm1 >= 0.0 && norm1 < 360.0) &&
                (norm2 >= 0.0 && norm2 < 360.0) &&
                (norm3 >= 0.0 && norm3 < 360.0);
    return TestResult("Heading Normalization", pass);
}

TestResult testHeadingDifference() {
    double diff1 = HeadingValidator::headingDifference(10.0, 20.0);
    double diff2 = HeadingValidator::headingDifference(350.0, 10.0);
    
    bool pass = (diff1 == 10.0) && (diff2 == 20.0);
    return TestResult("Heading Difference Calculation", pass);
}

// ============================================================================
// WEIGHT AND BALANCE TESTS (2 tests)
// ============================================================================

TestResult testWeightValidation() {
    auto valid = WeightBalanceValidator::validateWeight(3000.0, 5000.0);
    auto tooHeavy = WeightBalanceValidator::validateWeight(6000.0, 5000.0);
    auto negative = WeightBalanceValidator::validateWeight(-100.0, 5000.0);
    
    bool pass = valid.isValid && !tooHeavy.isValid && !negative.isValid;
    return TestResult("Aircraft Weight Validation", pass);
}

TestResult testCGValidation() {
    auto valid = WeightBalanceValidator::validateCG(25.0, 20.0, 30.0);
    auto tooForward = WeightBalanceValidator::validateCG(15.0, 20.0, 30.0);
    auto tooAft = WeightBalanceValidator::validateCG(35.0, 20.0, 30.0);
    
    bool pass = valid.isValid && !tooForward.isValid && !tooAft.isValid;
    return TestResult("Center of Gravity Validation", pass);
}

// ============================================================================
// WAYPOINT VALIDATION TESTS (3 tests)
// ============================================================================

TestResult testWaypointValidation() {
    Waypoint wp;
    wp.id = "TEST";
    wp.position = {45.0, 135.0, 10000.0, 0.0};
    wp.altitude = 10000.0;
    wp.type = "FIX";
    
    auto result = WaypointValidator::validateWaypoint(wp);
    bool pass = result.isValid;
    return TestResult("Waypoint Structure Validation", pass);
}

TestResult testWaypointInvalidCoordinates() {
    Waypoint wp;
    wp.id = "BAD";
    wp.position = {91.0, 135.0, 10000.0, 0.0};  // Invalid latitude
    wp.altitude = 10000.0;
    wp.type = "FIX";
    
    auto result = WaypointValidator::validateWaypoint(wp);
    bool pass = !result.isValid;
    return TestResult("Invalid Waypoint Detection", pass);
}

TestResult testWaypointSequenceValidation() {
    std::vector<Waypoint> waypoints;
    
    Waypoint wp1;
    wp1.id = "DEP";
    wp1.position = {45.0, 135.0, 5000.0, 0.0};
    wp1.altitude = 5000.0;
    waypoints.push_back(wp1);
    
    Waypoint wp2;
    wp2.id = "ARR";
    wp2.position = {50.0, 140.0, 5000.0, 0.0};
    wp2.altitude = 5000.0;
    waypoints.push_back(wp2);
    
    auto result = WaypointValidator::validateWaypointSequence(waypoints);
    bool pass = result.isValid;
    return TestResult("Waypoint Sequence Validation", pass);
}

// ============================================================================
// COORDINATE CALCULATION TESTS (5 tests)
// ============================================================================

TestResult testGreatCircleDistance() {
    // London to Paris (approx 215 NM)
    double dist = CoordinateUtils::haversineDistance(
        51.5074, -0.1278, 48.8566, 2.3522);
    
    bool pass = (dist > 200.0 && dist < 250.0);
    return TestResult("Great Circle Distance", pass,
        "Distance: " + std::to_string(dist) + " NM");
}

TestResult testBearingCalculation() {
    // North bearing should be ~0
    double bearing = CoordinateUtils::calculateBearing(
        0.0, 0.0, 1.0, 0.0);
    
    bool pass = (bearing > 350.0 || bearing < 10.0);
    return TestResult("Bearing Calculation", pass,
        "Bearing: " + std::to_string(bearing) + "°");
}

TestResult testDestinationPoint() {
    // Start at 0,0 and go 60 NM north
    Position dest = CoordinateUtils::destinationPoint(0.0, 0.0, 0.0, 60.0);
    
    bool pass = (dest.latitude > 0.9 && dest.latitude < 1.1);
    return TestResult("Destination Point Calculation", pass,
        "Destination: " + std::to_string(dest.latitude) + "°");
}

TestResult testCoordinateInterpolation() {
    Position interp = CoordinateUtils::interpolateCoordinate(
        0.0, 0.0, 2.0, 2.0, 0.5);
    
    bool pass = (std::abs(interp.latitude - 1.0) < 0.01) &&
                (std::abs(interp.longitude - 1.0) < 0.01);
    return TestResult("Coordinate Interpolation", pass);
}

TestResult testPathCrossesAntimeridian() {
    bool crosses = CoordinateUtils::pathCrossesAntimeridian(
        45.0, 170.0, 45.0, -170.0);
    
    bool pass = crosses;
    return TestResult("Antimeridian Path Detection", pass);
}

// ============================================================================
// EDGE CASE DETECTION TESTS (4 tests)
// ============================================================================

TestResult testNorthPoleEdgeCase() {
    auto ec = CoordinateUtils::detectEdgeCases(90.0, 0.0);
    bool pass = ec.isAtNorthPole;
    return TestResult("North Pole Detection", pass);
}

TestResult testSouthPoleEdgeCase() {
    auto ec = CoordinateUtils::detectEdgeCases(-90.0, 0.0);
    bool pass = ec.isAtSouthPole;
    return TestResult("South Pole Detection", pass);
}

TestResult testAntimeridianEdgeCase() {
    auto ec = CoordinateUtils::detectEdgeCases(45.0, 180.0);
    bool pass = ec.isAntimeridian;
    return TestResult("Antimeridian Edge Case", pass);
}

TestResult testPrimeMeridianEdgeCase() {
    auto ec = CoordinateUtils::detectEdgeCases(45.0, 0.0);
    bool pass = ec.isPrimeMeridian;
    return TestResult("Prime Meridian Edge Case", pass);
}

// ============================================================================
// ERROR HANDLING TESTS (3 tests)
// ============================================================================

TestResult testValidationExceptionThrow() {
    bool caught = false;
    try {
        throw ValidationException(
            ErrorCode::VALIDATION_INVALID_LATITUDE,
            "Test exception",
            "latitude", "91.0"
        );
    } catch (const ValidationException& e) {
        caught = (e.getErrorCode() == ErrorCode::VALIDATION_INVALID_LATITUDE);
    }
    return TestResult("Validation Exception", caught);
}

TestResult testConnectionException() {
    bool caught = false;
    try {
        throw ConnectionException(
            ErrorCode::SIMCONNECT_CONNECTION_FAILED,
            "Test connection error",
            true, 5
        );
    } catch (const ConnectionException& e) {
        caught = e.canAttemptReconnection();
    }
    return TestResult("Connection Exception", caught);
}

TestResult testErrorMessageMapping() {
    auto msg = ErrorMessageMap::getMessage(ErrorCode::VALIDATION_INVALID_LATITUDE);
    bool pass = !msg.empty() && msg.find("Latitude") != std::string::npos;
    return TestResult("Error Message Mapping", pass);
}

// ============================================================================
// DATA VALIDATION TESTS (3 tests)
// ============================================================================

TestResult testPointerValidation() {
    int value = 42;
    int* ptr = &value;
    int* nullPtr = nullptr;
    
    auto validResult = DataValidator::validatePointer(ptr, "test");
    auto invalidResult = DataValidator::validatePointer(nullPtr, "test");
    
    bool pass = validResult.isValid && !invalidResult.isValid;
    return TestResult("Pointer Validation", pass);
}

TestResult testNonEmptyStringValidation() {
    auto validResult = DataValidator::validateNonEmptyString("test", "name");
    auto invalidResult = DataValidator::validateNonEmptyString("", "name");
    
    bool pass = validResult.isValid && !invalidResult.isValid;
    return TestResult("Non-Empty String Validation", pass);
}

TestResult testNumericRangeValidation() {
    auto validResult = DataValidator::validateRange(50, 0, 100, "percentage");
    auto invalidResult = DataValidator::validateRange(150, 0, 100, "percentage");
    
    bool pass = validResult.isValid && !invalidResult.isValid;
    return TestResult("Numeric Range Validation", pass);
}

// ============================================================================
// TEST EXECUTION
// ============================================================================

void runAllTests() {
    TestRunner runner;

    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "RUNNING VALIDATION AND ERROR HANDLING TESTS\n";
    std::cout << std::string(70, '=') << "\n\n";

    // Coordinate tests
    runner.addResult(testLatitudeValidation());
    runner.addResult(testLongitudeValidation());
    runner.addResult(testCoordinatePairValidation());
    runner.addResult(testPoleCrossingDetection());
    runner.addResult(testAntimeridianCrossingDetection());
    runner.addResult(testCoordinateNormalization());

    // Altitude tests
    runner.addResult(testAltitudeValidation());
    runner.addResult(testVerticalSpeedValidation());
    runner.addResult(testAltitudeAtSeaLevel());
    runner.addResult(testMaxAircraftAltitude());

    // Airspeed tests
    runner.addResult(testIndicatedAirspeedValidation());
    runner.addResult(testTrueAirspeedValidation());
    runner.addResult(testGroundSpeedValidation());
    runner.addResult(testAirspeedUnderStall());

    // Heading tests
    runner.addResult(testHeadingValidation());
    runner.addResult(testHeadingNormalization());
    runner.addResult(testHeadingDifference());

    // Weight and balance tests
    runner.addResult(testWeightValidation());
    runner.addResult(testCGValidation());

    // Waypoint tests
    runner.addResult(testWaypointValidation());
    runner.addResult(testWaypointInvalidCoordinates());
    runner.addResult(testWaypointSequenceValidation());

    // Coordinate calculation tests
    runner.addResult(testGreatCircleDistance());
    runner.addResult(testBearingCalculation());
    runner.addResult(testDestinationPoint());
    runner.addResult(testCoordinateInterpolation());
    runner.addResult(testPathCrossesAntimeridian());

    // Edge case tests
    runner.addResult(testNorthPoleEdgeCase());
    runner.addResult(testSouthPoleEdgeCase());
    runner.addResult(testAntimeridianEdgeCase());
    runner.addResult(testPrimeMeridianEdgeCase());

    // Error handling tests
    runner.addResult(testValidationExceptionThrow());
    runner.addResult(testConnectionException());
    runner.addResult(testErrorMessageMapping());

    // Data validation tests
    runner.addResult(testPointerValidation());
    runner.addResult(testNonEmptyStringValidation());
    runner.addResult(testNumericRangeValidation());

    runner.printSummary();
}

}} // namespace AICopilot::Tests

#endif // VALIDATION_AND_ERROR_HANDLING_TESTS_HPP

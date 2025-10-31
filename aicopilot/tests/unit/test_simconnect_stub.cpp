// Simple unit test to validate SimConnectWrapper public API in stub mode
// This test requires BUILD_TESTS=ON and googletest available via CMake

#include "gtest/gtest.h"
#include "../../include/simconnect_wrapper.h"

using namespace AICopilot;

TEST(SimConnectWrapperStub, PublicAPI_NoCrash) {
    // In stub mode (no SimConnect SDK), the wrapper should be constructible
    SimConnectWrapper wrapper;

    // connect should return false (no real SimConnect available) but must not crash
    bool connected = wrapper.connect(SimulatorType::UNKNOWN, "UnitTest");
    (void)connected;

    // Call a variety of methods to ensure no undefined behavior in stub mode
    wrapper.disconnect();
    EXPECT_NO_FATAL_FAILURE(wrapper.processMessages());

    // Query methods should return default-constructed values
    auto state = wrapper.getAircraftState();
    auto ap = wrapper.getAutopilotState();
    auto pos = wrapper.getPosition();
    (void)state; (void)ap; (void)pos;

    // Control methods should be no-ops
    EXPECT_NO_FATAL_FAILURE(wrapper.setThrottle(0.5));
    EXPECT_NO_FATAL_FAILURE(wrapper.setElevator(0.0));
    EXPECT_NO_FATAL_FAILURE(wrapper.setAileron(0.0));
    EXPECT_NO_FATAL_FAILURE(wrapper.setRudder(0.0));
    EXPECT_NO_FATAL_FAILURE(wrapper.setFlaps(10));
    EXPECT_NO_FATAL_FAILURE(wrapper.setGear(true));
    EXPECT_NO_FATAL_FAILURE(wrapper.setParkingBrake(true));
    EXPECT_NO_FATAL_FAILURE(wrapper.setBrakes(0.2));
    EXPECT_NO_FATAL_FAILURE(wrapper.setMixture(0.8));
    EXPECT_NO_FATAL_FAILURE(wrapper.setPropellerPitch(0.5));
    EXPECT_NO_FATAL_FAILURE(wrapper.setMagnetos(3));
    EXPECT_NO_FATAL_FAILURE(wrapper.toggleEngineStarter(1));
    EXPECT_NO_FATAL_FAILURE(wrapper.setEngineState(1, true));
    EXPECT_NO_FATAL_FAILURE(wrapper.setLight("NAV", true));
    EXPECT_NO_FATAL_FAILURE(wrapper.requestATCMenu());

    // Subscription registration should be safe in stub mode
    wrapper.subscribeToAircraftState([](const AircraftState& s) { (void)s; });
    wrapper.subscribeToATCMessages([](const ATCMessage& m) { (void)m; });
}

set(TEST_SOURCES
    clearance_state_machine_test.cpp
)

add_executable(aicopilot_unit_tests ${TEST_SOURCES})

target_link_libraries(aicopilot_unit_tests PRIVATE aicopilot gtest gtest_main)

gtest_discover_tests(aicopilot_unit_tests)

# Test Infrastructure Quick Reference

**Last Updated:** October 27, 2025  
**Status:** Framework & Analysis Complete - Ready for Implementation

---

## 📊 At-A-Glance Status

| Metric | Current | Target | Gap |
|--------|---------|--------|-----|
| **Test Files** | 1 | 24 | -23 ❌ |
| **Test Cases** | ~20 | 250+ | -230 ❌ |
| **Components Tested** | 1/17 | 17/17 | -16 ❌ |
| **Code Coverage** | <5% | 70-80% | -65-75% ❌ |
| **Unit Tests** | P1-only | All | ❌ |
| **Integration Tests** | 0 | 7 | -7 ❌ |
| **Mock Infrastructure** | Minimal | Complete | ❌ |
| **CI/CD Tests** | Disabled | Enabled | ❌ |
| **Test Framework** | gtest | gtest + gmock | ⚠️ |

**Overall Status:** CRITICAL TEST GAPS 🛑

---

## 🚀 Quick Start (1 Day)

### Step 1: Enable Tests in Build (15 min)
```cmake
# CMakeLists.txt, line 18
option(BUILD_TESTS "Build unit tests" ON)  # Change OFF → ON
```

### Step 2: Update CMakeLists.txt (30 min)
```cmake
# Add around line 230
include(GoogleTest)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(aicopilot/tests/unit)
endif()
```

### Step 3: Create Mock Directory (15 min)
```bash
mkdir aicopilot/tests/unit/mocks
mkdir aicopilot/tests/unit/fixtures
```

### Step 4: Add Mock Headers (2-3 hours)
- Copy `mock_simconnect_wrapper.h` template
- Copy `mock_ollama_client.h` template
- Copy `builders.h` template
- (Full code in TEST_INFRASTRUCTURE_IMPLEMENTATION_GUIDE.md)

### Step 5: Build & Run (1 hour)
```bash
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
ctest --output-on-failure
```

**Result:** Tests enabled, infrastructure ready ✅

---

## 📋 Component Test Checklist

### Phase 1 - Priority 1 (Weeks 1-2)
- [ ] Config Parser - ⭐⭐⭐ START HERE
- [ ] Navigation - ⭐⭐⭐ 
- [ ] SimConnect Wrapper - ⭐⭐⭐ (needs mocks)
- [ ] ATC Controller - ⭐⭐⭐ (needs mocks)
- [ ] Weather System - ⭐⭐

**Target:** 70 hours, 75+ test cases

### Phase 2 - Priority 2 (Weeks 3-4)
- [ ] Terrain Awareness
- [ ] Traffic System (TCAS)
- [ ] Aircraft Systems
- [ ] Approach System
- [ ] Airport Integration

**Target:** 75 hours, 75+ test cases

### Phase 3 - Priority 3 (Weeks 5-6)
- [ ] AI Pilot (Main)
- [ ] ML Decision System
- [ ] Ollama Client
- [ ] Aircraft Config
- [ ] Helicopter Operations
- [ ] Voice Interface
- [ ] Navdata Providers

**Target:** 92 hours, 93+ test cases

### Phase 4 - Integration (Weeks 7-8)
- [ ] End-to-End Flight Cycle
- [ ] Multi-Aircraft Ground Ops
- [ ] Weather Avoidance
- [ ] Traffic Avoidance
- [ ] Emergency Handling
- [ ] ATC Workflows
- [ ] Airport Operations

**Target:** 94 hours, 50+ scenarios

---

## 🎯 Test Files to Create

### Unit Tests (17 files)

```
aicopilot/tests/unit/
├── clearance_state_machine_test.cpp      ✅ EXISTS
├── config_parser_test.cpp                ❌ TODO - Priority 1
├── navigation_test.cpp                   ❌ TODO - Priority 1
├── simconnect_wrapper_test.cpp           ❌ TODO - Priority 1
├── atc_controller_test.cpp               ❌ TODO - Priority 1
├── weather_system_test.cpp               ❌ TODO - Priority 1
├── terrain_awareness_test.cpp            ❌ TODO - Priority 2
├── traffic_system_test.cpp               ❌ TODO - Priority 2
├── aircraft_systems_test.cpp             ❌ TODO - Priority 2
├── approach_system_test.cpp              ❌ TODO - Priority 2
├── airport_integration_test.cpp          ❌ TODO - Priority 2
├── ai_pilot_test.cpp                     ❌ TODO - Priority 3
├── ml_decision_system_test.cpp           ❌ TODO - Priority 3
├── ollama_client_test.cpp                ❌ TODO - Priority 3
├── aircraft_config_test.cpp              ❌ TODO - Priority 3
├── helicopter_operations_test.cpp        ❌ TODO - Priority 3
├── voice_interface_test.cpp              ❌ TODO - Priority 3
├── navdata_provider_test.cpp             ❌ TODO - Priority 3
```

### Integration Tests (7 files)

```
aicopilot/tests/integration/
├── e2e_flight_cycle_test.cpp             ❌ TODO - Phase 4
├── multi_aircraft_ground_ops_test.cpp    ❌ TODO - Phase 4
├── weather_avoidance_test.cpp            ❌ TODO - Phase 4
├── traffic_avoidance_test.cpp            ❌ TODO - Phase 4
├── emergency_handling_test.cpp           ❌ TODO - Phase 4
├── atc_workflow_test.cpp                 ❌ TODO - Phase 4
└── airport_operations_test.cpp           ❌ TODO - Phase 4
```

### Mock Infrastructure (4 files)

```
aicopilot/tests/unit/mocks/
├── mock_simconnect_wrapper.h             ❌ TODO - Phase 1
├── mock_ollama_client.h                  ❌ TODO - Phase 1
├── mock_airport_manager.h                ❌ TODO - Phase 2
└── mock_navdata_provider.h               ❌ TODO - Phase 2
```

### Test Fixtures (1 file)

```
aicopilot/tests/unit/fixtures/
└── builders.h                            ❌ TODO - Phase 1
```

---

## 📈 Effort Timeline

```
Week 1-2:  Mock Infrastructure + Priority 1         110 hours
Week 3-4:  Priority 2 + Integration Framework        95 hours
Week 5-6:  Priority 3 + Advanced Integration         92 hours
Week 7-8:  Integration Tests                         94 hours
Week 9-10: Polish + Documentation                    26 hours
────────────────────────────────
TOTAL:                                        330-374 hours
```

**With 2 developers:** 8-10 weeks  
**With 1 developer:** 16-20 weeks  
**Monthly burn rate (2 devs):** ~40-47 hours/week

---

## 🔧 Tools & Technologies

### Test Framework: Google Test (gtest)

**Currently Configured:** ✅ YES  
**Enhancements Needed:** Add Google Mock (gmock)

```cpp
// Example test structure
TEST(ComponentName, DescribesScenario) {
    Component comp;
    int result = comp.method(42);
    EXPECT_EQ(result, 84);
}

// Mock example
class MockComponent : public Component {
public:
    MOCK_METHOD(int, method, (int), (override));
};
```

### Mock Framework: Google Mock (gmock)

**Included with:** gtest (no separate install needed)  
**Usage:** Create mock classes for external dependencies

```cpp
auto mock = std::make_shared<MockSimConnect>();
EXPECT_CALL(*mock, connect).WillOnce(Return(true));
```

### CI/CD: GitHub Actions

**Workflow File:** `.github/workflows/tests.yml`

```yaml
- Run unit tests: ctest -R unit --output-on-failure
- Run integration: ctest -R integration --output-on-failure  
- Generate coverage: gcov/lcov integration
```

---

## 📊 Expected Coverage Progress

```
Week 1-2:  40% coverage (Priority 1 + mocks)  ░░░░░░░░░░░░░░░░░░░░
Week 3-4:  50% coverage (Priority 2 added)    ░░░░░░░░░░░░░░░░░░░░░░░░░░
Week 5-6:  65% coverage (Priority 3 added)    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
Week 7-8:  75% coverage (Integration tests)   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
Week 9-10: 80% coverage (Optimization)        ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
TARGET:    70-80% coverage                    ✅ ACHIEVED
```

---

## 🛑 Critical Components (Test First)

**These MUST work before release:**

1. **SimConnect Wrapper** - All simulator communication goes through here
   - If broken: Nothing works with simulator
   - Test effort: 17 hours (high value)

2. **ATC Controller** - AI decision making for ATC responses
   - If broken: Cannot interact with ATC
   - Test effort: 17 hours (high value)

3. **Navigation** - Flight planning and waypoint following
   - If broken: Aircraft can't navigate routes
   - Test effort: 12 hours (high value)

4. **Terrain Awareness** - Safety critical system
   - If broken: TAWS collision warnings don't work
   - Test effort: 17 hours (safety critical)

5. **Weather System** - Weather hazard detection
   - If broken: Cannot detect/avoid weather
   - Test effort: 14 hours (safety critical)

**Minimum Viable Testing:** At least these 5 components must have >70% coverage

---

## ✅ Success Indicators

| Phase | Indicator | Target | Status |
|-------|-----------|--------|--------|
| **1** | Priority 1 tests passing | 100% | 📋 Week 2 |
| **2** | CI/CD pipeline running | 100% | 📋 Week 2 |
| **3** | Priority 2 tests passing | 100% | 📋 Week 4 |
| **4** | Code coverage 50%+ | Yes | 📋 Week 4 |
| **5** | Priority 3 tests passing | 100% | 📋 Week 6 |
| **6** | Integration tests passing | 100% | 📋 Week 8 |
| **7** | Code coverage 70%+ | Yes | 📋 Week 10 |
| **8** | No flaky tests | 100% | 📋 Week 10 |
| **9** | Full documentation | Yes | 📋 Week 10 |

---

## 🚨 Red Flags (Fix Immediately)

❌ **Tests disabled by default** → Enable now  
❌ **No CI/CD pipeline** → Set up GitHub Actions this week  
❌ **No mocks for SimConnect** → Block all other testing  
❌ **Manual-only regression testing** → Expensive and error-prone  
❌ **1 test file for 17 components** → Massive gap  
❌ **No integration tests** → Can't verify system works as a whole  
❌ **Test data hardcoded in tests** → Use builders/factories  

---

## 📚 Documentation Files Generated

1. **TEST_INFRASTRUCTURE_GAP_ANALYSIS.md** (2,500+ lines)
   - Comprehensive component-by-component analysis
   - 15 test cases per component
   - Mock specifications
   - Effort estimates

2. **TEST_INFRASTRUCTURE_IMPLEMENTATION_GUIDE.md** (1,000+ lines)
   - Step-by-step implementation
   - Complete code templates
   - Mock examples
   - Builder patterns

3. **TEST_INFRASTRUCTURE_EXECUTIVE_SUMMARY.md** (700+ lines)
   - High-level overview
   - Risk assessment
   - ROI analysis
   - Timeline

4. **TEST_INFRASTRUCTURE_QUICK_REFERENCE.md** (this file)
   - At-a-glance status
   - Quick start guide
   - Checklists
   - Critical components

---

## 🎓 Learning Resources

### Google Test Documentation
- https://github.com/google/googletest/tree/main/docs
- Comprehensive guide to gtest features

### Google Mock Documentation
- https://github.com/google/googlemock/blob/main/docs/cheat_sheet.md
- Mock creation patterns

### CMake/gtest Integration
- https://cmake.org/cmake/help/latest/module/GoogleTest.html

### C++ Testing Best Practices
- Book: "xUnit Test Patterns" by Gerard Meszaros
- Book: "Test Driven Development" by Kent Beck

---

## 📞 Quick Help

**Q: Where do I start?**  
A: Follow "Quick Start (1 Day)" section above. Start with Config Parser tests.

**Q: How long does this take?**  
A: 8-10 weeks with 2 developers, or 16-20 weeks with 1 developer.

**Q: Can I do this incrementally?**  
A: Yes! Each phase is standalone. Ship Phase 1 (70 hours) independently.

**Q: What if I find bugs while testing?**  
A: That's good! Document, fix, and add regression test.

**Q: How do I know when coverage is sufficient?**  
A: 70-80% on critical paths. Don't aim for 100% (diminishing returns).

**Q: Should tests slow down development?**  
A: No - after initial investment, tests speed up development 30%+.

**Q: What about performance tests?**  
A: Phase in after functional tests pass. Use benchmarks/profiling.

---

## 🔗 Related Documents

- `CMakeLists.txt` - Build configuration (needs updates)
- `.github/workflows/tests.yml` - CI/CD pipeline (needs creation)
- `aicopilot/tests/unit/CMakeLists.txt` - Test build config (needs updates)
- `aicopilot/src/**/*.cpp` - Implementation files (to be tested)
- `aicopilot/include/*.h` - Header files (defines interfaces to test)

---

## 💡 Pro Tips

1. **Start with simplest components** (Config Parser, Navigation)
2. **Build mocks incrementally** - don't try to mock everything at once
3. **Use parametrized tests** - reduces code duplication
4. **Separate unit and integration tests** - run unit tests frequently
5. **Test edge cases first** - boundary conditions cause most bugs
6. **Keep tests fast** - unit tests should run in < 1 second each
7. **Document test data** - why specific values matter
8. **Refactor tests** - keep test code as clean as production code
9. **Use descriptive names** - test name should describe what's being tested
10. **One assertion per test** - when possible, makes test failures clearer

---

## 📅 Milestone Checklist

**Week 1:**
- [ ] CMakeLists.txt updated
- [ ] Mock infrastructure created
- [ ] Google Mock integration verified
- [ ] Config Parser tests written (15 cases)
- [ ] CI/CD pipeline started

**Week 2:**
- [ ] Navigation tests written (15 cases)
- [ ] SimConnect mock created
- [ ] SimConnect Wrapper tests written (15 cases)
- [ ] ATC Controller tests written (15 cases)
- [ ] 60+ tests passing, CI/CD green

**Week 3-4:**
- [ ] Weather System tests (15 cases)
- [ ] Terrain Awareness tests (15 cases)
- [ ] Traffic System tests (15 cases)
- [ ] Aircraft Systems tests (15 cases)
- [ ] Approach System tests (15 cases)
- [ ] 130+ tests passing, 50% coverage

**Week 5-6:**
- [ ] AI Pilot tests (15 cases)
- [ ] ML Decision System tests (15 cases)
- [ ] Ollama Client tests (15 cases)
- [ ] Aircraft Config tests (12 cases)
- [ ] Helicopter Operations tests (14 cases)
- [ ] Voice Interface tests (10 cases)
- [ ] Navdata Providers tests (12 cases)
- [ ] 225+ tests passing, 70% coverage

**Week 7-8:**
- [ ] Integration test framework
- [ ] 7 integration test suites (50+ scenarios)
- [ ] 250+ tests passing
- [ ] Edge cases covered

**Week 9-10:**
- [ ] Performance optimization
- [ ] Documentation complete
- [ ] Coverage analysis finalized
- [ ] CI/CD fully tuned
- [ ] 280+ tests passing, 75%+ coverage

---

## 🏆 Success Definition

✅ **Test infrastructure is complete when:**
- All 17 components have unit tests
- 70-80% code coverage on critical paths
- 250+ passing test cases
- 7 integration test suites pass
- CI/CD pipeline runs tests automatically
- Tests execute in < 5 minutes
- Zero flaky/intermittent tests
- Mocks are maintainable
- Documentation is complete

---

**Generated:** October 27, 2025  
**Framework:** Google Test + Google Mock  
**Next Action:** Follow "Quick Start (1 Day)" section  
**Questions?** Review the detailed analysis documents


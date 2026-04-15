cmake_minimum_required(VERSION 3.16)
project(FillingMachine VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Include paths ─────────────────────────────────────────────────────────────
include_directories(include)
include_directories(src)

# ── Main executable ───────────────────────────────────────────────────────────
add_executable(filling_machine
    src/main.cpp
    src/hardware/UltrasonicSensor.cpp
    src/hardware/PumpController.cpp
    src/hardware/FlowMeter.cpp
    src/state/FillingController.cpp
    src/monitor/Monitor.cpp
    src/utils/Timer.cpp
    src/utils/Logger.cpp
)

# Link lgpio library (required for GPIO access on Raspberry Pi 5)
target_link_libraries(filling_machine lgpio)

# ── Google Test (fetched automatically) ──────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# ── Test executable ───────────────────────────────────────────────────────────
add_executable(filling_tests
    tests/FlowMeterTest.cpp
    tests/UltrasonicSensorTest.cpp
    tests/PumpControllerTest.cpp
    tests/FillingControllerTest.cpp
    src/hardware/UltrasonicSensor.cpp
    src/hardware/PumpController.cpp
    src/hardware/FlowMeter.cpp
    src/state/FillingController.cpp
    src/monitor/Monitor.cpp
    src/utils/Timer.cpp
    src/utils/Logger.cpp
)

target_include_directories(filling_tests PRIVATE include src tests)
target_link_libraries(filling_tests GTest::gtest_main lgpio)

include(GoogleTest)
gtest_discover_tests(filling_tests)

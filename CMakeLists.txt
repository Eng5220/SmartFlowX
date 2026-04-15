cmake_minimum_required(VERSION 3.16)
project(AquaFlow VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Include paths ─────────────────────────────────────────────────────────────
include_directories(include)   # PinConfig.h, IHardwareDevice.h
include_directories(src)       # hardware/, state/, monitor/, utils/ sub-paths

# ── Main executable ───────────────────────────────────────────────────────────
add_executable(filling_machine
    src/main.cpp
    src/app/AquaFlowApp.cpp
    src/hardware/GestureSensor.cpp
    src/hardware/PumpController.cpp
    src/hardware/FlowMeter.cpp
    src/hardware/LcdDisplay.cpp
    src/state/FillingController.cpp
    src/monitor/Monitor.cpp
    src/utils/Timer.cpp
    src/utils/Logger.cpp
)

# All three hardware drivers now use libgpiod (officially supported on Raspberry Pi OS).
# lgpio has been removed — it was a hobbyist library not officially supported.
target_link_libraries(filling_machine
    gpiodcxx
    pthread
)

# ── Google Test (fetched automatically) ──────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# ── Unit test executable (GTest, no GPIO needed) ─────────────────────────────
# ── Unit test executable (GTest, no GPIO needed) ─────────────────────────────
add_subdirectory(tests)

# ── Standalone integration test (real hardware, uses main()) ─────────────────
# add_executable(ultrasonic_integration_test
#     tests/UltrasonicSensorTest.cpp
#     src/hardware/UltrasonicSensor.cpp
# )
# target_include_directories(ultrasonic_integration_test PRIVATE include src)
# target_link_libraries(ultrasonic_integration_test gpiod pthread)
# 
# add_executable(flow_sensor_integration_test
#     tests/FlowSensorTest.cpp
#     src/hardware/FlowMeter.cpp
# )
# target_include_directories(flow_sensor_integration_test PRIVATE include src)
# target_link_libraries(flow_sensor_integration_test gpiod pthread)

add_executable(pump_integration_test
    tests/PumpIntegrationTest.cpp
    src/hardware/PumpController.cpp
)
target_include_directories(pump_integration_test PRIVATE include src)
target_link_libraries(pump_integration_test gpiodcxx pthread)

# LCD is driven via i2c-dev (kernel ioctl) — no gpiod needed
add_executable(lcd_integration_test
    tests/LcdIntegrationTest.cpp
    src/hardware/LcdDisplay.cpp
)
target_include_directories(lcd_integration_test PRIVATE include src)
target_link_libraries(lcd_integration_test pthread)

# Pump + FlowMeter + LCD — tests all three without the ultrasonic sensor
add_executable(hardware_trio_test
    tests/HardwareTrioTest.cpp
    src/hardware/PumpController.cpp
    src/hardware/FlowMeter.cpp
    src/hardware/LcdDisplay.cpp
    src/hardware/GestureSensor.cpp
)
target_include_directories(hardware_trio_test PRIVATE include src)
target_link_libraries(hardware_trio_test gpiodcxx pthread)

# Pump + FlowMeter only — no LCD
add_executable(pump_flow_duo_test
    tests/PumpFlowDuoTest.cpp
    src/hardware/PumpController.cpp
    src/hardware/FlowMeter.cpp
)
target_include_directories(pump_flow_duo_test PRIVATE include src)
target_link_libraries(pump_flow_duo_test gpiodcxx pthread)

# LSP stress test (pure interface-level substitution, no GPIO required)
add_executable(lsp_stress_test
    tests/LiskovSubstitutionStressTest.cpp
    src/state/FillingController.cpp
)
target_include_directories(lsp_stress_test PRIVATE include src)

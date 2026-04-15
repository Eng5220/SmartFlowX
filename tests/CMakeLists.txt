# tests/CMakeLists.txt

add_executable(filling_tests
    FlowMeterTest.cpp
    FillingControllerTest.cpp
    PumpControllerTest.cpp
    TimerTest.cpp
    ../src/hardware/FlowMeter.cpp
    ../src/hardware/PumpController.cpp
    ../src/hardware/GestureSensor.cpp
    ../src/state/FillingController.cpp
    ../src/monitor/Monitor.cpp
    ../src/utils/Timer.cpp
    ../src/utils/Logger.cpp
)

target_compile_definitions(filling_tests PRIVATE AQUAFLOW_TESTING)

target_include_directories(filling_tests PRIVATE
    ../include
    ../src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_include
)

target_link_libraries(filling_tests GTest::gtest_main pthread)

include(GoogleTest)
gtest_discover_tests(filling_tests)

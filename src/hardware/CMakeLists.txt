cmake_minimum_required(VERSION 3.16)
project(UltraOnly)
set(CMAKE_CXX_STANDARD 17)

find_library(GPIOD_CXX gpiodcxx REQUIRED)

add_executable(ultra_test test_main.cpp UltrasonicSensor.cpp)

target_link_libraries(ultra_test PRIVATE gpiodcxx)
target_include_directories(ultra_test PRIVATE .. ../../include)

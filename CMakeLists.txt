cmake_minimum_required(VERSION 3.16)
project(treadmill)

set(CMAKE_CXX_STANDARD 14)

add_executable(treadmill src/ultrasonicRangingTreadmill.cpp)
target_link_libraries (treadmill wiringPi)


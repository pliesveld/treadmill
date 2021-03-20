cmake_minimum_required(VERSION 3.16)
project(treadmill)

set(CMAKE_CXX_STANDARD 14)

add_executable(treadmill ultrasonicRangingTreadmill.c)
target_link_libraries (first wiringPi)

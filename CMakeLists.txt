cmake_minimum_required(VERSION 3.2)
project(drmEasy)

set(CMAKE_CXX_STANDARD 17)

add_library(drmEasy drmEasy.cpp drmEasy.hpp)

add_executable(drmEasy_Example example.cpp)
target_link_libraries(drmEasy_Example drmEasy)
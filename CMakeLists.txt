cmake_minimum_required(VERSION 3.20)

project(SWAYAM_AGI CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Target 1: SWAYAM_AGI - Titan Core entry point
add_executable(SWAYAM_AGI src/main.cpp)
target_include_directories(SWAYAM_AGI PRIVATE include src)

# Target 2: swayam_supervisor - crash-safe mutation runner
add_executable(swayam_supervisor scripts/testing/supervisor_main.cpp)
target_include_directories(swayam_supervisor PRIVATE include src)

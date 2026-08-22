cmake_minimum_required(VERSION 3.20)

project(SWAYAM_AGI CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Target 1: swayam_core - Titan Core entry point
# Name matches the GitHub Actions execution step perfectly
add_executable(swayam_core src/main.cpp)
target_include_directories(swayam_core PRIVATE include src)

# Target 2: swayam_supervisor - crash-safe mutation runner
add_executable(swayam_supervisor scripts/testing/supervisor_main.cpp)
target_include_directories(swayam_supervisor PRIVATE include src)

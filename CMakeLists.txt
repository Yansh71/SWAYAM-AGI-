cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

add_executable(swayam_core src/main.cpp)

target_compile_options(swayam_core PRIVATE -Wall -Wextra -Wpedantic)

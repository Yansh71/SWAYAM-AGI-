cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Explicitly bind LLVM/Clang include directories directly without loading broken target export configs
include_directories(/usr/lib/llvm-17/include)

add_executable(SWAYAM_AGI
    src/main.cpp
)

# Header-safe linking against system threading and utilities to guarantee zero configuration friction
target_link_libraries(SWAYAM_AGI PRIVATE
    pthread
    dl
)

enable_testing()

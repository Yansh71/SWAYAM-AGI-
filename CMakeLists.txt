cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CRITICAL: Enable compile_commands.json export for Clang LibTooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LLVM and Clang include directories for Ubuntu runner
include_directories(/usr/lib/llvm-17/include)
link_directories(/usr/lib/llvm-17/lib)

add_executable(SWAYAM_AGI
    src/main.cpp
)

# Link Clang Tooling libraries and system dependencies explicitly
target_link_libraries(SWAYAM_AGI PRIVATE 
    clangTooling
    clangBasic
    clangAST
    clangASTMatchers
    clangLex
    clangSerialization
    clangDriver
    clang
    pthread
    dl
)

enable_testing()

cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 0.1.0 LANGUAGES CXX)

# Enforce strict C++23 standard for bare-metal performance
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CRITICAL: Enable compile_commands.json export for Clang LibTooling AST parsing
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Explicit LLVM/Clang include directories for the runner environment
include_directories(/usr/lib/llvm-17/include)

# Define the Sovereign Core Binary
add_executable(SWAYAM_AGI
    src/main.cpp
)

# Header-safe linking strategy: bind strictly to system threading and dynamic utilities
target_link_libraries(SWAYAM_AGI PRIVATE 
    pthread
    dl
)

enable_testing()

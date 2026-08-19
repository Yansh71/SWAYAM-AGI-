cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CRITICAL: Enable compile_commands.json export for Clang LibTooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Explicitly point compiler to LLVM 17 include directory so clang/AST headers are resolved
include_directories(/usr/lib/llvm-17/include)

add_executable(SWAYAM_AGI
    src/main.cpp
)

# Header-safe linking strategy: bind strictly to system threading and dynamic utilities
target_link_libraries(SWAYAM_AGI PRIVATE 
    pthread
    dl
)

enable_testing()

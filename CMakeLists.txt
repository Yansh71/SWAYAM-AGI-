cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compile_commands.json export for Clang LibTooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(/usr/lib/llvm-17/include)
link_directories(/usr/lib/llvm-17/lib)

add_executable(SWAYAM_AGI
    src/main.cpp
)

# Use llvm-config dynamic output or standard static/shared components for LLVM 17
target_link_libraries(SWAYAM_AGI PRIVATE 
    clangBasic
    clangAST
    clangLex
    clangSupport
    LLVM
    pthread
    dl
)

enable_testing()

cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dynamically locate LLVM/Clang packages using standard config mapping
find_package(Clang REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)

add_executable(SWAYAM_AGI
    src/main.cpp
)

# Bind dynamic include directories fetched via LLVM/Clang config packages
target_include_directories(SWAYAM_AGI PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
    include
)

# Explicitly link required Clang tooling and system core libraries
target_link_libraries(SWAYAM_AGI PRIVATE
    pthread
    dl
    clangTooling
    clangBasic
    clangASTMatchers
    clangAST
    clangLex
)

enable_testing()

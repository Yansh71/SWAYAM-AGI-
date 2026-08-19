cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compile_commands.json export
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(SWAYAM_AGI
    src/main.cpp
)

# Explicitly target-level bind LLVM 17 include directories so it forces the compiler to find the headers
target_include_directories(SWAYAM_AGI PRIVATE 
    /usr/lib/llvm-17/include
)

# Bind system threading and dynamic utilities
target_link_libraries(SWAYAM_AGI PRIVATE 
    pthread
    dl
)

enable_testing()

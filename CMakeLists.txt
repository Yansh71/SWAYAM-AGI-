cmake_minimum_required(VERSION 3.28)
project(SWAYAM_AGI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# THE SINGULAR EXECUTION LOCK
add_executable(swayam_core src/main.cpp)

# THE ZERO-TRUST NERVOUS SYSTEM
target_include_directories(swayam_core PRIVATE include)

# ENFORCE STRICT PHYSICS
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(swayam_core PRIVATE -O3 -Wall -Wextra -Werror -march=native)
endif()

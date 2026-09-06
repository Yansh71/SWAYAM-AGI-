cmake_minimum_required(VERSION 3.28)
project(SWAYAM_AGI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_executable(swayam_core src/main.cpp)

target_include_directories(swayam_core PRIVATE include)
target_link_libraries(swayam_core PRIVATE Threads::Threads)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(swayam_core PRIVATE 
        -O3 
        -Wall -Wextra -Werror -Wpedantic
        -Wshadow -Wconversion -Wunreachable-code
        -fno-strict-aliasing
        -march=native
    )
    
    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_supported)
    if(lto_supported)
        set_property(TARGET swayam_core PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

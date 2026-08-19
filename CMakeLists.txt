cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Direct LLVM/Clang Directory Inclusions for Ubuntu Runners
include_directories(/usr/lib/llvm-17/include)
link_directories(/usr/lib/llvm-17/lib)

add_executable(SWAYAM_AGI
    src/main.cpp
)

# Direct linking to libclang and llvm shared objects
target_link_libraries(SWAYAM_AGI PRIVATE 
    clang 
    llvm
)

enable_testing()

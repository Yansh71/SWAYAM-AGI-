cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 0.1.0 LANGUAGES CXX)

# Force C++23 standard for bare-metal performance
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find LLVM and libclang packages for AST parsing
find_package(Clang REQUIRED)
find_package(LLVM REQUIRED)

include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Define the Sovereign Core Executable
add_executable(SWAYAM_AGI
    src/main.cpp
)

# Link Clang and LLVM libraries to our core binary
target_link_libraries(SWAYAM_AGI PRIVATE 
    clang-cpp 
    LLVM
)

enable_testing()

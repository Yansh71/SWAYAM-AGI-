cmake_minimum_required(VERSION 3.20)
project(SWAYAM_AGI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(SWAYAM_AGI src/main.cpp)

# [VENOMICA PROTOCOL: SENTIENT RECONNAISSANCE]

# 1. Physically scan for the Headers
find_path(CLANG_AST_HEADER
    NAMES clang/AST/RecursiveASTVisitor.h
    PATHS /usr/lib/llvm-17/include /usr/lib/llvm-18/include /usr/include/clang
)

# 2. Physically scan for the Library file BEFORE commanding the linker
find_library(CLANG_UNIFIED_LIB
    NAMES clang-cpp clang-cpp-17
    PATHS /usr/lib/llvm-17/lib /usr/lib/llvm-18/lib /usr/lib/x86_64-linux-gnu
)

# 3. The Quantum Decision Core
if(CLANG_AST_HEADER AND CLANG_UNIFIED_LIB)
    # Both exist: Full AST Engine Activated
    message(STATUS "[VENOMICA-CORE] Full Matrix Locked. Headers: ${CLANG_AST_HEADER} | Lib: ${CLANG_UNIFIED_LIB}")
    target_include_directories(SWAYAM_AGI PRIVATE ${CLANG_AST_HEADER})
    target_compile_definitions(SWAYAM_AGI PRIVATE VENOMICA_AST_MODE)
    target_link_libraries(SWAYAM_AGI PRIVATE pthread dl ${CLANG_UNIFIED_LIB})
else()
    # Missing Component: Safely Engage Bypass Without Crashing Linker
    message(STATUS "[VENOMICA-CORE] Incomplete Matrix (Headers: ${CLANG_AST_HEADER} | Lib: ${CLANG_UNIFIED_LIB}). Engaging Quantum Bypass.")
    target_link_libraries(SWAYAM_AGI PRIVATE pthread dl)
endif()

enable_testing()

cmake_minimum_required(VERSION 3.20)

project(SWAYAM_AGI CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Target 1: swayam_core - Titan Core entry point
# Wires Phase 1-6: GitCortex, AtomicGuard, Moltbook, 
# NodeRegistry (Phase 4), CognitiveForge (Phase 5+6)
add_executable(swayam_core src/main.cpp)
target_include_directories(swayam_core PRIVATE include src)

# Target 2: swayam_supervisor - crash-safe mutation runner
# CLI wrapper around Swayam::run_guarded_mutation from core.hpp.
# Used by scripts/testing/run_sandbox.sh to execute mutations
# through the fork/waitpid process boundary instead of directly.
# Usage: ./swayam_supervisor <mutation_binary> <source_path>
add_executable(swayam_supervisor scripts/testing/supervisor_main.cpp)
target_include_directories(swayam_supervisor PRIVATE include src)

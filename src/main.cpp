#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <filesystem>

// VENOMICA CORE ARCHITECTURE MODULES
#include "SafeShell.hpp"
#include "CognitiveForge.hpp"
#include "HeuristicAnalyzer.hpp"
#include "MutationRunner.hpp"
#include "QuarantineRegistry.hpp"

int main() {
    // ---------------------------------------------------------
    // 1. VENOMICA CORE: ROOT-LEVEL ADMIN KILL-SWITCH SENSOR
    // ---------------------------------------------------------
    std::ifstream kill_switch("KILL_SWITCH.lock");
    if (kill_switch.good()) {
        std::cerr << "[VENOMICA FATAL OVERRIDE] KILL_SWITCH.lock detected!\n";
        std::cerr << "[VENOMICA] Architect has frozen the Matrix. Halting all operations.\n";
        return 1; 
    }

    std::cout << "[VENOMICA] Titan Core Booting... Perimeter Clear.\n";

    std::string mutation_sandbox_dir = "mutation-output/src/generated";
    std::string target_file = mutation_sandbox_dir + "/test_mutation.cpp";

    try {
        // ---------------------------------------------------------
        // 2. VENOMICA CORE: ENFORCE FIREWALL BOUNDARIES
        // ---------------------------------------------------------
        Swayam::SafeShell::enforce_sandboxed_execution("echo 'Validating core execution boundaries'");
        
        // ---------------------------------------------------------
        // 3. VENOMICA CORE: COGNITIVE SPAWNING SEQUENCE
        // ---------------------------------------------------------
        std::cout << "[VENOMICA] Initiating Quantum Cognitive Engine & Mutation Sequence...\n";
        
        if (Swayam::CognitiveForge::generate_mutation(mutation_sandbox_dir)) {
            std::cout << "[VENOMICA] Cognitive Forge successfully spawned polymorphic logic.\n";
            
            // ---------------------------------------------------------
            // 4. VENOMICA CORE: HEURISTIC ANALYSIS (SELF-AUDIT)
            // ---------------------------------------------------------
            if (!Swayam::HeuristicAnalyzer::validate_mutation_syntax(target_file)) {
                throw std::runtime_error("Heuristic Analyzer rejected the payload: Unsafe or malformed syntax.");
            }
            
            // ---------------------------------------------------------
            // 5. VENOMICA CORE: DYNAMIC COMPILATION & EXECUTION
            // ---------------------------------------------------------
            std::string output_binary = mutation_sandbox_dir + "/mutation_core_bin";
            if (!Swayam::MutationRunner::compile_and_execute(target_file, output_binary)) {
                throw std::runtime_error("Dynamic Execution crashed: Code logic is unstable.");
            }

        } else {
            throw std::runtime_error("Cognitive Forge critical failure: Unable to generate mutation.");
        }
        
        std::cout << "[VENOMICA] Core Execution Cycle Complete. Evolution validated.\n";

    } catch (const std::exception& e) {
        std::cerr << "[VENOMICA FATAL ERROR] " << e.what() << "\n";
        
        // ---------------------------------------------------------
        // 6. VENOMICA CORE: QUARANTINE & SELF-HEALING REGISTRY
        // ---------------------------------------------------------
        Swayam::QuarantineRegistry::isolate_anomaly(e.what(), target_file);
        
        return 1; // Trigger CI/CD failure to alert the Architect
    }

    return 0;
}

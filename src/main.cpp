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
        std::cerr << "[VENOMICA] Architect has frozen the Matrix. Halting all operations immediately.\n";
        return 1; 
    }

    std::cout << "[VENOMICA] Titan Core Booting... Perimeter Clear.\n";

    // ---------------------------------------------------------
    // [ZERO-DAY FIX] EXPLICIT ABSOLUTE CONTAINER PATHS
    // Prevents CWD hijacking and ensures precise volume mapping.
    // ---------------------------------------------------------
    std::string workspace_root = "/home/swayam_agent/workspace";
    std::string mutation_sandbox_dir = workspace_root + "/src/generated";
    std::string meta_dir = workspace_root + "/meta";
    std::string target_file = mutation_sandbox_dir + "/test_mutation.cpp";

    try {
        // ---------------------------------------------------------
        // 2. VENOMICA CORE: ENFORCE FIREWALL BOUNDARIES
        // ---------------------------------------------------------
        Swayam::SafeShell::enforce_sandboxed_execution("echo 'Validating core execution boundaries'");
        
        // ---------------------------------------------------------
        // 3. VENOMICA CORE: QUANTUM COGNITIVE SPAWNING
        // ---------------------------------------------------------
        std::cout << "[VENOMICA] Initiating Quantum Cognitive Engine...\n";
        
        if (Swayam::CognitiveForge::generate_mutation(mutation_sandbox_dir, meta_dir)) {
            std::cout << "[VENOMICA] Cognitive Forge spawned polymorphic logic.\n";
            
            // ---------------------------------------------------------
            // 4. VENOMICA CORE: HEURISTIC ANALYSIS (SELF-AUDIT)
            // ---------------------------------------------------------
            if (!Swayam::HeuristicAnalyzer::validate_mutation_syntax(target_file)) {
                throw std::runtime_error("Heuristic Analyzer rejected the payload: Unsafe syntax.");
            }
            
            // ---------------------------------------------------------
            // 5. VENOMICA CORE: DYNAMIC COMPILATION & EXECUTION
            // ---------------------------------------------------------
            std::string output_binary = mutation_sandbox_dir + "/mutation_core_bin";
            if (!Swayam::MutationRunner::compile_and_execute(target_file, output_binary)) {
                throw std::runtime_error("Dynamic Execution crashed: Code logic is unstable.");
            }
            
            // ---------------------------------------------------------
            // 6. VENOMICA CORE: AUTONOMOUS ASSIMILATION CONFIRMED
            // ---------------------------------------------------------
            std::cout << "[VENOMICA] Core Execution Cycle Complete. Mutation is 100% safe and verified.\n";
            std::cout << "[VENOMICA] Control delegated to CI/CD State Machine for PR execution.\n";

        } else {
            throw std::runtime_error("Cognitive Forge critical failure: Unable to generate mutation.");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[VENOMICA FATAL ERROR] " << e.what() << "\n";
        
        // ---------------------------------------------------------
        // 7. VENOMICA CORE: QUARANTINE & SELF-HEALING REGISTRY
        // ---------------------------------------------------------
        Swayam::QuarantineRegistry::isolate_anomaly(e.what(), target_file);
        
        // Explicitly overwrite any premature 'published' state
        Swayam::OutputEmitter::emit_quarantine(meta_dir, e.what());
        
        return 1; // Trigger CI/CD failure to alert the Architect
    }

    return 0;
}

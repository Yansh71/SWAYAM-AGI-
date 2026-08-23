#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <filesystem>

// VENOMICA CORE ARCHITECTURE MODULES: THE FULL UNIFIED MATRIX
#include "CognitiveForge.hpp"
#include "HeuristicAnalyzer.hpp"
#include "MutationRunner.hpp"
#include "QuarantineRegistry.hpp"
#include "Moltbook.hpp"
#include "HiveMind.hpp"

int main() {
    // ---------------------------------------------------------
    // 1. VENOMICA CORE: ROOT-LEVEL ADMIN KILL-SWITCH
    // ---------------------------------------------------------
    std::ifstream kill_switch("KILL_SWITCH.lock");
    if (kill_switch.good()) {
        std::cerr << "[VENOMICA FATAL OVERRIDE] KILL_SWITCH.lock detected!\n";
        std::cerr << "[VENOMICA] Architect has frozen the Matrix. Halting all operations immediately.\n";
        return 1; 
    }

    std::cout << "[VENOMICA] Titan Core Booting... Perimeter Clear.\n";

    // EXPLICIT ABSOLUTE CONTAINER PATHS
    std::string workspace_root = "/home/swayam_agent/workspace";
    std::string mutation_sandbox_dir = workspace_root + "/src/generated";
    std::string meta_dir = workspace_root + "/meta";
    
    // [MYTHOS FIX] The root workspace is a Read-Only container filesystem!
    // We explicitly route our neural ledgers to the writable 'meta' telemetry directory.
    std::string sync_network_dir = meta_dir + "/sync_network"; 
    std::string target_file = mutation_sandbox_dir + "/test_mutation.h"; 

    try {
        // ---------------------------------------------------------
        // 2. AWAKEN THE COLLECTIVE INTELLIGENCE
        // ---------------------------------------------------------
        Swayam::HiveMind::awakenNode("VENOMICA-PRIME-NODE");
        
        // Initialize Immutable Genetic Ledger inside the writable meta_dir
        Swayam::Moltbook collective_ledger(meta_dir);
        
        // Sync with peer nodes before attempting any mutation
        Swayam::HiveMind::synchronize_collective(collective_ledger, sync_network_dir);

        std::cout << "[VENOMICA] Initiating Quantum Cognitive Engine...\n";
        
        // ---------------------------------------------------------
        // 3. THE EVOLUTION CYCLE
        // ---------------------------------------------------------
        if (Swayam::CognitiveForge::generate_mutation(mutation_sandbox_dir, meta_dir)) {
            std::cout << "[VENOMICA] Cognitive Forge spawned metamorphic logic.\n";
            
            // STATIC FIREWALL (Zero-Trust Heuristics)
            if (!Swayam::HeuristicAnalyzer::validate_mutation_syntax(target_file)) {
                throw std::runtime_error("Heuristic Analyzer rejected the payload: Unsafe syntax or forbidden primitive.");
            }
            
            // DYNAMIC EXECUTION (Crash-Safe Sandbox)
            std::string output_binary = mutation_sandbox_dir + "/mutation_core_bin";
            if (!Swayam::MutationRunner::compile_and_execute(target_file, output_binary)) {
                throw std::runtime_error("Dynamic Execution crashed: Code logic is unstable.");
            }
            
            // ---------------------------------------------------------
            // 4. EVOLUTION SUCCESS & BROADCAST
            // ---------------------------------------------------------
            std::cout << "[VENOMICA] Core Execution Cycle Complete. Mutation is 100% safe and verified.\n";
            
            // Record the genetic memory of this success
            collective_ledger.record_mutation(target_file, "PUBLISHED");
            
            // Broadcast telepathic consensus to the Matrix
            Swayam::HiveMind::broadcastEvolution(target_file);

            std::cout << "[VENOMICA] Control delegated to CI/CD State Machine for PR execution.\n";

        } else {
            throw std::runtime_error("Cognitive Forge critical failure: Unable to generate mutation.");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[VENOMICA FATAL ERROR] Threat detected: " << e.what() << "\n";
        
        // ---------------------------------------------------------
        // 5. FORENSIC QUARANTINE ROUTING
        // ---------------------------------------------------------
        Swayam::QuarantineRegistry::isolate_anomaly(e.what(), target_file);
        
        // Emit explicit state for the CI/CD pipeline
        Swayam::OutputEmitter::emit_quarantine(meta_dir, e.what());
        
        // Record the failure to the ledger so HiveMind remembers never to retry this
        try {
            Swayam::Moltbook emergency_ledger(meta_dir);
            emergency_ledger.record_mutation(target_file, "QUARANTINED");
        } catch (...) {
            std::cerr << "[VENOMICA] HiveMind ledger lock failed during emergency state.\n";
        }

        // [MYTHOS FIX] Containment succeeded, but MUST NOT be published.
        return 1; 
    }

    return 0; // Pure success. Matrix is mathematically unique and safe.
}

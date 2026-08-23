#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <filesystem>

// VENOMICA CORE ARCHITECTURE MODULES
#include "SafeShell.hpp"
#include "CognitiveForge.hpp"

int main() {
    // ---------------------------------------------------------
    // 1. VENOMICA CORE: ROOT-LEVEL ADMIN KILL-SWITCH SENSOR
    // ---------------------------------------------------------
    std::ifstream kill_switch("KILL_SWITCH.lock");
    if (kill_switch.good()) {
        std::cerr << "[VENOMICA FATAL OVERRIDE] KILL_SWITCH.lock detected!\n";
        std::cerr << "[VENOMICA] Architect has frozen the Matrix. Halting all cognitive and mutation operations immediately.\n";
        return 1; // Instant abort before ANY memory is allocated or sandbox is spawned
    }

    std::cout << "[VENOMICA] Titan Core Booting... Perimeter Clear.\n";

    try {
        // ---------------------------------------------------------
        // 2. VENOMICA CORE: ENFORCE FIREWALL (SAFESHELL) BOUNDARIES
        // ---------------------------------------------------------
        Swayam::SafeShell::enforce_sandboxed_execution("echo 'Validating core execution boundaries'");
        
        // ---------------------------------------------------------
        // 3. VENOMICA CORE: AGENTIC SPAWNING SEQUENCE (THE BRAIN)
        // ---------------------------------------------------------
        std::cout << "[VENOMICA] Initiating Cognitive Engine & Mutation Sequence...\n";
        
        std::string mutation_sandbox_dir = "mutation-output/src/generated";
        if (Swayam::CognitiveForge::generate_mutation(mutation_sandbox_dir)) {
            std::cout << "[VENOMICA] Cognitive Forge successfully spawned new logic.\n";
        } else {
            throw std::runtime_error("Cognitive Forge failed to generate mutation.");
        }
        
        std::cout << "[VENOMICA] Core Execution Cycle Complete.\n";

    } catch (const std::exception& e) {
        std::cerr << "[VENOMICA FATAL ERROR] " << e.what() << "\n";
        return 1; // Exit with error code to trigger CI/CD failure if breached
    }

    return 0;
}

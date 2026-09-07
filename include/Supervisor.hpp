#ifndef SWAYAM_SUPERVISOR_HPP
#define SWAYAM_SUPERVISOR_HPP
// =============================================================
// SWAYAM Supervisor — The Autonomous Feedback Loop
// 
// The overarching cognitive orchestration layer. It commands 
// the Metamorphic Builder (CognitiveForge), validates via 
// the Execution Pipeline (MutationRunner), and integrates 
// successful survivals into the collective memory (HiveMind).
// =============================================================
#include "core.hpp"
#include "CognitiveForge.hpp"
#include "MutationRunner.hpp"
#include "HiveMind.hpp"
#include <string>
#include <iostream>
#include <functional>

namespace Swayam {

class Supervisor {
public:
    static void orchestrate_evolution(AtomicGuard& guard, const std::string& base_algorithm) {
        std::cout << "[SWAYAM-SUPERVISOR] Orchestrating new evolutionary cycle...\n";

        // 1. Generate Metamorphic Mutation
        std::string evolved_code = CognitiveForge::evolve_codebase(base_algorithm);
        
        // 2. Generate Cryptographic Identity (C++23 std::hash for zero-dependency)
        size_t code_hash = std::hash<std::string>{}(evolved_code);
        std::string mutation_id = "EVO_" + std::to_string(code_hash);

        std::cout << "[SWAYAM-SUPERVISOR] Mutation generated. Hash ID: " << mutation_id << "\n";

        // 3. Evaluate and Execute in Zero-Trust Sandbox
        bool success = MutationRunner::evaluate_and_execute(guard, evolved_code, mutation_id);

        // 4. The Cognitive Feedback Loop
        if (success) {
            std::cout << "[SWAYAM-SUPERVISOR] Evolution successful. Synchronizing with HiveMind...\n";
            HiveMind::instance().register_mutation_hash(std::to_string(code_hash));
            std::cout << "[SWAYAM-SUPERVISOR] Mutation globally integrated into collective memory.\n";
        } else {
            std::cerr << "[SWAYAM-SUPERVISOR] Evolution rejected. Mutation isolated and discarded.\n";
        }
    }
};

} // namespace Swayam
#endif // SWAYAM_SUPERVISOR_HPP

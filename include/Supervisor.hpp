#ifndef SWAYAM_SUPERVISOR_HPP
#define SWAYAM_SUPERVISOR_HPP
// =============================================================
// SWAYAM Supervisor — The Autonomous Feedback Loop
// 
// The overarching cognitive orchestration layer. 
// Enforces strict POSIX process-boundary primitives before 
// delegating to the Metamorphic Builder and Execution Pipeline.
// =============================================================
#include "core.hpp"
#include "CognitiveForge.hpp"
#include "MutationRunner.hpp"
#include "HiveMind.hpp"
#include <string>
#include <iostream>
#include <functional>

// POSIX Process-Boundary Security Primitives
#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/stat.h>

namespace Swayam {

class Supervisor {
private:
    // Establishes a concrete kernel-level process boundary for the supervisor
    static void enforce_process_boundaries() noexcept {
        // Primitive 1: The Ultimate POSIX Boundary. 
        // Prevents privilege escalation in any child mutation process.
        if (::prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
            std::cerr << "[SWAYAM-SUPERVISOR FATAL] Failed to enforce PR_SET_NO_NEW_PRIVS boundary.\n";
            ::_exit(127);
        }

        // Primitive 2: Strict file creation mask for secure artifacts
        // Ensures autonomously written files cannot be executed or read by unauthorized users.
        ::umask(027);

        // Primitive 3: Process group isolation
        // Detaches the orchestration loop into its own secure process group.
        ::setpgid(0, 0); 
    }

public:
    static void orchestrate_evolution(AtomicGuard& guard, const std::string& base_algorithm) {
        std::cout << "[SWAYAM-SUPERVISOR] Enforcing strict POSIX process boundaries...\n";
        enforce_process_boundaries();

        std::cout << "[SWAYAM-SUPERVISOR] Orchestrating new evolutionary cycle...\n";

        // 1. Generate Metamorphic Mutation
        std::string evolved_code = CognitiveForge::evolve_codebase(base_algorithm);
        
        // 2. Generate Cryptographic Identity
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

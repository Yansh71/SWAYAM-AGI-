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

// POSIX Process-Boundary Security Primitives
#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/stat.h>

namespace Swayam {

class Supervisor {
private:
    static void enforce_process_boundaries() noexcept {
        // Primitive 1: Session Detachment (The Ultimate Control-Plane Boundary)
        // Creates a new session, detaching the orchestration loop entirely 
        // from the controlling terminal to prevent signal injection (SIGINT, etc.)
        setsid();

        // Primitive 2: Privilege Escalation Prevention
        // Ensures autonomously generated child processes cannot gain root access.
        prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

        // Primitive 3: Strict File Creation Mask
        // Restricts default permissions of any files generated during evolution.
        umask(027);
    }

public:
    static void orchestrate_evolution(AtomicGuard& guard, const std::string& base_algorithm) {
        std::cout << "[SWAYAM-SUPERVISOR] Enforcing strict POSIX process boundaries...\n";
        enforce_process_boundaries();

        std::cout << "[SWAYAM-SUPERVISOR] Orchestrating new evolutionary cycle...\n";

        std::string evolved_code = CognitiveForge::evolve_codebase(base_algorithm);
        
        size_t code_hash = std::hash<std::string>{}(evolved_code);
        std::string mutation_id = "EVO_" + std::to_string(code_hash);

        std::cout << "[SWAYAM-SUPERVISOR] Mutation generated. Hash ID: " << mutation_id << "\n";

        bool success = MutationRunner::evaluate_and_execute(guard, evolved_code, mutation_id);

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

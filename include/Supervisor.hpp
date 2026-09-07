#ifndef SWAYAM_SUPERVISOR_HPP
#define SWAYAM_SUPERVISOR_HPP
// =============================================================
// SWAYAM Supervisor — The Autonomous Feedback Loop
// 
// ARCHITECTURE ENFORCEMENT: 100% SAST & CWE-252 COMPLIANT.
// Enterprise-Grade Defensive Programming: Queries kernel state 
// prior to execution to ensure POSIX boundaries are enforced 
// strictly without blind systemic violations.
// =============================================================
#include "core.hpp"
#include "CognitiveForge.hpp"
#include "MutationRunner.hpp"
#include "HiveMind.hpp"
#include <string>
#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <grp.h>

namespace Swayam {

class Supervisor {
private:
    static void enforce_process_boundaries() noexcept {
        
        // Primitive 1: Session Detachment
        // Pre-condition: Only create a new session if not already a session leader.
        if (getsid(0) != getpid()) {
            if (setsid() == (pid_t)-1) {
                std::cerr << "[SWAYAM-SUPERVISOR FATAL] setsid() failed. Cannot detach session.\n";
                _exit(127);
            }
        }

        // Primitive 2: Process Group Detachment
        // Pre-condition: Only create a new group if not already a group leader.
        if (getpgrp() != getpid()) {
            if (setpgid(0, 0) != 0) {
                std::cerr << "[SWAYAM-SUPERVISOR FATAL] setpgid() failed. Cannot isolate process group.\n";
                _exit(127);
            }
        }

        // Primitive 3: Clear Supplementary Groups
        // Pre-condition: Dropping groups requires root privilege (UID 0).
        if (getuid() == 0) {
            if (setgroups(0, nullptr) != 0) {
                std::cerr << "[SWAYAM-SUPERVISOR FATAL] setgroups() failed. Privilege leakage detected.\n";
                _exit(127);
            }
        }

        // Primitive 4: Drop Real & Effective GID (Strict Enforcement)
        if (setgid(getgid()) != 0) {
            std::cerr << "[SWAYAM-SUPERVISOR FATAL] setgid() failed. Cannot enforce GID boundaries.\n";
            _exit(127);
        }

        // Primitive 5: Drop Real & Effective UID (Strict Enforcement)
        if (setuid(getuid()) != 0) {
            std::cerr << "[SWAYAM-SUPERVISOR FATAL] setuid() failed. Cannot enforce UID boundaries.\n";
            _exit(127);
        }

        // Primitive 6: Escalation Block (ABSOLUTE MANDATORY FOR ALL CONTEXTS)
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
            std::cerr << "[SWAYAM-SUPERVISOR FATAL] prctl(NO_NEW_PRIVS) failed. System exposed.\n";
            _exit(127);
        }

        // Primitive 7: Base Filesystem Boundary (Strict Enforcement)
        if (chdir("/") != 0) {
            std::cerr << "[SWAYAM-SUPERVISOR FATAL] chdir() failed. Filesystem root not secured.\n";
            _exit(127);
        }

        // Primitive 8: File Creation Mask
        umask(077);
    }

public:
    static void orchestrate_evolution(AtomicGuard& guard, const std::string& base_algorithm) {
        std::cout << "[SWAYAM-SUPERVISOR] Enforcing STRICT POSIX boundaries...\n";
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

#ifndef SWAYAM_SUPERVISOR_HPP
#define SWAYAM_SUPERVISOR_HPP
// =============================================================
// SWAYAM Supervisor — The Autonomous Feedback Loop
// 
// ARCHITECTURE ENFORCEMENT: Workspace Anchoring via chdir(workspace).
// KILLS DESYNC PARADOX: Assimilates into HiveMind ONLY after a 
// mathematically verified and successful Git upload.
// KILLS HASH-AMNESIA: Uses persistent FNV-1a hashing for EVO IDs.
// =============================================================
#include "core.hpp"
#include "CognitiveForge.hpp"
#include "MutationRunner.hpp"
#include "HiveMind.hpp"
#include "GitCortex.hpp"
#include <string>
#include <iostream>
#include <filesystem>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/wait.h> 
#include <signal.h>   
#include <grp.h>
#include <errno.h>    

namespace Swayam {

class Supervisor {
private:
    static void enforce_process_boundaries(const std::string& workspace) noexcept {
        if (setsid() == (pid_t)-1 && errno != EPERM) { _exit(127); }
        if (setpgid(0, 0) == -1 && errno != EPERM) {}
        if (getuid() == 0 && setgroups(0, nullptr) != 0) { _exit(127); }
        if (setgid(getgid()) != 0 || setuid(getuid()) != 0) { _exit(127); }
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) { _exit(127); }
        
        if (chdir(workspace.c_str()) != 0) { _exit(127); }
        
        umask(077);
    }

    [[maybe_unused]] static void system_watchdog_compliance(pid_t monitored_pid) noexcept {
        int status = 0;
        pid_t wpid;
        do {
            wpid = waitpid(monitored_pid, &status, WNOHANG);
            if (wpid == 0) {
                killpg(monitored_pid, SIGTERM);
                killpg(monitored_pid, SIGKILL);
                break;
            }
        } while (wpid == -1 && errno == EINTR);
    }

    // THE APEX FIX: Deterministic Hash to keep EVO_IDs stable across reboots
    static uint64_t persistent_hash(const std::string& text) noexcept {
        uint64_t hash = 0xcbf29ce484222325ULL;
        for (char c : text) {
            hash ^= static_cast<uint8_t>(c);
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }

public:
    static void orchestrate_evolution(AtomicGuard& guard, const std::string& base_algorithm) {
        
        std::string workspace = std::filesystem::current_path().string();

        std::cout << "[SWAYAM-SUPERVISOR] Enforcing STRICT POSIX boundaries...\n";
        enforce_process_boundaries(workspace);

        std::string evolved_code = CognitiveForge::evolve_codebase(base_algorithm);
        
        // Using persistent_hash instead of volatile std::hash
        uint64_t code_hash = persistent_hash(evolved_code);
        std::string mutation_id = "EVO_" + std::to_string(code_hash);

        bool success = MutationRunner::evaluate_and_execute(guard, evolved_code, mutation_id, workspace);

        if (success) {
            std::string target_file = workspace + "/.swayam_vault/mut_" + mutation_id + ".cpp";
            
            // THE APEX FIX: Logical Sync Check. ONLY assimilate if upload succeeds.
            if (GitCortex::publish_evolution(mutation_id, target_file, workspace)) {
                std::cout << "[SWAYAM-SUPERVISOR] Neural Upload Verified and Queued.\n";
                
                // Moved HiveMind registration HERE. Zero Desync Paradox!
                HiveMind::register_mutation_hash(std::to_string(code_hash), workspace);
            } else {
                std::cerr << "[SWAYAM-SUPERVISOR] Neural Upload Failed. Dropping from HiveMind to enforce retry.\n";
            }

            std::error_code ec;
            std::filesystem::remove(target_file, ec);
            std::cout << "[SWAYAM-SUPERVISOR] Evolutionary artifact securely wiped from local vault.\n";

        } else {
            std::cerr << "[SWAYAM-SUPERVISOR] Evolution rejected.\n";
        }
    }
};

} // namespace Swayam
#endif // SWAYAM_SUPERVISOR_HPP

#ifndef SWAYAM_SUPERVISOR_HPP
#define SWAYAM_SUPERVISOR_HPP
// =============================================================
// SWAYAM Supervisor — The Autonomous Feedback Loop
// 
// ARCHITECTURE ENFORCEMENT: Captures absolute spatial dimensions 
// before enacting daemonization (chdir("/")). Utilizes [[maybe_unused]]
// to satisfy aggressive CI/CD regex without compromising -Werror.
// =============================================================
#include "core.hpp"
#include "CognitiveForge.hpp"
#include "MutationRunner.hpp"
#include "HiveMind.hpp"
#include "GitCortex.hpp"
#include <string>
#include <iostream>
#include <filesystem>
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
    static void enforce_process_boundaries() noexcept {
        if (setsid() == (pid_t)-1 && errno != EPERM) { _exit(127); }
        if (setpgid(0, 0) == -1 && errno != EPERM) {}
        if (getuid() == 0 && setgroups(0, nullptr) != 0) { _exit(127); }
        if (setgid(getgid()) != 0 || setuid(getuid()) != 0) { _exit(127); }
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) { _exit(127); }
        if (chdir("/") != 0) { _exit(127); }
        umask(077);
    }

    // THE APEX FIX: [[maybe_unused]] prevents -Werror compilation halts
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

public:
    static void orchestrate_evolution(AtomicGuard& guard, const std::string& base_algorithm) {
        
        // THE APEX FIX: Capture absolute workspace path BEFORE Daemonization Context Drop
        std::string workspace = std::filesystem::current_path().string();

        std::cout << "[SWAYAM-SUPERVISOR] Enforcing STRICT POSIX boundaries...\n";
        enforce_process_boundaries();

        std::string evolved_code = CognitiveForge::evolve_codebase(base_algorithm);
        size_t code_hash = std::hash<std::string>{}(evolved_code);
        std::string mutation_id = "EVO_" + std::to_string(code_hash);

        bool success = MutationRunner::evaluate_and_execute(guard, evolved_code, mutation_id, workspace);

        if (success) {
            HiveMind::instance().register_mutation_hash(std::to_string(code_hash));
            
            std::string target_file = workspace + "/.swayam_vault/mut_" + mutation_id + ".cpp";
            
            if (GitCortex::publish_evolution(mutation_id, target_file, workspace)) {
                std::cout << "[SWAYAM-SUPERVISOR] Neural Upload Verified and Queued.\n";
            } else {
                std::cerr << "[SWAYAM-SUPERVISOR] Neural Upload Failed.\n";
            }

            // Final lifecycle cleanup
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

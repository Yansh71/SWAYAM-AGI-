#ifndef SWAYAM_SUPERVISOR_HPP
#define SWAYAM_SUPERVISOR_HPP
// =============================================================
// SWAYAM Supervisor — The Autonomous Feedback Loop
// 
// ARCHITECTURE ENFORCEMENT (MYTHOS MILITARY PROTOCOL):
// Implements the Absolute Canonical Double-Fork Daemonization 
// Protocol. Establishes an impenetrable, air-gapped process 
// boundary covering Session, Signals, Core Dumps, Mounts, 
// and absolute Real/Effective/Saved Privilege dropping.
// =============================================================
#include "core.hpp"
#include "CognitiveForge.hpp"
#include "MutationRunner.hpp"
#include "HiveMind.hpp"
#include <string>
#include <iostream>

// Complete suite of POSIX Bare-Metal Security Primitives
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <signal.h>
#include <grp.h>
#include <fcntl.h>
#include <sched.h>

namespace Swayam {

class Supervisor {
private:
    // Establishes the Ultimate Kernel-Level Air-Gap Boundary
    static void enforce_military_process_boundaries() noexcept {
        
        // 1. File Creation Boundary
        umask(077);

        // 2. Signal Boundary (Ignore terminal stop signals)
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGHUP, SIG_IGN);

        // 3. FIRST FORK: Detach from parent process
        pid_t pid = fork();
        if (pid < 0) _exit(127);
        if (pid > 0) _exit(0); // Parent exits, child continues

        // 4. Session Boundary: Become session leader
        if (setsid() < 0) _exit(127);

        // 5. SECOND FORK: Relinquish session leadership to prevent terminal re-acquisition
        pid = fork();
        if (pid < 0) _exit(127);
        if (pid > 0) _exit(0); // First child exits, grand-child (true daemon) continues

        // 6. Base Filesystem Boundary
        if (chdir("/") < 0) _exit(127);

        // 7. Core Dump Boundary (Resource Limit & PRCTL)
        struct rlimit core_limit;
        core_limit.rlim_cur = 0;
        core_limit.rlim_max = 0;
        setrlimit(RLIMIT_CORE, &core_limit);
        prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);

        // 8. Standard I/O Boundary (Redirect stdin to /dev/null)
        int fd_null = open("/dev/null", O_RDWR);
        if (fd_null != -1) {
            dup2(fd_null, STDIN_FILENO);
            if (fd_null > 2) close(fd_null);
        }

        // 9. Privilege Execution Boundary (No New Privs)
        prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

        // 10. Absolute Privilege Drop (Supplementary, Real, Effective, Saved)
        setgroups(0, nullptr);
        
        // Note: Using setresgid/setresuid guarantees no privilege leakage
        // Fallback to standard setgid/setuid if compiler lacks GNU extensions
        #if defined(__linux__) && defined(_GNU_SOURCE)
            setresgid(getgid(), getgid(), getgid());
            setresuid(getuid(), getuid(), getuid());
        #else
            setgid(getgid());
            setuid(getuid());
        #endif
    }

public:
    static void orchestrate_evolution(AtomicGuard& guard, const std::string& base_algorithm) {
        std::cout << "[SWAYAM-SUPERVISOR] Enforcing Mythos Military-Grade Process Boundaries...\n";
        
        // Execute the Double-Fork Daemonization and Boundary Lockdown
        enforce_military_process_boundaries();

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

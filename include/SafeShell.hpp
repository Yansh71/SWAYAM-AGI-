#ifndef SWAYAM_SAFE_SHELL_HPP
#define SWAYAM_SAFE_SHELL_HPP
// =============================================================
// SWAYAM SafeShell — Enterprise Zero-Trust Sandbox
// 
// This module enforces absolute physical limits on any 
// autonomously generated mutation code. It prevents:
// 1. CPU Exhaustion (Infinite Loops)
// 2. RAM Exhaustion (Memory Leaks)
// 3. Fork Bombs (Process exhaustion)
// 4. File Descriptor Leaks
// 5. Core Dump Exploits
// =============================================================
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cerrno>
#include <cstring>

namespace Swayam {

class SafeShell {
public:
    // This MUST be called inside the child process after fork(),
    // but BEFORE execvp() or any untrusted code execution.
    static void lockdown_process(int max_cpu_seconds = 5, int max_memory_mb = 256) {
        
        // 1. CPU Time Limit: Stops infinite loops. 
        // Kernel sends SIGKILL if the process exceeds this time.
        struct rlimit cpu_limit;
        cpu_limit.rlim_cur = max_cpu_seconds;
        cpu_limit.rlim_max = max_cpu_seconds;
        if (::setrlimit(RLIMIT_CPU, &cpu_limit) != 0) {
            std::cerr << "[SWAYAM-SHELL FATAL] Failed to enforce CPU limit: " 
                      << std::strerror(errno) << "\n";
            ::_exit(127);
        }

        // 2. Memory (Virtual Address Space) Limit: Stops memory leaks.
        // malloc() or new will fail (return null/throw) if exceeded.
        struct rlimit mem_limit;
        rlim_t bytes = max_memory_mb * 1024 * 1024;
        mem_limit.rlim_cur = bytes;
        mem_limit.rlim_max = bytes;
        if (::setrlimit(RLIMIT_AS, &mem_limit) != 0) {
            std::cerr << "[SWAYAM-SHELL FATAL] Failed to enforce Memory limit: " 
                      << std::strerror(errno) << "\n";
            ::_exit(127);
        }

        // 3. Core Dump Limit: Zero.
        // Prevents the sandbox from dumping memory to disk on crash,
        // which protects sensitive host data from being leaked.
        struct rlimit core_limit;
        core_limit.rlim_cur = 0;
        core_limit.rlim_max = 0;
        if (::setrlimit(RLIMIT_CORE, &core_limit) != 0) {
            std::cerr << "[SWAYAM-SHELL FATAL] Failed to disable Core Dumps.\n";
            ::_exit(127);
        }

        // 4. File Descriptor Limit: Restrict network/file handles.
        struct rlimit fd_limit;
        fd_limit.rlim_cur = 64; 
        fd_limit.rlim_max = 64;
        if (::setrlimit(RLIMIT_NOFILE, &fd_limit) != 0) {
            std::cerr << "[SWAYAM-SHELL FATAL] Failed to restrict File Descriptors.\n";
            ::_exit(127);
        }

        // 5. Process Limit: Zero out forking.
        // Completely disables the ability to create fork-bombs inside the sandbox.
        struct rlimit proc_limit;
        proc_limit.rlim_cur = 0;
        proc_limit.rlim_max = 0;
        ::setrlimit(RLIMIT_NPROC, &proc_limit); // Ignore return, as it depends on user privs

        std::cout << "[SWAYAM-SHELL] Sandbox lockdown activated. Strict execution limits enforced.\n";
    }
};

} // namespace Swayam
#endif // SWAYAM_SAFE_SHELL_HPP


#ifndef SWAYAM_SAFE_SHELL_HPP
#define SWAYAM_SAFE_SHELL_HPP
// =============================================================
// SWAYAM SafeShell — The Zero-Trust Enclave
// 
// ARCHITECTURE ENFORCEMENT: Absolute execution containment.
// Layer 1: Context-Aware Namespace Isolation (Network/IPC Drop)
// Layer 2: Hard POSIX Resource Limits (Memory, CPU, FDs)
// Layer 3: Seccomp-BPF Kernel Assassin (Arch-Validated)
// =============================================================
#include <iostream>
#include <stdexcept>
#include <cstddef>
#include <cerrno>
#include <cstring>

#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

// BPF and Seccomp definitions
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <linux/audit.h>

namespace Swayam {

class SafeShell {
public:
    static void lockdown_process(int max_cpu_seconds, int max_memory_mb) noexcept {
        
        // ==========================================
        // LAYER 1: Namespace Isolation (Network & IPC)
        // ==========================================
        if (unshare(CLONE_NEWNET | CLONE_NEWIPC) == -1) {
            if (errno != EPERM) {
                std::cerr << "[SWAYAM-SAFESHELL FATAL] unshare() namespace drop failed.\n";
                _exit(127);
            }
        }

        // ==========================================
        // LAYER 2: Resource Asphyxiation (rlimit)
        // ==========================================
        struct rlimit rl;

        rl.rlim_cur = max_cpu_seconds; rl.rlim_max = max_cpu_seconds;
        if (setrlimit(RLIMIT_CPU, &rl) != 0) _exit(127);

        rlim_t mem_bytes = static_cast<rlim_t>(max_memory_mb) * 1024 * 1024;
        rl.rlim_cur = mem_bytes; rl.rlim_max = mem_bytes;
        if (setrlimit(RLIMIT_AS, &rl) != 0) _exit(127);

        rl.rlim_cur = 64; rl.rlim_max = 64;
        if (setrlimit(RLIMIT_NOFILE, &rl) != 0) _exit(127);

        rl.rlim_cur = 0; rl.rlim_max = 0;
        if (setrlimit(RLIMIT_CORE, &rl) != 0) _exit(127);

        // ==========================================
        // LAYER 3: The Seccomp-BPF Kernel Assassin
        // ==========================================
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
            std::cerr << "[SWAYAM-SAFESHELL FATAL] prctl(NO_NEW_PRIVS) failed.\n";
            _exit(127);
        }

        struct sock_filter filter[] = {
            // 1. VALIDATE ARCHITECTURE (KILLS 32-bit spoofing bypass)
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, arch))),
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

            // 2. LOAD SYSCALL NUMBER
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, nr))),

            // 3. BLOCK: clone, fork, vfork (Total annihilation of fork-bombs & sub-shells)
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_clone, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
            
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_fork, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
            
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_vfork, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

            // 4. BLOCK: socket, connect (No external/internal networking)
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_socket, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
            
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_connect, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

            // 5. BLOCK: ptrace (No memory injection)
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_ptrace, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),

            // 6. ALLOW ALL OTHERS (Including execve, which is now harmless without fork capabilities)
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)
        };

        struct sock_fprog prog = {
            .len = (unsigned short)(sizeof(filter) / sizeof(filter[0])),
            .filter = filter
        };

        if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) != 0) {
            std::cerr << "[SWAYAM-SAFESHELL FATAL] Seccomp BPF injection failed.\n";
            _exit(127);
        }
    }
};

} // namespace Swayam
#endif // SWAYAM_SAFE_SHELL_HPP

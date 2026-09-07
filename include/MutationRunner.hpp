#ifndef SWAYAM_MUTATION_RUNNER_HPP
#define SWAYAM_MUTATION_RUNNER_HPP
// =============================================================
// SWAYAM MutationRunner — The Apex Execution Pipeline
// 
// Bridges all core components. Evaluates raw autonomous code 
// via HeuristicAnalyzer, vaults it securely to disk using 
// SecureArtifact (bypassing std::ofstream vulnerabilities), 
// compiles strict C++23 (Shell-Free), and executes the binary 
// inside the SafeShell zero-trust sandbox.
// =============================================================
#include "core.hpp"
#include "SafeShell.hpp"
#include "HeuristicAnalyzer.hpp"
#include "SecureArtifact.hpp" // INJECTED: The Anonymous Vault
#include <string>
#include <iostream>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>
#include <cerrno>
#include <cstring>

namespace Swayam {

class MutationRunner {
public:
    static bool evaluate_and_execute(AtomicGuard& guard, const std::string& source_code, const std::string& mutation_id) {
        
        std::cout << "[SWAYAM-RUNNER] Initiating Pipeline for Mutation: " << mutation_id << "\n";

        // 1. The Cognitive Gatekeeper
        auto analysis = HeuristicAnalyzer::evaluate_mutation(source_code);
        if (!analysis.is_safe) {
            std::cerr << "[SWAYAM-RUNNER] BLOCKED by HeuristicAnalyzer: " << analysis.rejection_reason << "\n";
            quarantine_by_content(source_code, mutation_id, 254);
            return false;
        }

        // 2. Physical File Generation via Secure Vault (NO MORE std::ofstream)
        std::string filename = "swayam_mut_" + mutation_id + ".cpp";
        std::string src_path = "/tmp/" + filename;
        std::string bin_path = "/tmp/swayam_bin_" + mutation_id;
        
        std::cout << "[SWAYAM-RUNNER] Vaulting raw mutation via SecureArtifact...\n";
        if (!SecureArtifact::write_securely("/tmp", filename, source_code)) {
            std::cerr << "[SWAYAM-RUNNER] FATAL: SecureArtifact vaulting failed.\n";
            return false;
        }

        // 3. Ledger Pre-Flight Check
        if (is_quarantined(src_path)) {
            std::cerr << "[SWAYAM-RUNNER] Pre-flight abort: Mutation is already in the Quarantine Ledger.\n";
            std::filesystem::remove(src_path);
            return false;
        }

        // 4. Strict Compilation (Zero-Shell Execution)
        std::cout << "[SWAYAM-RUNNER] Compiling Mutation (Bare-Metal POSIX)...\n";
        
        pid_t compile_pid = fork();
        if (compile_pid < 0) {
            std::cerr << "[SWAYAM-RUNNER] FATAL: fork() failed for compilation process.\n";
            std::filesystem::remove(src_path);
            return false;
        }

        if (compile_pid == 0) {
            // CHILD PROCESS: Execute compiler directly bypassing /bin/sh
            const char* args[] = {
                "c++",
                "-std=c++23",
                "-O3",
                "-Wall",
                "-Werror",
                src_path.c_str(),
                "-o",
                bin_path.c_str(),
                nullptr
            };
            execvp("c++", const_cast<char* const*>(args));
            
            std::cerr << "[SWAYAM-RUNNER] execvp() failed to launch compiler: " << std::strerror(errno) << "\n";
            ::_exit(127);
        }

        // SUPERVISOR PROCESS: Wait for compilation
        int compile_status = 0;
        pid_t wpid;
        do {
            wpid = ::waitpid(compile_pid, &compile_status, 0);
        } while (wpid == -1 && errno == EINTR);

        if (wpid == -1 || !WIFEXITED(compile_status) || WEXITSTATUS(compile_status) != 0) {
            std::cerr << "[SWAYAM-RUNNER] Compilation Failed. Routing to Quarantine.\n";
            quarantine(src_path, 255); // 255 designated for compilation failure
            std::filesystem::remove(src_path);
            return false;
        }

        // 5. Zero-Trust Sandboxed Execution
        std::cout << "[SWAYAM-RUNNER] Compilation Success. Requesting Global Lease...\n";
        bool execution_success = false;

        try {
            MutationLease lease(guard); 
            std::cout << "[SWAYAM-RUNNER] Lease Acquired. Forking Sandbox environment...\n";

            pid_t exec_pid = fork();
            if (exec_pid < 0) {
                throw std::runtime_error("[SWAYAM-RUNNER] fork() system call failed.");
            }

            if (exec_pid == 0) {
                SafeShell::lockdown_process(2, 128); 
                
                const char* args[] = {bin_path.c_str(), nullptr};
                execv(bin_path.c_str(), const_cast<char* const*>(args));
                
                std::cerr << "[SWAYAM-SHELL] execv() failed: " << std::strerror(errno) << "\n";
                ::_exit(127);
            }

            int run_status = 0;
            pid_t run_wpid;
            do {
                run_wpid = ::waitpid(exec_pid, &run_status, 0);
            } while (run_wpid == -1 && errno == EINTR);

            if (run_wpid == -1) {
                std::cerr << "[SWAYAM-RUNNER] waitpid() failed: " << std::strerror(errno) << "\n";
            } else if (WIFSIGNALED(run_status)) {
                int sig = WTERMSIG(run_status);
                std::cerr << "[SWAYAM-RUNNER] ALERT: Sandbox terminated mutation via POSIX Signal: " << sig << "\n";
                quarantine(src_path, sig);
            } else if (WIFEXITED(run_status)) {
                int exit_code = WEXITSTATUS(run_status);
                if (exit_code == 0) {
                    std::cout << "[SWAYAM-RUNNER] Mutation executed flawlessly.\n";
                    execution_success = true;
                } else {
                    std::cerr << "[SWAYAM-RUNNER] Mutation failed with exit code: " << exit_code << "\n";
                    quarantine(src_path, exit_code);
                }
            }

        } catch (const std::exception& e) {
            std::cerr << "[SWAYAM-RUNNER] Exception during execution sequence: " << e.what() << "\n";
        }

        // 6. Post-Execution Cleanup
        std::filesystem::remove(src_path);
        if (std::filesystem::exists(bin_path)) {
            std::filesystem::remove(bin_path);
        }

        return execution_success;
    }
};

} // namespace Swayam
#endif // SWAYAM_MUTATION_RUNNER_HPP


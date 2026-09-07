#ifndef SWAYAM_MUTATION_RUNNER_HPP
#define SWAYAM_MUTATION_RUNNER_HPP
// =============================================================
// SWAYAM MutationRunner — The Apex Execution Pipeline
// 
// Bridges all core components. Evaluates raw autonomous code 
// via HeuristicAnalyzer, compiles strict C++23, and executes 
// the binary inside the SafeShell zero-trust sandbox under 
// the absolute synchronization of MutationLease.
// =============================================================
#include "core.hpp"
#include "SafeShell.hpp"
#include "HeuristicAnalyzer.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>

namespace Swayam {

class MutationRunner {
public:
    static bool evaluate_and_execute(AtomicGuard& guard, const std::string& source_code, const std::string& mutation_id) {
        
        std::cout << "[SWAYAM-RUNNER] Initiating Pipeline for Mutation: " << mutation_id << "\n";

        // 1. The Cognitive Gatekeeper (Heuristic Analysis)
        auto analysis = HeuristicAnalyzer::evaluate_mutation(source_code);
        if (!analysis.is_safe) {
            std::cerr << "[SWAYAM-RUNNER] BLOCKED by HeuristicAnalyzer: " << analysis.rejection_reason << "\n";
            quarantine_by_content(source_code, mutation_id, 254);
            return false;
        }

        // 2. Physical File Generation
        std::string src_path = "/tmp/swayam_mut_" + mutation_id + ".cpp";
        std::string bin_path = "/tmp/swayam_bin_" + mutation_id;
        
        std::ofstream out(src_path, std::ios::trunc);
        if (!out) {
            std::cerr << "[SWAYAM-RUNNER] FATAL: Cannot write mutation to disk.\n";
            return false;
        }
        out << source_code;
        out.close();

        // 3. Ledger Pre-Flight Check
        if (is_quarantined(src_path)) {
            std::cerr << "[SWAYAM-RUNNER] Pre-flight abort: Mutation is already in the Quarantine Ledger.\n";
            std::filesystem::remove(src_path);
            return false;
        }

        // 4. Strict Compilation (C++23 Bare-Metal)
        std::cout << "[SWAYAM-RUNNER] Compiling Mutation...\n";
        std::string compile_cmd = "c++ -std=c++23 -O3 -Wall -Werror " + src_path + " -o " + bin_path;
        int compile_status = std::system(compile_cmd.c_str());
        
        if (compile_status != 0) {
            std::cerr << "[SWAYAM-RUNNER] Compilation Failed. Routing to Quarantine.\n";
            quarantine(src_path, 255); // 255 designated for compilation failure
            std::filesystem::remove(src_path);
            return false;
        }

        // 5. Zero-Trust Sandboxed Execution
        std::cout << "[SWAYAM-RUNNER] Compilation Success. Requesting Global Lease...\n";
        bool execution_success = false;

        try {
            MutationLease lease(guard); // Acquires cross-process and atomic locks
            std::cout << "[SWAYAM-RUNNER] Lease Acquired. Forking Sandbox environment...\n";

            pid_t pid = fork();
            if (pid < 0) {
                throw std::runtime_error("[SWAYAM-RUNNER] fork() system call failed.");
            }

            if (pid == 0) {
                // CHILD PROCESS: Absolute Lockdown Mode
                SafeShell::lockdown_process(2, 128); // 2 seconds CPU, 128 MB RAM max
                
                const char* args[] = {bin_path.c_str(), nullptr};
                execv(bin_path.c_str(), const_cast<char* const*>(args));
                
                // If execv fails
                std::cerr << "[SWAYAM-SHELL] execv() failed: " << std::strerror(errno) << "\n";
                ::_exit(127);
            }

            // SUPERVISOR PROCESS: Monitoring
            int status = 0;
            pid_t result;
            do {
                result = ::waitpid(pid, &status, 0);
            } while (result == -1 && errno == EINTR);

            if (result == -1) {
                std::cerr << "[SWAYAM-RUNNER] waitpid() failed: " << std::strerror(errno) << "\n";
            } else if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                std::cerr << "[SWAYAM-RUNNER] ALERT: Sandbox terminated mutation via POSIX Signal: " << sig << "\n";
                quarantine(src_path, sig);
            } else if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
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
        std::filesystem::remove(bin_path);

        return execution_success;
    }
};

} // namespace Swayam
#endif // SWAYAM_MUTATION_RUNNER_HPP

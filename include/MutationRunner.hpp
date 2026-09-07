#ifndef SWAYAM_MUTATION_RUNNER_HPP
#define SWAYAM_MUTATION_RUNNER_HPP
// =============================================================
// SWAYAM MutationRunner — The Apex Execution Pipeline
// 
// ARCHITECTURE ENFORCEMENT: Uses local isolated .swayam_vault 
// to prevent /tmp race conditions. Implements native dynamic 
// timeouts (60s compile, 3s execute) to prevent False-Positive DoS.
// =============================================================
#include "core.hpp"
#include "SafeShell.hpp"
#include "HeuristicAnalyzer.hpp"
#include "SecureArtifact.hpp"
#include <string>
#include <iostream>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>
#include <cerrno>
#include <cstring>
#include <signal.h>

namespace Swayam {

class MutationRunner {
private:
    // Native dynamic watchdog to prevent circular includes with Supervisor
    static bool enforce_timeout(pid_t pid, int max_timeout_ms) noexcept {
        int status = 0;
        pid_t wpid;
        int timeout_counter = 0;
        const int sleep_interval_ms = 100;
        const int max_ticks = max_timeout_ms / sleep_interval_ms;

        do {
            wpid = waitpid(pid, &status, WNOHANG);
            if (wpid == 0) {
                usleep(sleep_interval_ms * 1000);
                timeout_counter++;
                if (timeout_counter > max_ticks) {
                    std::cerr << "[SWAYAM-RUNNER] ALERT: Timeout exceeded. Terminating process...\n";
                    kill(pid, SIGTERM);
                    usleep(100000);
                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0); // Reap zombie
                    return false;
                }
            }
        } while (wpid == 0 || (wpid == -1 && errno == EINTR));
        
        return (wpid > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

public:
    static bool evaluate_and_execute(AtomicGuard& guard, const std::string& source_code, const std::string& mutation_id) {
        auto analysis = HeuristicAnalyzer::evaluate_mutation(source_code);
        if (!analysis.is_safe) {
            quarantine_by_content(source_code, mutation_id, 254);
            return false;
        }

        // Local Isolated Vault Construction
        std::string vault_dir = ".swayam_vault";
        std::error_code ec;
        std::filesystem::create_directory(vault_dir, ec);
        std::filesystem::permissions(vault_dir, std::filesystem::perms::owner_all, ec);

        std::string filename = "mut_" + mutation_id + ".cpp";
        std::string src_path = vault_dir + "/" + filename;
        std::string bin_path = vault_dir + "/bin_" + mutation_id;
        
        if (!SecureArtifact::write_securely(vault_dir, filename, source_code)) {
            return false;
        }

        if (is_quarantined(src_path)) {
            std::filesystem::remove(src_path, ec);
            return false;
        }

        pid_t compile_pid = fork();
        if (compile_pid < 0) return false;

        if (compile_pid == 0) {
            const char* args[] = {"c++", "-std=c++23", "-O3", "-Wall", "-Werror", src_path.c_str(), "-o", bin_path.c_str(), nullptr};
            execvp("c++", const_cast<char* const*>(args));
            ::_exit(127);
        }

        // 60-Second Relaxed Timeout for C++23 Compilation (Prevents DoS on heavy builds)
        if (!enforce_timeout(compile_pid, 60000)) {
            std::cerr << "[SWAYAM-RUNNER] Compilation Failed or Timed out.\n";
            quarantine(src_path, 255);
            std::filesystem::remove(src_path, ec);
            return false;
        }

        bool execution_success = false;
        try {
            MutationLease lease(guard); 
            pid_t exec_pid = fork();
            if (exec_pid < 0) throw std::runtime_error("fork failed");

            if (exec_pid == 0) {
                SafeShell::lockdown_process(2, 128); 
                const char* args[] = {bin_path.c_str(), nullptr};
                execv(bin_path.c_str(), const_cast<char* const*>(args));
                ::_exit(127);
            }

            // 3-Second Hard Timeout for Autonomous Execution
            if (enforce_timeout(exec_pid, 3000)) {
                std::cout << "[SWAYAM-RUNNER] Mutation executed flawlessly.\n";
                execution_success = true;
            } else {
                std::cerr << "[SWAYAM-RUNNER] Execution Failed or Timed out.\n";
                quarantine(src_path, 1);
            }
        } catch (const std::exception& e) {
            std::cerr << "[SWAYAM-RUNNER] Exception: " << e.what() << "\n";
        }

        std::filesystem::remove(src_path, ec);
        std::filesystem::remove(bin_path, ec);
        return execution_success;
    }
};

} // namespace Swayam
#endif // SWAYAM_MUTATION_RUNNER_HPP

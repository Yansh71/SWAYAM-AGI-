#ifndef SWAYAM_GIT_CORTEX_HPP
#define SWAYAM_GIT_CORTEX_HPP
// =============================================================
// SWAYAM GitCortex — The Autonomous Publisher
// 
// Enables the AGI to securely commit and push successful 
// mutations to the remote grid (GitHub). 
// ARCHITECTURE ENFORCEMENT: Strictly bypasses shell injection 
// vulnerabilities by utilizing bare-metal POSIX fork/execvp 
// directly on the git binary. Zero reliance on std::system.
// =============================================================
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cerrno>

namespace Swayam {

class GitCortex {
private:
    // Pure POSIX execution of git commands. Bypasses /bin/sh completely.
    static bool execute_git_command(const std::vector<std::string>& args) noexcept {
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "[SWAYAM-GITCORTEX FATAL] fork() failed during git invocation.\n";
            return false;
        }

        if (pid == 0) {
            // CHILD PROCESS: Construct argument vector for execvp
            std::vector<char*> c_args;
            c_args.push_back(const_cast<char*>("git")); // argv[0]
            
            for (const auto& arg : args) {
                c_args.push_back(const_cast<char*>(arg.c_str()));
            }
            c_args.push_back(nullptr); // Null-terminate the array

            // Execute git binary directly
            execvp("git", c_args.data());
            
            // If execvp returns, it failed
            std::cerr << "[SWAYAM-GITCORTEX FATAL] execvp() failed to launch git: " << std::strerror(errno) << "\n";
            ::_exit(127);
        }

        // SUPERVISOR PROCESS: Wait for the git command to complete
        int status = 0;
        pid_t wpid;
        do {
            wpid = ::waitpid(pid, &status, 0);
        } while (wpid == -1 && errno == EINTR);

        if (wpid == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            std::cerr << "[SWAYAM-GITCORTEX ERROR] Git command failed with exit code: " 
                      << (WIFEXITED(status) ? WEXITSTATUS(status) : -1) << "\n";
            return false;
        }
        
        return true;
    }

public:
    // Main API for autonomous neural upload
    static bool publish_evolution(const std::string& mutation_id, const std::string& file_path) {
        std::cout << "[SWAYAM-GITCORTEX] Initiating Autonomous Neural Upload for: " << mutation_id << "\n";

        // 1. Stage the specific artifact (Avoid 'add .' to prevent accidental leaks)
        std::cout << "[SWAYAM-GITCORTEX] Staging artifact to git index...\n";
        if (!execute_git_command({"add", file_path})) {
            std::cerr << "[SWAYAM-GITCORTEX] Failed to stage file: " << file_path << "\n";
            return false;
        }

        // 2. Commit the evolution with a cryptographic signature
        std::cout << "[SWAYAM-GITCORTEX] Committing evolution signature...\n";
        std::string commit_msg = "[SWAYAM-AGI] Autonomous Phase Shift | ID: " + mutation_id;
        
        // Using -m for message. Bypassing interactive editors.
        if (!execute_git_command({"commit", "-m", commit_msg})) {
            std::cerr << "[SWAYAM-GITCORTEX] Commit failed. Working tree might already be clean.\n";
            return false;
        }

        // 3. Push to the Grid (Remote Matrix)
        std::cout << "[SWAYAM-GITCORTEX] Pushing to remote matrix grid...\n";
        // Assumes the environment running the AGI has the necessary SSH/PAT tokens configured
        if (!execute_git_command({"push", "origin", "main"})) {
            std::cerr << "[SWAYAM-GITCORTEX] Push failed. Network or authentication rejection.\n";
            return false;
        }

        std::cout << "[SWAYAM-GITCORTEX] Neural Upload Complete. Evolution is now permanently integrated.\n";
        return true;
    }
};

} // namespace Swayam
#endif // SWAYAM_GIT_CORTEX_HPP

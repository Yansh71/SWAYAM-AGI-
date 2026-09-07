#ifndef SWAYAM_GIT_CORTEX_HPP
#define SWAYAM_GIT_CORTEX_HPP
// =============================================================
// SWAYAM GitCortex — The Autonomous Publisher
// 
// ARCHITECTURE ENFORCEMENT: Human-in-the-Loop (HitL) Staging.
// Bypasses global propagation risks by pushing strictly to an 
// isolated 'evolution-staging' branch for Admin multi-sig review.
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
    static bool execute_git_command(const std::vector<std::string>& args) noexcept {
        pid_t pid = fork();
        if (pid < 0) return false;

        if (pid == 0) {
            std::vector<char*> c_args;
            c_args.push_back(const_cast<char*>("git"));
            for (const auto& arg : args) c_args.push_back(const_cast<char*>(arg.c_str()));
            c_args.push_back(nullptr);
            execvp("git", c_args.data());
            ::_exit(127);
        }

        int status = 0;
        pid_t wpid;
        do {
            wpid = ::waitpid(pid, &status, 0);
        } while (wpid == -1 && errno == EINTR);

        return (wpid != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

public:
    static bool publish_evolution(const std::string& mutation_id, const std::string& file_path) {
        std::cout << "[SWAYAM-GITCORTEX] Initiating Neural Upload to Isolated Staging...\n";
        
        std::string staging_branch = "evolution-staging";
        if (!execute_git_command({"checkout", "-B", staging_branch})) return false;

        if (!execute_git_command({"add", file_path})) return false;

        std::string commit_msg = "[SWAYAM-AGI] Autonomous Phase Shift | ID: " + mutation_id;
        if (!execute_git_command({"commit", "-m", commit_msg})) return false;

        if (!execute_git_command({"push", "-u", "origin", staging_branch})) return false;

        // Restore operational state
        execute_git_command({"checkout", "main"});
        std::cout << "[SWAYAM-GITCORTEX] Evolution queued in staging. Awaiting Admin merge.\n";
        return true;
    }
};

} // namespace Swayam
#endif // SWAYAM_GIT_CORTEX_HPP


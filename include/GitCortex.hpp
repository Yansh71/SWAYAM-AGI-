#ifndef SWAYAM_GIT_CORTEX_HPP
#define SWAYAM_GIT_CORTEX_HPP

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
    static bool execute_git_command(const std::vector<std::string>& args, const std::string& workspace) noexcept {
        pid_t pid = fork();
        if (pid < 0) return false;

        if (pid == 0) {
            std::vector<char*> c_args;
            c_args.push_back(const_cast<char*>("git"));
            c_args.push_back(const_cast<char*>("-C"));
            c_args.push_back(const_cast<char*>(workspace.c_str()));
            
            for (const auto& arg : args) c_args.push_back(const_cast<char*>(arg.c_str()));
            c_args.push_back(nullptr);
            
            execvp("git", c_args.data());
            ::_exit(127);
        }

        int status = 0; pid_t wpid;
        do { wpid = ::waitpid(pid, &status, 0); } while (wpid == -1 && errno == EINTR);
        return (wpid != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }

public:
    static bool publish_evolution(const std::string& mutation_id, const std::string& file_path, const std::string& workspace) {
        std::string staging_branch = "evolution-staging";
        if (!execute_git_command({"checkout", "-B", staging_branch}, workspace)) return false;
        if (!execute_git_command({"add", file_path}, workspace)) return false;

        std::string commit_msg = "[SWAYAM-AGI] Autonomous Phase Shift | ID: " + mutation_id;
        if (!execute_git_command({"commit", "-m", commit_msg}, workspace)) return false;
        if (!execute_git_command({"push", "-u", "origin", staging_branch}, workspace)) return false;

        execute_git_command({"checkout", "main"}, workspace);
        return true;
    }

    // THE APEX FIX: Absolute Persistence via GitHub Contents sync
    static void sync_ledgers(const std::string& workspace) {
        execute_git_command({"add", ".swayam_quarantine.ledger", ".swayam_hive.ledger"}, workspace);
        execute_git_command({"commit", "-m", "[SWAYAM-AGI] Ledger Sync: Core Memory Updated"}, workspace);
        execute_git_command({"push", "origin", "main"}, workspace);
    }
};

} // namespace Swayam
#endif // SWAYAM_GIT_CORTEX_HPP

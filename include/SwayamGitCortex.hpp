#ifndef SWAYAM_GIT_CORTEX_HPP
#define SWAYAM_GIT_CORTEX_HPP

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

namespace Swayam {

class GitCortex {
private:
    static bool run_git_command(const std::vector<std::string>& args) {
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& a : args) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        pid_t pid = fork();
        if (pid < 0) {
            throw std::runtime_error("[VENOMICA-CORTEX] fork() failed.");
        }
        if (pid == 0) {
            execvp("git", argv.data());
            _exit(127);
        }

        int status = 0;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    static bool looks_like_a_safe_relative_path(const std::string& path) {
        if (path.empty() || path.front() == '-') {
            return false;
        }
        for (char c : path) {
            bool ok = std::isalnum(static_cast<unsigned char>(c)) || 
                      (c == '.') || (c == '/') || (c == '_');
            if (!ok) {
                return false;
            }
        }
        return true;
    }

public:
    static bool publish_successful_mutation(const std::string& mutation_file) {
        std::cout << "[VENOMICA-CORTEX] Engaging Git Cortex. Assimilating successful evolution...\n";

        if (!looks_like_a_safe_relative_path(mutation_file)) {
            std::cerr << "[VENOMICA-CORTEX FATAL] Rejected suspicious mutation path.\n";
            return false;
        }

        if (!run_git_command({"git", "config", "--local", "user.name", "swayam-publisher-bot-yansh[bot]"})) return false;
        if (!run_git_command({"git", "config", "--local", "user.email", "swayam-publisher-bot-yansh[bot]@users.noreply.github.com"})) return false;
        if (!run_git_command({"git", "add", "--", mutation_file})) return false;
        if (!run_git_command({"git", "commit", "-m", "[VENOMICA-AUTONOMY] Autonomous Mutation Assimilated"})) return false;
        if (!run_git_command({"git", "push", "origin", "HEAD:main"})) return false;

        return true;
    }
};

} // namespace Swayam
#endif // SWAYAM_GIT_CORTEX_HPP

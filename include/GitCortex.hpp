#ifndef SWAYAM_GIT_CORTEX_HPP
#define SWAYAM_GIT_CORTEX_HPP

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>

namespace Swayam {

class GitCortex {
public:
    // Uses the same Zero-Trust execvp architecture as MutationRunner
    static bool execute_git_command(const std::vector<std::string>& args) {
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>("git")); // Force git binary
        for (const auto& a : args) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        pid_t pid = fork();
        if (pid < 0) return false;
        
        if (pid == 0) {
            execvp("git", argv.data());
            _exit(127);
        }
        
        int status = 0;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
};

} // namespace Swayam

#endif // SWAYAM_GIT_CORTEX_HPP

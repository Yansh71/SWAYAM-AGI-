#ifndef SWAYAM_GIT_CORTEX_HPP
#define SWAYAM_GIT_CORTEX_HPP

#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>

namespace Swayam {

class GitCortex {
public:
    static bool sync_state() {
        pid_t pid = fork();
        if (pid == 0) {
            const char* args[] = {"git", "status", "--porcelain", nullptr};
            execvp("git", const_cast<char* const*>(args));
            _exit(127);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
        }
        throw std::runtime_error("[VENOMICA] Fork failed in GitCortex.");
    }
};

} // namespace Swayam
#endif // SWAYAM_GIT_CORTEX_HPP

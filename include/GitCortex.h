#ifndef GIT_CORTEX_H
#define GIT_CORTEX_H

#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>

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

#endif // GIT_CORTEX_H

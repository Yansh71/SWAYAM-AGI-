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

// Autonomous publisher for surviving mutations (Phase 10).
// SECURITY NOTE: this file previously built a single shell command
// string and ran it through std::system(). That is a shell-injection
// vector: any character in mutation_file with special meaning to
// /bin/sh (spaces, quotes, backticks, ;, $, etc.) could alter
// or extend the command that actually runs with push access to main.
// Every git invocation below instead goes through fork() + execvp()
// with a fixed argv array, mirroring the pattern already used in
// GitCortex::sync_state(). No shell is ever invoked, so no argument
// value - however it got there - can be interpreted as shell syntax.
class GitCortex {
private:
    // Run a one git subcommand via fork()+execvp(). No shell involved.
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
            // Child: exec git directly with a fixed argv. No shell.
            execvp("git", argv.data());
            _exit(127); // execvp itself failed to start
        }

        int status = 0;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    // Defense in depth. Moltbook::write_mutation() already rejects ..
    // and path separators before a file is ever written to
    // src/generated/. This is a second, independent check on the
    // same string right before it is used to build a git argv.
    // It does not trust that the caller validated anything.
    static bool looks_like_a_safe_relative_path(const std::string& path) {
        if (path.empty() || path.front() == '-') {
            return false; // never let it be parsed as a git option
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
        std::cout << "[VENOMICA-CORTEX] Engaging Git Cortex. "
                  << "Assimilating successful evolution...\n";

        if (!looks_like_a_safe_relative_path(mutation_file)) {
            std::cerr << "[VENOMICA-CORTEX FATAL] Rejected suspicious "
                      << "mutation path before it reached git: "
                      << mutation_file << "\n";
            return false;
        }

        if (!run_git_command({"git", "config", "--local", "user.name",
                              "swayam-publisher-bot-yansh[bot]"})) {
            return false;
        }
        if (!run_git_command({"git", "config", "--local", "user.email",
                              "swayam-publisher-bot-yansh[bot]@users.noreply.github.com"})) {
            return false;
        }

        // -- marks the end of options: mutation_file is always
        // treated as a pathspec, never as a git flag, even if the
        // character whitelist above were somehow bypassed.
        if (!run_git_command({"git", "add", "--", mutation_file})) {
            return false;
        }

        if (!run_git_command({"git", "commit", "-m",
                              "[VENOMICA-AUTONOMY] Autonomous Mutation Assimilated"})) {
            return false;
        }

        if (!run_git_command({"git", "push", "origin", "HEAD:main"})) {
            return false;
        }

        return true;
    }
};

} // namespace Swayam
#endif // SWAYAM_GIT_CORTEX_HPP

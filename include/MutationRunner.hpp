#ifndef SWAYAM_MUTATION_RUNNER_HPP
#define SWAYAM_MUTATION_RUNNER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <stdexcept>

namespace Swayam {

class MutationRunner {
private:
    // Core isolated command execution mechanism (Zero Shell)
    static int run_command_safe(const std::vector<std::string>& args) {
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& a : args) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "[VENOMICA-RUNNER FATAL] fork() failed.\n";
            return -1;
        }
        if (pid == 0) {
            // Child process: Direct exec, no shell interpolation
            execvp(argv[0], argv.data());
            _exit(127); // Force strict exit if execvp fails
        }
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return -1; // Crashed or killed by signal
    }

public:
    // Dynamically compiles and executes the forged C++ mutation
    static bool compile_and_execute(const std::string& source_file, const std::string& output_bin) {
        std::cout << "[VENOMICA-RUNNER] Initiating dynamic compilation of neural payload...\n";

        // Command to compile the generated C++ file using GCC C++23 (No Shell)
        std::vector<std::string> compile_cmd = {"g++", "-std=c++23", source_file, "-o", output_bin};
        
        int compile_status = run_command_safe(compile_cmd);
        if (compile_status != 0) {
            std::cerr << "[VENOMICA-RUNNER FATAL] Dynamic compilation failed. Syntax or Linker error in mutation.\n";
            return false;
        }

        std::cout << "[VENOMICA-RUNNER] Compilation successful. Engaging dynamic isolated execution...\n";

        // Execute the newly created binary safely
        std::vector<std::string> run_cmd = {"./" + output_bin};
        int run_status = run_command_safe(run_cmd);

        if (run_status != 0) {
            std::cerr << "[VENOMICA-RUNNER FATAL] Mutation execution crashed or returned non-zero state.\n";
            return false;
        }

        std::cout << "[VENOMICA-RUNNER] Mutation executed flawlessly. Evolution cycle validated!\n";
        return true;
    }
};

} // namespace Swayam

#endif // SWAYAM_MUTATION_RUNNER_HPP

#ifndef SWAYAM_MUTATION_RUNNER_HPP
#define SWAYAM_MUTATION_RUNNER_HPP

#include <iostream>
#include <string>
#include <cstdlib>

namespace Swayam {

class MutationRunner {
public:
    // Dynamically compiles and executes the forged C++ mutation
    static bool compile_and_execute(const std::string& source_file, const std::string& output_bin) {
        std::cout << "[VENOMICA-RUNNER] Initiating dynamic compilation of neural payload...\n";
        
        // Command to compile the generated C++ file using GCC C++23
        std::string compile_cmd = "g++ -std=c++23 " + source_file + " -o " + output_bin;
        
        int compile_status = std::system(compile_cmd.c_str());
        if (compile_status != 0) {
            std::cerr << "[VENOMICA-RUNNER FATAL] Dynamic compilation failed. Syntax or Linker error in mutation.\n";
            return false;
        }
        
        std::cout << "[VENOMICA-RUNNER] Compilation successful. Engaging dynamic isolated execution...\n";
        
        // Execute the newly created binary
        std::string run_cmd = "./" + output_bin;
        int run_status = std::system(run_cmd.c_str());
        
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

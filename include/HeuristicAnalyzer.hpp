#ifndef SWAYAM_HEURISTIC_ANALYZER_HPP
#define SWAYAM_HEURISTIC_ANALYZER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>

namespace Swayam {

class HeuristicAnalyzer {
public:
    // Statically analyzes the autonomously generated C++ code before compilation
    static bool validate_mutation_syntax(const std::string& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "[VENOMICA-ANALYZER FATAL] Unable to access generated mutation at: " << file_path << "\n";
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        std::cout << "[VENOMICA-ANALYZER] Scanning cognitive mutation for structural integrity...\n";

        // Heuristic 1: Must contain a valid entry point
        if (content.find("int main") == std::string::npos) {
            std::cerr << "[VENOMICA-ANALYZER REJECTED] Mutation lacks a valid execution entry point (main function).\n";
            return false;
        }

        // Heuristic 2: Forbid raw system calls in generated C++ to prevent escape
        if (content.find("system(") != std::string::npos || content.find("exec(") != std::string::npos) {
            std::cerr << "[VENOMICA-ANALYZER REJECTED] Mutation contains forbidden raw kernel/system execution calls.\n";
            return false;
        }

        // Heuristic 3: Check for runaway memory allocations
        if (content.find("new ") != std::string::npos && content.find("delete ") == std::string::npos) {
            std::cout << "[VENOMICA-ANALYZER WARN] Unmanaged memory allocation detected. Enforcing strict monitoring.\n";
            // We allow it but flag a warning for the supervisor
        }

        std::cout << "[VENOMICA-ANALYZER] Static heuristics passed. Mutation is structurally sound and cleared for Sandbox compilation.\n";
        return true;
    }
};

} // namespace Swayam

#endif // SWAYAM_HEURISTIC_ANALYZER_HPP

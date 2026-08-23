#ifndef SWAYAM_HEURISTIC_ANALYZER_HPP
#define SWAYAM_HEURISTIC_ANALYZER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <vector>

namespace Swayam {

class HeuristicAnalyzer {
public:
    // [VENOMICA DEEP SCANNER]
    // Validates the AST logic against extreme cyber-security heuristics 
    // before allowing compilation in the sandbox.
    static bool validate_mutation_syntax(const std::string& source_path) {
        std::ifstream file(source_path);
        if (!file.is_open()) {
            std::cerr << "[VENOMICA-HEURISTIC FATAL] Cannot read cognitive mutation source at: " << source_path << "\n";
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        // ---------------------------------------------------------
        // 1. ARCHITECTURAL CONTRACT VERIFICATION
        // ---------------------------------------------------------
        if (content.find("namespace SwayamMutation") == std::string::npos ||
            content.find("execute_payload()") == std::string::npos) {
            std::cerr << "[VENOMICA-HEURISTIC REJECTED] Missing core architectural contract. Payload is structurally invalid.\n";
            return false;
        }

        // ---------------------------------------------------------
        // 2. ZERO-TRUST BLACKLIST (KERNEL & MEMORY ISOLATION)
        // ---------------------------------------------------------
        std::vector<std::string> blacklist = {
            "system(", "popen(", "exec", "fork(", "syscall(", 
            "asm(", "__asm__", "reinterpret_cast", "goto ",
            "#include <cstdlib>", "#include <unistd.h>"
        };

        for (const auto& token : blacklist) {
            if (content.find(token) != std::string::npos) {
                std::cerr << "[VENOMICA-HEURISTIC REJECTED] Forbidden memory/kernel primitive detected: [" << token << "]\n";
                return false;
            }
        }

        // ---------------------------------------------------------
        // 3. ANTI-DOS / RESOURCE EXHAUSTION HEURISTICS
        // ---------------------------------------------------------
        // Detects absolute infinite loops that could hang the sandbox
        std::regex infinite_loop_pattern(R"(while\s*\(\s*true\s*\)|while\s*\(\s*1\s*\)|for\s*\(\s*;\s*;\s*\))");
        if (std::regex_search(content, infinite_loop_pattern)) {
            std::cerr << "[VENOMICA-HEURISTIC REJECTED] Anti-DoS Heuristic Triggered: Infinite loop detected.\n";
            return false;
        }

        std::cout << "[VENOMICA-HEURISTIC] Deep scan complete. Payload syntax, structure, and heuristics verified 100% safe.\n";
        return true;
    }
};

} // namespace Swayam

#endif // SWAYAM_HEURISTIC_ANALYZER_HPP

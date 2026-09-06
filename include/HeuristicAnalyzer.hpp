#ifndef SWAYAM_HEURISTIC_ANALYZER_HPP
#define SWAYAM_HEURISTIC_ANALYZER_HPP
// =============================================================
// SWAYAM HeuristicAnalyzer — The Cognitive Gatekeeper
// 
// Validates autonomously generated code BEFORE it reaches the 
// SafeShell sandbox. Implements 3-Pass Static Analysis:
// 1. Zero-Trust Blacklist (Blocks OS-level escapes)
// 2. Cyclomatic Complexity Calculation
// 3. Absolute Threshold Enforcement
// =============================================================
#include <string>
#include <string_view>
#include <array>
#include <iostream>

namespace Swayam {

class HeuristicAnalyzer {
private:
    // Pass 1: Fatal Tokens. No autonomous mutation should ever use these.
    static constexpr std::array<std::string_view, 8> forbidden_keywords = {
        "system(", 
        "exec(", 
        "execve(", 
        "fork(", 
        "popen(", 
        "socket(",
        "ptrace(",
        "asm("
    };

    // Pass 2: Control Flow Tokens for Cyclomatic Complexity
    static constexpr std::array<std::string_view, 6> complexity_keywords = {
        "if (", "for (", "while (", "case ", "catch (", "?"
    };

    // Bare-metal string scanner for zero-latency counting
    static size_t count_occurrences(std::string_view code, std::string_view token) noexcept {
        size_t count = 0;
        size_t pos = 0;
        while ((pos = code.find(token, pos)) != std::string_view::npos) {
            ++count;
            pos += token.length();
        }
        return count;
    }

public:
    struct AnalysisResult {
        bool is_safe;
        int complexity_score;
        std::string rejection_reason;
    };

    static AnalysisResult evaluate_mutation(const std::string& source_code) {
        AnalysisResult result { true, 1, "PASSED_ALL_CHECKS" };
        std::string_view code_view = source_code;

        // Step 1: Blacklist Scan
        for (const auto& keyword : forbidden_keywords) {
            if (code_view.find(keyword) != std::string_view::npos) {
                result.is_safe = false;
                result.rejection_reason = "CRITICAL THREAT: Forbidden token detected -> " + std::string(keyword);
                return result;
            }
        }

        // Step 2: Complexity Calculation
        int complexity = 1; // Base path
        for (const auto& keyword : complexity_keywords) {
            complexity += count_occurrences(code_view, keyword);
        }
        result.complexity_score = complexity;

        // Step 3: Threshold Validation
        // Hard-cap at 15 to prevent unpredictable state explosions
        constexpr int MAX_COMPLEXITY = 15;
        if (complexity > MAX_COMPLEXITY) {
            result.is_safe = false;
            result.rejection_reason = "COMPLEXITY VIOLATION: Score (" + 
                                      std::to_string(complexity) + 
                                      ") exceeds maximum threshold (" + 
                                      std::to_string(MAX_COMPLEXITY) + ").";
            return result;
        }

        return result;
    }
};

} // namespace Swayam
#endif // SWAYAM_HEURISTIC_ANALYZER_HPP

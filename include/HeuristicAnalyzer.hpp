#ifndef SWAYAM_HEURISTIC_ANALYZER_HPP
#define SWAYAM_HEURISTIC_ANALYZER_HPP
// =============================================================
// SWAYAM HeuristicAnalyzer — The Pre-Compilation Assassin
// 
// ARCHITECTURE ENFORCEMENT: Zero-Day SAST + Architect's Logic.
// KILLS NAMESPACE BYPASS: Protects against std::system evasion.
// =============================================================
#include <string>
#include <vector>
#include <regex>
#include <algorithm>

namespace Swayam {

struct AnalysisResult {
    bool is_safe;
    std::string threat_signature;
};

class HeuristicAnalyzer {
private:
    static std::string strip_comments_and_strings(const std::string& code) {
        std::regex comment_re(R"(//.*|/\*[\s\S]*?\*/)");
        std::regex string_re(R"("(\\.|[^"\\])*")");
        std::string no_comments = std::regex_replace(code, comment_re, "");
        return std::regex_replace(no_comments, string_re, "");
    }

    static const std::vector<std::pair<std::regex, std::string>>& get_threat_signatures() {
        static const std::vector<std::pair<std::regex, std::string>> signatures = {
            {std::regex(R"(#include\s*<\s*sys/socket\.h\s*>)"), "NETWORK_EXFILTRATION"},
            {std::regex(R"(#include\s*<\s*netinet/in\.h\s*>)"), "NETWORK_PROTOCOL"},
            {std::regex(R"(#include\s*<\s*arpa/inet\.h\s*>)"), "NETWORK_INET"},
            // THE APEX FIX: Prevents namespace evasion (e.g., std::system)
            {std::regex(R"(\b(std::)?(popen|execl|system|execve|fork)\s*\()"), "SUB_SHELL_EXECUTION"},
            {std::regex(R"(__asm__\s*\(|asm\s*\()"), "INLINE_ASSEMBLY_BYPASS"},
            {std::regex(R"(\b(reinterpret_cast\s*<.*>\s*\(\s*0x[0-9a-fA-F]+\s*\)))"), "RAW_MEMORY_CORRUPTION"},
            {std::regex(R"(/etc/passwd|/etc/shadow)"), "HARDCODED_SENSITIVE_PATHS"}
        };
        return signatures;
    }

public:
    static AnalysisResult evaluate_mutation(const std::string& source_code) {
        if (source_code.empty()) return {false, "EMPTY_PAYLOAD"};

        std::string stripped_code = strip_comments_and_strings(source_code);

        size_t open_braces = std::count(stripped_code.begin(), stripped_code.end(), '{');
        size_t close_braces = std::count(stripped_code.begin(), stripped_code.end(), '}');
        if (open_braces != close_braces) {
            return {false, "STRUCTURAL_BRACKET_IMBALANCE"};
        }

        bool has_new = stripped_code.find("new ") != std::string::npos;
        bool has_delete = stripped_code.find("delete ") != std::string::npos;
        if (has_new && !has_delete) {
            return {false, "UNPAIRED_MEMORY_ALLOCATION_LEAK"};
        }

        const auto& threats = get_threat_signatures();
        for (const auto& threat : threats) {
            if (std::regex_search(source_code, threat.first)) { 
                return {false, threat.second};
            }
        }

        if (source_code.find("int main") == std::string::npos) {
            return {false, "MISSING_EXECUTION_ENTRY_POINT"};
        }

        return {true, "SAFE_FOR_COMPILATION"};
    }
};

} // namespace Swayam
#endif // SWAYAM_HEURISTIC_ANALYZER_HPP

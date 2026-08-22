#ifndef SWAYAM_HEURISTIC_ANALYZER_HPP
#define SWAYAM_HEURISTIC_ANALYZER_HPP

// ======
// SWAYAM Phase 6: Predictive Execution Layer
// HeuristicAnalyzer - native, dependency-free C++23 static
// analysis that runs BEFORE compilation.
//
// Philosophy:
//   The crash-safe supervisor (core.hpp) handles what we cannot
//   predict. The heuristic analyzer handles what we can.
//   Together they form a two-tier defense:
//     Tier 1 (this file): deterministic pre-quarantine.
//                         Same input -> same verdict. Auditable.
//     Tier 2 (core.hpp):  fork-isolated execution.
//                         Catches what tier 1 misses.
//
// Design constraints:
//   - Zero external dependencies (no libclang, no LLVM)
//   - O(n) single-scan sanitizer pass (strips comments/strings)
//   - All analysis runs on sanitized source to prevent
//     false-positives from patterns inside string literals
//   - Line numbers preserved via newline counting
//   - CRITICAL findings block compilation entirely
//   - WARNING findings log but allow compilation (tier 2 catches)
// ======

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace Swayam {

class HeuristicAnalyzer {
public:
    // Finding: one rule violation.
    struct Finding {
        enum class Severity { CRITICAL, WARNING, INFO };
        Severity severity;
        std::string rule_id;
        std::string description;
        size_t line_number = 0; // 0 = not line-specific

        std::string severity_str() const noexcept {
            switch (severity) {
                case Severity::CRITICAL: return "CRITICAL";
                case Severity::WARNING:  return "WARNING";
                case Severity::INFO:     return "INFO";
            }
            return "UNKNOWN";
        }
    };

    // AnalysisResult: full verdict for one source string.
    struct AnalysisResult {
        std::vector<Finding> findings;
        bool pre_quarantine = false; // true = block compile, quarantine hash
        uint32_t critical_count = 0;
        uint32_t warning_count = 0;
        uint32_t info_count = 0;
    };

    // analyze() - main entry point.
    // Call before CognitiveForge::compile().
    static AnalysisResult analyze(const std::string& source) {
        AnalysisResult result;

        // Pass 1: structural integrity on raw source
        // (brackets inside comments still indicate malformed source
        // so we check those on raw, then sanitize for pass 2+3)
        const std::string clean = sanitize(source);

        check_structural_integrity(source, clean, result);
        check_unsafe_patterns(clean, source, result);
        check_resource_management(clean, result);
        check_concurrency_patterns(clean, result);

        // Tally
        for (const auto& f : result.findings) {
            switch (f.severity) {
                case Finding::Severity::CRITICAL: result.critical_count++; break;
                case Finding::Severity::WARNING:  result.warning_count++; break;
                case Finding::Severity::INFO:     result.info_count++; break;
            }
        }

        result.pre_quarantine = (result.critical_count > 0);
        return result;
    }

    // report() - write findings to stdout for CI log visibility.
    static void report(const AnalysisResult& r, const std::string& module_name) {
        std::cout << "[HEURISTIC] " << module_name << " -> "
                  << r.critical_count << " critical | "
                  << r.warning_count << " warnings | "
                  << r.info_count << " info\n";

        for (const auto& f : r.findings) {
            std::cout << "[HEURISTIC] [" << f.severity_str()
                      << "][" << f.rule_id << "]";
            if (f.line_number > 0) {
                std::cout << " L" << f.line_number;
            }
            std::cout << " - " << f.description << "\n";
        }

        if (r.pre_quarantine) {
            std::cout << "[HEURISTIC] VERDICT: PRE-QUARANTINE. "
                      << "Compilation blocked. Hash will be logged "
                      << "to quarantine.log with signal=254.\n";
        } else if (r.warning_count > 0) {
            std::cout << "[HEURISTIC] VERDICT: WARNINGS PRESENT. "
                      << "Proceeding to compile - crash-safe supervisor "
                      << "is the second tier.\n";
        } else {
            std::cout << "[HEURISTIC] VERDICT: CLEAN. Proceeding to compile.\n";
        }
    }

private:
    // sanitize(): single O(n) pass.
    // Strips // comments, /* block comments */, string literals,
    // and char literals. Preserves newlines for line counting.
    static std::string sanitize(const std::string& src) {
        std::string out;
        out.reserve(src.size());
        const size_t n = src.size();
        size_t i = 0;
        
        while (i < n) {
            // Line comment
            if (i + 1 < n && src[i] == '/' && src[i+1] == '/') {
                while (i < n && src[i] != '\n') ++i;
                continue;
            }
            // Block comment
            if (i + 1 < n && src[i] == '/' && src[i+1] == '*') {
                i += 2;
                while (i + 1 < n && !(src[i] == '*' && src[i+1] == '/')) {
                    if (src[i] == '\n') out += '\n'; // preserve line numbers
                    ++i;
                }
                i += 2; // consume closing */
                continue;
            }
            // String literal - replace content with spaces
            if (src[i] == '"') {
                out += '"';
                ++i;
                while (i < n && src[i] != '"') {
                    if (src[i] == '\\') { out += ' '; ++i; if (i < n) { out += ' '; ++i; } continue; }
                    out += (src[i] == '\n') ? '\n' : ' ';
                    ++i;
                }
                if (i < n) { out += '"'; ++i; }
                continue;
            }
            // Char literal
            if (src[i] == '\'' && i > 0 && src[i-1] != '\\') {
                out += '\'';
                ++i;
                while (i < n && src[i] != '\'') {
                    out += (src[i] == '\\') ? ' ' : ((src[i] == '\n') ? '\n' : ' ');
                    ++i;
                }
                if (i < n) { out += '\''; ++i; }
                continue;
            }
            out += src[i++];
        }
        return out;
    }

    // find_line(): 1-based line number for position pos in src.
    static size_t find_line(const std::string& src, size_t pos) noexcept {
        size_t count = 1;
        for (size_t i = 0; i < pos && i < src.size(); ++i) {
            if (src[i] == '\n') ++count;
        }
        return count;
    }

    // add(): append a finding.
    static void add(AnalysisResult& r,
                    Finding::Severity sev,
                    const std::string& rule_id,
                    const std::string& desc,
                    size_t line = 0) {
        r.findings.push_back({sev, rule_id, desc, line});
    }

    // count_token(): count non-overlapping occurrences of tok
    // that are not part of a longer alphanumeric token.
    static size_t count_token(const std::string& src, const std::string& tok) noexcept {
        size_t count = 0, pos = 0;
        while ((pos = src.find(tok, pos)) != std::string::npos) {
            size_t after = pos + tok.size();
            bool boundary_after = (after >= src.size() || 
                                   !std::isalnum(static_cast<unsigned char>(src[after])));
            if (boundary_after) ++count;
            ++pos;
        }
        return count;
    }

    // CHECK PASS 1: Structural Integrity
    static void check_structural_integrity(const std::string& raw,
                                           const std::string& clean,
                                           AnalysisResult& r) {
        int64_t curly = 0, paren = 0, square = 0;
        
        for (char c : clean) {
            if (c == '{') ++curly;
            else if (c == '(') ++paren;
            else if (c == '}') --curly;
            else if (c == ')') --paren;
            else if (c == '[') ++square;
            else if (c == ']') --square;
        }
        
        if (curly != 0) {
            add(r, Finding::Severity::CRITICAL, "SI-001",
                std::string("Unbalanced '{}': ") +
                std::to_string(std::abs(curly)) +
                (curly > 0 ? " unclosed" : " extra"));
        }
        if (paren != 0) {
            add(r, Finding::Severity::CRITICAL, "SI-002",
                std::string("Unbalanced '()': ") +
                std::to_string(std::abs(paren)) +
                (paren > 0 ? " unclosed" : " extra"));
        }
        if (square != 0) {
            add(r, Finding::Severity::WARNING, "SI-003",
                "Unbalanced '[]'. Check array subscript expressions.");
        }

        // Include guard
        bool has_pragma = raw.find("#pragma once") != std::string::npos;
        bool has_ifndef = raw.find("#ifndef") != std::string::npos;
        bool has_endif = raw.find("#endif") != std::string::npos;

        if (!has_pragma && !(has_ifndef && has_endif)) {
            add(r, Finding::Severity::WARNING, "SI-004",
                "No include guard (#pragma once or #ifndef/#endif). "
                "Multiple inclusion will cause ODR violations.");
        }
    }

    // CHECK PASS 2: Unsafe Patterns
    static void check_unsafe_patterns(const std::string& clean,
                                      const std::string& raw,
                                      AnalysisResult& r) {
        struct Rule {
            std::string id;
            std::string pattern;
            Finding::Severity sev;
            std::string desc;
        };

        static const std::vector<Rule> rules = {
            {"UP-001", "system(", Finding::Severity::CRITICAL, "std::system() detected. Mutation must not spawn processes via shell."},
            {"UP-002", "execvp(", Finding::Severity::CRITICAL, "execvp() in generated code. Process spawning is the supervisor's role only."},
            {"UP-003", "execve(", Finding::Severity::CRITICAL, "execve() in generated code. Same constraint as UP-002."},
            {"UP-004", "fork(", Finding::Severity::CRITICAL, "fork() in generated code. Mutations must not create child processes."},
            {"UP-005", "popen(", Finding::Severity::CRITICAL, "popen() detected - shell execution, pipe to process. Equivalent risk to system()."},
            {"UP-006", "strcpy(", Finding::Severity::CRITICAL, "strcpy() - no bounds check, guaranteed stack smash risk. Use std::string or strncpy."},
            {"UP-007", "strcat(", Finding::Severity::CRITICAL, "strcat() - no bounds check. Use std::string operator+=."},
            {"UP-008", "gets(", Finding::Severity::CRITICAL, "gets() removed in C++14. Use std::getline."},
            {"UP-009", "sprintf(", Finding::Severity::CRITICAL, "sprintf() - no bounds check. Use snprintf or std::format."},
            {"UP-010", "scanf(", Finding::Severity::CRITICAL, "scanf() without width specifier is unsafe. Use std::cin or explicit width."},
            {"UP-011", "reinterpret_cast", Finding::Severity::CRITICAL, "reinterpret_cast near-certain undefined behaviour in generated code."},
            {"UP-012", "new", Finding::Severity::WARNING, "Raw 'new' - prefer std::make_unique / std::make_shared."},
            {"UP-013", "delete", Finding::Severity::WARNING, "Raw 'delete' pairs with raw new. Prefer RAII."},
            {"UP-014", "delete[]", Finding::Severity::WARNING, "Raw 'delete[]' - prefer std::vector/std::array."},
            {"UP-015", "malloc(", Finding::Severity::WARNING, "malloc() in C++ - prefer 'new' or std::vector."},
            {"UP-016", "free(", Finding::Severity::WARNING, "free() in C++ mixes C/C++ memory management."},
            {"UP-017", "memcpy(", Finding::Severity::WARNING, "memcpy() - verify source and destination buffer sizes."},
            {"UP-018", "memset(", Finding::Severity::WARNING, "memset() on non-trivial type overwrites vtable pointer."},
            {"UP-019", "const_cast", Finding::Severity::WARNING, "const_cast modifying a const object via cast is UB."},
            {"UP-020", "goto ", Finding::Severity::WARNING, "'goto' bypasses RAII destructors - jumps past variable declarations leak resources."},
            {"UP-021", "using namespace std", Finding::Severity::INFO, "'using namespace std' in header contaminates all translation units."},
            {"UP-022", "volatile", Finding::Severity::INFO, "'volatile' for inter-thread communication, prefer std::atomic."}
        };

        for (const auto& rule : rules) {
            size_t pos = clean.find(rule.pattern);
            if (pos != std::string::npos) {
                size_t line = find_line(raw, pos);
                add(r, rule.sev, rule.id, rule.desc + " (first at line " + std::to_string(line) + ")", line);
            }
        }
    }

    // CHECK PASS 3A: Resource Management Pairing
    static void check_resource_management(const std::string& clean, AnalysisResult& r) {
        const size_t new_count = count_token(clean, "new");
        const size_t del_count = count_token(clean, "delete");
        const size_t del_arr_count = count_token(clean, "delete[]");
        const size_t total_delete = del_count + del_arr_count;

        // SWAYAM-ANALYZER-SUPPRESS logic can be added here in the future if needed

        if (new_count > 0 && total_delete == 0) {
            add(r, Finding::Severity::CRITICAL, "RM-001",
                std::to_string(new_count) + " raw 'new' with zero 'delete'. Guaranteed memory leak.");
        } else if (new_count > 0 && total_delete > 0 && new_count != total_delete) {
            Finding::Severity sev = (new_count < total_delete) ? Finding::Severity::CRITICAL : Finding::Severity::WARNING;
            add(r, sev, "RM-002",
                "new/delete count mismatch: " + std::to_string(new_count) + " new vs " + std::to_string(total_delete) + " delete. " +
                (new_count < total_delete ? "Potential double-free or delete of stack memory." : "Potential memory leak."));
        }

        const size_t fopen_count = count_token(clean, "fopen(");
        const size_t fclose_count = count_token(clean, "fclose(");
        if (fopen_count > 0 && fclose_count == 0) {
            add(r, Finding::Severity::WARNING, "RM-003",
                std::to_string(fopen_count) + " fopen() with no fclose(). File descriptor leak.");
        }

        const size_t malloc_count = count_token(clean, "malloc(");
        const size_t free_count = count_token(clean, "free(");
        if (malloc_count > 0 && free_count == 0) {
            add(r, Finding::Severity::WARNING, "RM-004",
                std::to_string(malloc_count) + " malloc() with no free(). Memory leak.");
        } else if (malloc_count < free_count && free_count > 0) {
            add(r, Finding::Severity::CRITICAL, "RM-005",
                "More free() than malloc() - potential double-free.");
        }
    }

    // CHECK PASS 3B: Concurrency Patterns
    static void check_concurrency_patterns(const std::string& clean, AnalysisResult& r) {
        const bool has_mutex = clean.find("std::mutex") != std::string::npos;
        const bool has_lock_fn = clean.find(".lock()") != std::string::npos;
        const bool has_guard = clean.find("lock_guard") != std::string::npos;
        const bool has_unique = clean.find("unique_lock") != std::string::npos;
        const bool has_scoped = clean.find("scoped_lock") != std::string::npos;

        if (has_mutex && has_lock_fn && !(has_guard || has_unique || has_scoped)) {
            add(r, Finding::Severity::CRITICAL, "CP-001",
                "std::mutex::lock() without lock_guard/scoped_lock/unique_lock. "
                "An exception between lock() and unlock() causes permanent "
                "deadlock - the exact failure mode AtomicGuard was designed "
                "to prevent. Use RAII locking.");
        }

        auto check_infinite = [&](const std::string& pattern) {
            size_t pos = clean.find(pattern);
            if (pos == std::string::npos) return;
            
            size_t end = std::min(pos + 600, clean.size());
            std::string window = clean.substr(pos, end - pos);
            
            if (window.find("break") == std::string::npos &&
                window.find("return") == std::string::npos &&
                window.find("throw") == std::string::npos &&
                window.find("exit(") == std::string::npos &&
                window.find("_exit(") == std::string::npos) {
                add(r, Finding::Severity::WARNING, "CP-002",
                    "Infinite loop pattern '" + pattern + "' with no "
                    "visible break/return/throw/exit in body (~600 chars). "
                    "May cause sandbox timeout and SIGKILL.");
            }
        };

        check_infinite("while(true)");
        check_infinite("while (true)");
        check_infinite("for(;;)");
        check_infinite("for( ; ; )");
    }
};

} // namespace Swayam
#endif // SWAYAM_HEURISTIC_ANALYZER_HPP

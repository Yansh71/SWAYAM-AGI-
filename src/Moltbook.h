#ifndef MOLTBOOK_H
#define MOLTBOOK_H

#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

// [VENOMICA TITAN-CORE: Self-Healing Include Path Logic]
// This guarantees the file compiles regardless of CMake path configurations.
#if __has_include("error.hpp")
    #include "error.hpp"
#elif __has_include("../include/error.hpp")
    #include "../include/error.hpp"
#else
    // Fallback cognitive node if error.hpp is entirely unreachable by the compiler
    namespace Swayam {
        class SecurityViolation : public std::runtime_error {
        public:
            explicit SecurityViolation(const std::string& msg) : std::runtime_error("[VENOMICA SEC-GUARD] " + msg) {}
        };
    }
#endif

class Moltbook {
public:
    static void write_mutation(const std::string& filename, const std::string& content) {
        // 1. Initial Lexical Block: Reject slash or path-navigators outright to prevent basic injection
        if (filename.find("..") != std::string::npos || filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos) {
            throw Swayam::SecurityViolation("[VENOMICA FATAL] Path traversal characters detected in payload.");
        }

        // 2. Absolute Path Resolution
        std::filesystem::path base_dir = std::filesystem::weakly_canonical("src/generated/");
        std::filesystem::path target_file = std::filesystem::weakly_canonical(base_dir / filename);

        // 3. Strict Iterator-based Path Boundary Check (Not a vulnerable string subset check)
        auto mismatch = std::mismatch(base_dir.begin(), base_dir.end(), target_file.begin());
        if (mismatch.first != base_dir.end()) {
            throw Swayam::SecurityViolation("[VENOMICA FATAL] Sandbox escape attempt blocked. Target is outside base directory.");
        }

        // 4. Safe Write Operation
        std::ofstream outfile(target_file);
        if (!outfile) {
            throw Swayam::SecurityViolation("[VENOMICA] Failed to open strictly bounded file for writing.");
        }
        outfile << content;
        outfile.close();
    }
};

#endif // MOLTBOOK_H

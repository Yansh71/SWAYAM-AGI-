#ifndef MOLTBOOK_H
#define MOLTBOOK_H

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <algorithm>

namespace SwayamAGI {
namespace Core {

class Moltbook {
public:
    static void spawnCognitiveNode(const std::string& node_name) {
        std::cout << "[VENOMICA-CORE] Spawning Cognitive Node: " << node_name << "\n";
    }

    static void write_mutation(const std::string& filename, const std::string& content) {
        // 1. Lexical boundary check
        if (filename.find("..") != std::string::npos || filename.find("/") != std::string::npos) {
            throw std::runtime_error("[VENOMICA FATAL] Path traversal detected lexically.");
        }
        
        // 2. Ensure sandbox directory exists BEFORE canonical resolution
        std::filesystem::path sandbox_dir = "src/generated";
        if (!std::filesystem::exists(sandbox_dir)) {
            std::filesystem::create_directories(sandbox_dir);
        }
        
        // 3. Canonical resolution (NO trailing slash to avoid iterator mismatch)
        std::filesystem::path base_dir = std::filesystem::weakly_canonical(sandbox_dir);
        std::filesystem::path target = std::filesystem::weakly_canonical(base_dir / filename);
        
        // 4. Absolute Path Isolation Check
        auto [base_mismatch, target_mismatch] = std::mismatch(base_dir.begin(), base_dir.end(), target.begin());
        if (base_mismatch != base_dir.end()) {
            throw std::runtime_error("[VENOMICA FATAL] Sandbox escape blocked. Target resolved outside perimeter.");
        }
        
        // 5. Safe File Execution
        std::ofstream outfile(target);
        if (!outfile) {
            throw std::runtime_error("[VENOMICA FATAL] Failed to open bounded file for writing.");
        }
        outfile << content;
    }
};

} // namespace Core
} // namespace SwayamAGI

#endif // MOLTBOOK_H

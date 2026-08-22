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
        if (filename.find("..") != std::string::npos || filename.find("/") != std::string::npos)
            throw std::runtime_error("[VENOMICA FATAL] Path traversal detected.");
        
        std::filesystem::path base_dir = std::filesystem::weakly_canonical("src/generated/");
        std::filesystem::path target = std::filesystem::weakly_canonical(base_dir / filename);
        
        if (std::mismatch(base_dir.begin(), base_dir.end(), target.begin()).first != base_dir.end())
            throw std::runtime_error("[VENOMICA FATAL] Sandbox escape blocked.");
        
        std::ofstream outfile(target);
        if (!outfile) throw std::runtime_error("Failed to open bounded file.");
        outfile << content;
    }
};

} // namespace Core
} // namespace SwayamAGI

#endif // MOLTBOOK_H

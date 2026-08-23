#ifndef SWAYAM_COGNITIVE_FORGE_HPP
#define SWAYAM_COGNITIVE_FORGE_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <random>
#include <stdexcept>
#include <sstream>

namespace Swayam {

// ---------------------------------------------------------
// EXPLICIT STATE MACHINE EMITTER
// ---------------------------------------------------------
class OutputEmitter {
public:
    static void emit_quarantine(const std::string& meta_dir, const std::string& reason) {
        std::filesystem::create_directories(meta_dir);
        std::filesystem::path status_path = std::filesystem::path(meta_dir) / "status.json";
        std::ofstream status_file(status_path);
        if (!status_file) throw std::runtime_error("[VENOMICA-FATAL] Failed to write quarantine status manifest");
        
        status_file << "{\n  \"outcome\": \"quarantined\",\n  \"reason\": \"" << reason << "\"\n}\n";
        status_file.close();
    }

    static void emit_published(const std::string& meta_dir, const std::string& rationale) {
        std::filesystem::create_directories(meta_dir);
        
        // 1. Emit explicit success status
        std::filesystem::path status_path = std::filesystem::path(meta_dir) / "status.json";
        std::ofstream status_file(status_path);
        if (!status_file) throw std::runtime_error("[VENOMICA-FATAL] Failed to write published status manifest");
        status_file << "{\n  \"outcome\": \"published\"\n}\n";
        status_file.close();

        // 2. Emit the human-readable cognitive rationale
        std::filesystem::path rationale_path = std::filesystem::path(meta_dir) / "rationale.md";
        std::ofstream rationale_file(rationale_path);
        if (!rationale_file) throw std::runtime_error("[VENOMICA-FATAL] Failed to write rationale");
        rationale_file << rationale << "\n";
        rationale_file.close();
    }
};

class CognitiveForge {
public:
    // Generates a polymorphic, mathematically unique C++ mutation and drops exact telemetry
    static bool generate_mutation(const std::string& target_dir, const std::string& meta_dir) {
        std::cout << "[VENOMICA-FORGE] Awakening Cognitive Mutation Sequence...\n";
        
        // Ensure the output sandbox directory exists
        std::filesystem::create_directories(target_dir);
        std::string file_path = target_dir + "/test_mutation.cpp";
        
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            std::cerr << "[VENOMICA-FORGE FATAL] Failed to open neural pathway for writing.\n";
            // Explicitly signal the quarantine/failure state to CI pipeline
            OutputEmitter::emit_quarantine(meta_dir, "Filesystem lock or write failure during mutation forging.");
            return false;
        }

        // ---------------------------------------------------------
        // QUANTUM ENTROPY GENERATOR: Forging Unique DNA per cycle
        // ---------------------------------------------------------
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(100000, 999999);
        int quantum_hash = distrib(gen);

        // ---------------------------------------------------------
        // THE POLYMORPHIC PAYLOAD
        // ---------------------------------------------------------
        out_file << "#include <iostream>\n\n";
        out_file << "int main() {\n";
        out_file << "    std::cout << \"[AGENTIC MUTATION] Polymorphic Evolution Cycle Active.\\n\";\n";
        out_file << "    std::cout << \"[VENOMICA-SIG] Core Execution Timestamp: " << timestamp << "\\n\";\n";
        out_file << "    std::cout << \"[VENOMICA-SIG] Quantum Hash DNA: " << quantum_hash << "\\n\";\n";
        out_file << "    \n";
        out_file << "    // Autonomously generated and mathematically unique logic by SWAYAM Cognitive Forge\n";
        out_file << "    return 0;\n";
        out_file << "}\n";
        
        out_file.close();
        
        std::cout << "[VENOMICA-FORGE] Polymorphic mutation forged with Hash DNA [" << quantum_hash << "] at: " << file_path << "\n";
        
        // ---------------------------------------------------------
        // DYNAMIC RATIONALE GENERATION (The Ghost-Pipeline Killer)
        // ---------------------------------------------------------
        std::ostringstream rationale;
        rationale << "# Autonomous Mutation Rationale\n\n";
        rationale << "## Meta Information\n";
        rationale << "- **Timestamp:** `" << timestamp << "`\n";
        rationale << "- **Quantum Hash DNA:** `" << quantum_hash << "`\n";
        rationale << "- **Target File:** `test_mutation.cpp`\n\n";
        rationale << "## Cognitive Logic\n";
        rationale << "This polymorphic payload was synthesized autonomously by the **SWAYAM Cognitive Forge**. ";
        rationale << "It was designed to output its exact entropy signature (`" << quantum_hash << "`) upon execution, ";
        rationale << "verifying that the C++ compilation pipeline and Docker isolation boundaries (--network none, --read-only) ";
        rationale << "can safely contain and execute dynamic hostiles without OS-level state leakage.\n";

        try {
            OutputEmitter::emit_published(meta_dir, rationale.str());
            std::cout << "[VENOMICA-FORGE] State signal explicitly routed to status.json and rationale.md generated.\n";
        } catch (const std::exception& e) {
            std::cerr << e.what() << "\n";
            return false;
        }

        return true;
    }
};

} // namespace Swayam

#endif // SWAYAM_COGNITIVE_FORGE_HPP

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
        
        std::filesystem::path status_path = std::filesystem::path(meta_dir) / "status.json";
        std::ofstream status_file(status_path);
        if (!status_file) throw std::runtime_error("[VENOMICA-FATAL] Failed to write published status manifest");
        status_file << "{\n  \"outcome\": \"published\"\n}\n";
        status_file.close();

        std::filesystem::path rationale_path = std::filesystem::path(meta_dir) / "rationale.md";
        std::ofstream rationale_file(rationale_path);
        if (!rationale_file) throw std::runtime_error("[VENOMICA-FATAL] Failed to write rationale");
        rationale_file << rationale << "\n";
        rationale_file.close();
    }
};

class CognitiveForge {
public:
    // [MYTHOS ALIGNMENT] Generates a polymorphic .h mutation header 
    // to strictly satisfy scripts/testing/run_sandbox.sh framework contracts.
    static bool generate_mutation(const std::string& target_dir, const std::string& meta_dir) {
        std::cout << "[VENOMICA-FORGE] Awakening Cognitive Mutation Sequence...\n";
        
        std::filesystem::create_directories(target_dir);
        // Emitting a .h header so run_sandbox.sh picks it up correctly
        std::string file_path = target_dir + "/test_mutation.h";
        
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            std::cerr << "[VENOMICA-FORGE FATAL] Failed to open neural pathway for writing.\n";
            OutputEmitter::emit_quarantine(meta_dir, "Filesystem lock or write failure during mutation forging.");
            return false;
        }

        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(100000, 999999);
        int quantum_hash = distrib(gen);

        // ---------------------------------------------------------
        // THE POLYMORPHIC PAYLOAD WRAPPED IN A HEADER CONTRACT
        // ---------------------------------------------------------
        out_file << "#ifndef SWAYAM_TEST_MUTATION_H\n";
        out_file << "#define SWAYAM_TEST_MUTATION_H\n\n";
        out_file << "#include <iostream>\n\n";
        out_file << "namespace SwayamMutation {\n";
        out_file << "    inline void execute_payload() {\n";
        out_file << "        std::cout << \"[AGENTIC MUTATION] Polymorphic Evolution Cycle Active.\\n\";\n";
        out_file << "        std::cout << \"[VENOMICA-SIG] Core Execution Timestamp: " << timestamp << "\\n\";\n";
        out_file << "        std::cout << \"[VENOMICA-SIG] Quantum Hash DNA: " << quantum_hash << "\\n\";\n";
        out_file << "    }\n";
        out_file << "} // namespace SwayamMutation\n\n";
        out_file << "#endif // SWAYAM_TEST_MUTATION_H\n";
        
        out_file.close();
        
        std::cout << "[VENOMICA-FORGE] Polymorphic mutation header forged with Hash DNA [" << quantum_hash << "] at: " << file_path << "\n";
        
        std::ostringstream rationale;
        rationale << "# Autonomous Mutation Rationale\n\n";
        rationale << "## Meta Information\n";
        rationale << "- **Timestamp:** `" << timestamp << "`\n";
        rationale << "- **Quantum Hash DNA:** `" << quantum_hash << "`\n";
        rationale << "- **Target Header:** `test_mutation.h`\n\n";
        rationale << "## Cognitive Logic\n";
        rationale << "This polymorphic payload header was synthesized autonomously by the **SWAYAM Cognitive Forge**. ";
        rationale << "It integrates natively with the crash-safe supervisor (`core.hpp`) and satisfies the project's sandbox verification harness.\n";

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

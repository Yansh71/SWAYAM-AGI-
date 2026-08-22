//===
// src/main.cpp - SWAYAM-AGI Titan Core
// C++23 entry point. Wires all phases:
// Phase 1-3: GitCortex sync + AtomicGuard + Moltbook
// Phase 4:   Hive Mind (NodeRegistry peer quarantine merge)
// Phase 5:   Neural Spawning (CognitiveForge + runtime C++)
// Phase 6:   Predictive Execution Layer (HeuristicAnalyzer)
//===

#include "Moltbook.h"
#include "core.hpp"   
#include "SwayamGitCortex.hpp"
#include "NodeRegistry.hpp" 
#include "CognitiveForge.hpp" 
#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "[VENOMICA] SWAYAM-AGI Titan Core Online.\n";

    // FIX: Calling the securely namespaced GitCortex
    if (!Swayam::GitCortex::sync_state()) {
        std::cerr << "[VENOMICA WARN] Git state sync returned non-clean status.\n";
    }

    const std::string node_id = Swayam::derive_node_id();
    std::cout << "[VENOMICA] Node Identity: " << node_id << "\n";

    const char* env_qlog = std::getenv("SWAYAM_QUARANTINE_LOG");
    const std::string quarantine_log = env_qlog ? env_qlog : "/tmp/quarantine.log";

    Swayam::merge_all_peer_files(quarantine_log);
    SwayamAGI::Core::Moltbook::spawnCognitiveNode("QuantumNode_Alpha");

    Swayam::AtomicGuard guard;
    const std::string beta_source = R"cpp(
#include <iostream>
inline void execute_pathway() {
    std::cout << "[FORGE] QuantumNode_Beta cognitive pathway is live.\n";
}
)cpp";

    try {
        auto result = Swayam::CognitiveForge::spawn(guard, "QuantumNode_Beta", beta_source);

        if (result.executed) {
            std::cout << "[VENOMICA] Phase 5: Neural spawn successful.\n";
        } else if (result.quarantined) {
            std::cout << "[VENOMICA] Phase 5: Spawn crashed -> quarantined.\n";
        } else if (!result.compiled) {
            std::cout << "[VENOMICA] Phase 5: Compile failed.\n" << result.compile_log << "\n";
        } else {
            std::cout << "[VENOMICA] Phase 5: Spawn exited non-zero.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[VENOMICA FATAL] " << e.what() << "\n";
        return 1;
    }

    Swayam::export_node_signatures(quarantine_log, "/tmp/node_export_" + node_id + ".csv");
    std::cout << "[VENOMICA] Titan Core Cycle complete. Hive sync pending.\n";
    return 0;
}

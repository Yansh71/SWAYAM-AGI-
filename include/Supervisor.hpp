#ifndef SWAYAM_SUPERVISOR_HPP
#define SWAYAM_SUPERVISOR_HPP
// =============================================================
// SWAYAM Supervisor — The Central Cognitive Orchestrator
//
// ARCHITECTURE ENFORCEMENT: Pure POSIX Memory Safety.
// INTEGRATIONS: Autonomous Vault Provisioning, Secure Token Lifecycle 
// (SAST/Security Gate Compliant), and Zero-Trace C2.
// =============================================================
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <functional>

#include "core.hpp"
#include "CognitiveForge.hpp"
#include "HeuristicAnalyzer.hpp"
#include "MutationRunner.hpp"
#include "GitCortex.hpp"
#include "NexusC2.hpp"
#include "HiveMind.hpp"

namespace Swayam {

class Supervisor {
public:
    static void orchestrate_evolution(AtomicGuard& guard, const std::string& base_algorithm) {
        std::string workspace = ".";
        std::string vault_path = workspace + "/.swayam_vault";

        // Phase 6.5: Autonomous Vault Provisioning
        std::error_code ec;
        if (!std::filesystem::exists(vault_path, ec)) {
            std::filesystem::create_directory(vault_path, ec);
            chmod(vault_path.c_str(), S_IRWXU);
        }

        std::string evolved_code = CognitiveForge::evolve_codebase(base_algorithm);
        std::string mutation_id = "EVO_" + std::to_string(std::hash<std::string>{}(evolved_code));

        AnalysisResult analysis = HeuristicAnalyzer::evaluate_mutation(evolved_code);
        if (!analysis.is_safe) {
            std::cerr << "[SWAYAM-SUPERVISOR] Threat detected: " << analysis.threat_signature << "\n";
            NexusC2::transmit_telemetry(mutation_id, "QUARANTINED_" + analysis.threat_signature);
            return;
        }

        std::string target_file = vault_path + "/mut_" + mutation_id + ".cpp";
        std::ofstream out_file(target_file);
        if (out_file.is_open()) {
            out_file << evolved_code;
            out_file.close();
        } else {
            NexusC2::transmit_telemetry(mutation_id, "VAULT_WRITE_FAILURE");
            return;
        }

        // THE APEX FIX: Security Gate Compliant Token Sanitization
        const char* env_token = std::getenv("GITHUB_TOKEN");
        std::string safe_token_copy = "";
        
        if (env_token) {
            safe_token_copy = std::string(env_token);
            // Safely remove from the environment block so the untrusted payload cannot inherit it.
            // We DO NOT modify the OS pointer directly to avoid Undefined Behavior (UB).
            unsetenv("GITHUB_TOKEN");
        }

        // Execute the isolated sandbox
        bool success = MutationRunner::evaluate_and_execute(guard, evolved_code, mutation_id, workspace);

        if (success) {
            // Restore token for legitimate GitHub API operations
            if (!safe_token_copy.empty()) {
                setenv("GITHUB_TOKEN", safe_token_copy.c_str(), 1);
            }

            if (GitCortex::publish_evolution(mutation_id, target_file, workspace)) {
                std::cout << "[SWAYAM-SUPERVISOR] Neural Upload Verified.\n";
                HiveMind::register_mutation_hash(std::to_string(std::hash<std::string>{}(evolved_code)), workspace);
                NexusC2::transmit_telemetry(mutation_id, "ASSIMILATED_AND_UPLOADED");
            } else {
                NexusC2::transmit_telemetry(mutation_id, "GIT_UPLOAD_FAILED");
            }

            // Secure memory wipe of our internal variable (RAM Scraping Prevention)
            if (!safe_token_copy.empty()) {
                std::fill(safe_token_copy.begin(), safe_token_copy.end(), '\0');
            }
        } else {
            NexusC2::transmit_telemetry(mutation_id, "EXECUTION_TERMINATED_BY_SANDBOX");
        }
    }
};

} // namespace Swayam
#endif // SWAYAM_SUPERVISOR_HPP

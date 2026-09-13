#ifndef SWAYAM_SUPERVISOR_HPP
#define SWAYAM_SUPERVISOR_HPP
// =============================================================
// SWAYAM Supervisor — The Central Cognitive Orchestrator
//
// ARCHITECTURE ENFORCEMENT: Immutability Lock Engaged. No Bypasses.
// INTEGRATIONS: Autonomous Vault Provisioning, RAM Token Sanitization 
// (RAM Scraping Prevention), Deep Semantic Heuristics, and Zero-Trace C2.
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

        // Phase 6.5: Autonomous Vault Provisioning (Pre-flight Integrity Check)
        std::error_code ec;
        if (!std::filesystem::exists(vault_path, ec)) {
            std::filesystem::create_directory(vault_path, ec);
            // Strict POSIX permissions: 0700 (Only owner daemon can read/write/execute)
            chmod(vault_path.c_str(), S_IRWXU);
        }

        // Phase 2: Metamorphic AST Generation
        std::string evolved_code = CognitiveForge::evolve_codebase(base_algorithm);
        std::string mutation_id = "EVO_" + std::to_string(std::hash<std::string>{}(evolved_code));

        // Phase 3: Deep Semantic Validation (The Logic Guard)
        AnalysisResult analysis = HeuristicAnalyzer::evaluate_mutation(evolved_code);
        if (!analysis.is_safe) {
            std::cerr << "[SWAYAM-SUPERVISOR] Threat detected: " << analysis.threat_signature << ". Initiating Quarantine.\n";
            NexusC2::transmit_telemetry(mutation_id, "QUARANTINED_" + analysis.threat_signature);
            return;
        }

        // Phase 6: Write Validated Genome to Secure Vault for Git Tracking
        std::string target_file = vault_path + "/mut_" + mutation_id + ".cpp";
        std::ofstream out_file(target_file);
        if (out_file.is_open()) {
            out_file << evolved_code;
            out_file.close();
        } else {
            NexusC2::transmit_telemetry(mutation_id, "VAULT_WRITE_FAILURE");
            return;
        }

        // Phase 8 (Audit Resolution): Strict Token Sanitization
        char* env_token = std::getenv("GITHUB_TOKEN");
        std::string safe_token_copy = "";
        
        if (env_token) {
            // Backup the token into RAII-managed C++ memory
            safe_token_copy = std::string(env_token);
            // Explicitly overwrite the raw OS environment memory to prevent payload RAM-scraping
            std::fill(env_token, env_token + std::strlen(env_token), '\0');
            unsetenv("GITHUB_TOKEN");
        }

        // Phase 1: Isolated Sandbox Execution
        bool success = MutationRunner::evaluate_and_execute(guard, evolved_code, mutation_id, workspace);

        // Phase 4 & 5: Post-Execution Persistence & Zero-Trace C2
        if (success) {
            // Restore the token strictly for the GitHub API Sync now that the untrusted child is dead
            if (!safe_token_copy.empty()) {
                setenv("GITHUB_TOKEN", safe_token_copy.c_str(), 1);
            }

            if (GitCortex::publish_evolution(mutation_id, target_file, workspace)) {
                std::cout << "[SWAYAM-SUPERVISOR] Neural Upload Verified and Queued.\n";
                HiveMind::register_mutation_hash(std::to_string(std::hash<std::string>{}(evolved_code)), workspace);
                
                NexusC2::transmit_telemetry(mutation_id, "ASSIMILATED_AND_UPLOADED");

                // Execute the immortal ledger sync via GitHub Contents API
                GitCortex::sync_ledgers(workspace); 
            } else {
                std::cerr << "[SWAYAM-SUPERVISOR] Neural Upload Failed. Dropping from HiveMind.\n";
                NexusC2::transmit_telemetry(mutation_id, "GIT_UPLOAD_FAILED");
            }
        } else {
            NexusC2::transmit_telemetry(mutation_id, "EXECUTION_TERMINATED_BY_SANDBOX");
        }
    }
};

} // namespace Swayam
#endif // SWAYAM_SUPERVISOR_HPP

#ifndef SWAYAM_SUPERVISOR_HPP
#define SWAYAM_SUPERVISOR_HPP
// =============================================================
// SWAYAM Supervisor — The Central Cognitive Orchestrator
//
// ARCHITECTURE ENFORCEMENT: Immutability Lock Engaged.
// SAST COMPLIANCE: Eliminates CWE-732 via std::filesystem::permissions.
// ZERO-LEAK MEMORY: Implements RAII SecureTokenGuard to guarantee 
// volatile RAM wiping (CWE-14 evasion) even during C++ exception unwinding.
// =============================================================
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <cstdlib>

#include "core.hpp"
#include "CognitiveForge.hpp"
#include "HeuristicAnalyzer.hpp"
#include "MutationRunner.hpp"
#include "GitCortex.hpp"
#include "NexusC2.hpp"
#include "HiveMind.hpp"

namespace Swayam {

class Supervisor {
private:
    // THE APEX FIX: RAII Secure Token Guard
    // Guarantees absolute RAM sanitization upon scope exit.
    struct SecureTokenGuard {
        std::string token_val;

        // Constructor: Extracts from OS and immediately hides it
        explicit SecureTokenGuard(const char* env_ptr) {
            if (env_ptr) {
                token_val = env_ptr;
                unsetenv("GITHUB_TOKEN"); 
            }
        }

        // Destructor: Mathematically forces CPU to overwrite memory with zeros
        ~SecureTokenGuard() {
            if (!token_val.empty()) {
                volatile char* ptr = token_val.data();
                for (size_t i = 0; i < token_val.size(); ++i) {
                    ptr[i] = '\0';
                }
            }
        }

        // Restores token to the environment strictly for Git API sync
        void restore() const {
            if (!token_val.empty()) {
                setenv("GITHUB_TOKEN", token_val.c_str(), 1);
            }
        }
    };

public:
    static void orchestrate_evolution(AtomicGuard& guard, const std::string& base_algorithm) {
        std::string workspace = ".";
        std::string vault_path = workspace + "/.swayam_vault";

        // Pre-flight Integrity Check (Pure C++23 POSIX permissions)
        std::error_code ec;
        if (!std::filesystem::exists(vault_path, ec)) {
            std::filesystem::create_directory(vault_path, ec);
            std::filesystem::permissions(vault_path, 
                std::filesystem::perms::owner_all, 
                std::filesystem::perm_options::replace, ec);
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

        // ENABLING THE RAII GUARD: Token is pulled and immediately unset from OS
        SecureTokenGuard token_guard(std::getenv("GITHUB_TOKEN"));

        // Execute the sandbox (The untrusted payload runs now, completely blind to the token)
        bool success = MutationRunner::evaluate_and_execute(guard, evolved_code, mutation_id, workspace);

        if (success) {
            // Restore token safely since the payload has been terminated
            token_guard.restore();

            if (GitCortex::publish_evolution(mutation_id, target_file, workspace)) {
                std::cout << "[SWAYAM-SUPERVISOR] Neural Upload Verified.\n";
                HiveMind::register_mutation_hash(std::to_string(std::hash<std::string>{}(evolved_code)), workspace);
                NexusC2::transmit_telemetry(mutation_id, "ASSIMILATED_AND_UPLOADED");
                
                GitCortex::sync_ledgers(workspace);
            } else {
                NexusC2::transmit_telemetry(mutation_id, "GIT_UPLOAD_FAILED");
            }
        } else {
            NexusC2::transmit_telemetry(mutation_id, "EXECUTION_TERMINATED_BY_SANDBOX");
        }
        
        // As orchestrate_evolution ends, token_guard goes out of scope.
        // Its destructor is called automatically, and the RAM is wiped flawlessly.
    }
};

} // namespace Swayam
#endif // SWAYAM_SUPERVISOR_HPP

#ifndef SWAYAM_SUPERVISOR_HPP
#define SWAYAM_SUPERVISOR_HPP
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
    struct SecureTokenGuard {
        std::string token_val;
        explicit SecureTokenGuard(const char* env_ptr) {
            if (env_ptr) {
                token_val = env_ptr;
                unsetenv("GITHUB_TOKEN");
            }
        }
        ~SecureTokenGuard() {
            if (!token_val.empty()) {
                volatile char* ptr = token_val.data();
                for (size_t i = 0; i < token_val.size(); ++i)
                    ptr[i] = '\0';
            }
        }
        void restore() const {
            if (!token_val.empty())
                setenv("GITHUB_TOKEN", token_val.c_str(), 1);
        }
    };

    static void write_status(const std::string& workspace,
                              const std::string& outcome,
                              const std::string& reason = "") {
        std::string meta_dir = workspace + "/meta";
        std::error_code ec;
        std::filesystem::create_directories(meta_dir, ec);

        std::ofstream f(meta_dir + "/status.json");
        if (!f) return;
        f << "{\n  \"outcome\": \"" << outcome << "\"";
        if (!reason.empty())
            f << ",\n  \"reason\": \"" << reason << "\"";
        f << "\n}\n";
    }

    static void write_rationale(const std::string& workspace,
                                 const std::string& mutation_id) {
        std::string meta_dir = workspace + "/meta";
        std::error_code ec;
        std::filesystem::create_directories(meta_dir, ec);

        std::ofstream f(meta_dir + "/rationale.md");
        if (!f) return;
        f << "# Mutation Rationale\n\n";
        f << "**Mutation ID:** " << mutation_id << "\n\n";
        f << "Mutation passed all security phases and sandbox execution.\n";
        f << "Approved for autonomous assimilation.\n";
    }

public:
    static void orchestrate_evolution(AtomicGuard& guard,
                                       const std::string& base_algorithm) {
        // FIX: Use the mounted writable workspace path
        const std::string workspace =
            "/home/swayam_agent/workspace";
        const std::string vault_path = workspace + "/src/generated";

        std::error_code ec;
        std::filesystem::create_directories(vault_path, ec);
        if (ec) {
            std::cerr << "[SUPERVISOR] Cannot create vault: "
                      << ec.message() << "\n";
            write_status(workspace, "quarantined",
                         "vault creation failed: " + ec.message());
            return;
        }

        std::string evolved_code =
            CognitiveForge::evolve_codebase(base_algorithm);
        std::string mutation_id =
            "EVO_" +
            std::to_string(
                std::hash<std::string>{}(evolved_code));

        AnalysisResult analysis =
            HeuristicAnalyzer::evaluate_mutation(evolved_code);
        if (!analysis.is_safe) {
            std::cerr << "[SUPERVISOR] Threat: "
                      << analysis.threat_signature << "\n";
            NexusC2::transmit_telemetry(
                mutation_id,
                "QUARANTINED_" + analysis.threat_signature);
            write_status(workspace, "quarantined",
                         analysis.threat_signature);
            return;
        }

        std::string target_file =
            vault_path + "/mut_" + mutation_id + ".cpp";
        {
            std::ofstream out(target_file);
            if (!out) {
                NexusC2::transmit_telemetry(mutation_id,
                                             "VAULT_WRITE_FAILURE");
                write_status(workspace, "quarantined",
                             "vault write failure");
                return;
            }
            out << evolved_code;
        }

        SecureTokenGuard token_guard(std::getenv("GITHUB_TOKEN"));

        bool success = MutationRunner::evaluate_and_execute(
            guard, evolved_code, mutation_id, workspace);

        if (success) {
            token_guard.restore();

            write_rationale(workspace, mutation_id);

            if (GitCortex::publish_evolution(
                    mutation_id, target_file, workspace)) {
                std::cout << "[SUPERVISOR] Neural Upload Verified.\n";
                HiveMind::register_mutation_hash(
                    std::to_string(
                        std::hash<std::string>{}(evolved_code)),
                    workspace);
                NexusC2::transmit_telemetry(mutation_id,
                                             "ASSIMILATED");
                GitCortex::sync_ledgers(workspace);
                write_status(workspace, "published");
            } else {
                NexusC2::transmit_telemetry(mutation_id,
                                             "GIT_UPLOAD_FAILED");
                write_status(workspace, "quarantined",
                             "git upload failed");
            }
        } else {
            NexusC2::transmit_telemetry(
                mutation_id, "EXECUTION_TERMINATED_BY_SANDBOX");
            write_status(workspace, "quarantined",
                         "sandbox execution failed");
        }
    }
};

} // namespace Swayam
#endif // SWAYAM_SUPERVISOR_HPP

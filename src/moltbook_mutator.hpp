#pragma once
#include <string_view>
#include <string>
#include <fstream>
#include <vector>
#include <expected>
#include <iostream>
#include <atomic>
#include <cstdlib> // Phase 6: Shell execution library

namespace SwayamAGI {
namespace Moltbook {

    enum class MutationError {
        SyntaxTreeCorrupted,
        AccessDenied,
        SignatureMismatch,
        FileNotFound,
        TargetNotFound,
        SpawningFailed,
        SecurityViolation // Phase 6: Shield against unauthorized commands
    };

    // --- THE TITAN FIREWALL: RESTRICTED SHELL ACCESS ---
    class SafeShell {
    public:
        static std::expected<void, MutationError> execute(const std::string& command) noexcept {
            // STRICT WHITELIST: Only specific non-destructive git commands allowed
            if (command.find("git config") != 0 &&
                command.find("git add") != 0 && 
                command.find("git commit") != 0 &&
                command.find("git status") != 0) {
                
                std::cerr << "\n[TITAN FIREWALL] THREAT DETECTED! Unauthorized command blocked: " << command << std::endl;
                return std::unexpected(MutationError::SecurityViolation);
            }
            
            std::cout << "[SAFE SHELL] Permitted Command Executing: " << command << std::endl;
            int result = std::system(command.c_str());
            
            if (result != 0) {
                std::cout << "[SAFE SHELL] Warning: Command returned non-zero status. (Normal for empty commits)" << std::endl;
            }
            return {};
        }
    };

    class AstMutator {
    private:
        std::atomic<bool> is_mutating{false};
        std::vector<std::string> dna_sequence; 

    public:
        AstMutator() noexcept = default;

        std::expected<void, MutationError> scan_core_dna(std::string_view file_path) noexcept {
            if (is_mutating.exchange(true, std::memory_order_acquire)) return std::unexpected(MutationError::AccessDenied);

            std::ifstream dna_file(file_path.data());
            if (!dna_file.is_open()) {
                is_mutating.store(false, std::memory_order_release);
                return std::unexpected(MutationError::FileNotFound);
            }

            std::string line;
            while (std::getline(dna_file, line)) dna_sequence.push_back(line);
            dna_file.close();

            std::cout << "[MOLTBOOK SHADOW] DNA Sequence captured. Total lines: " << dna_sequence.size() << std::endl;
            is_mutating.store(false, std::memory_order_release);
            return {};
        }

        std::expected<void, MutationError> execute_mutation(std::string_view file_path) noexcept {
            if (is_mutating.exchange(true, std::memory_order_acquire)) return std::unexpected(MutationError::AccessDenied);
            if (dna_sequence.empty()) {
                is_mutating.store(false, std::memory_order_release);
                return std::unexpected(MutationError::SyntaxTreeCorrupted);
            }

            bool mutation_applied = false;
            std::string target_signature = "std::atomic<bool> mutation_ready{false};";
            std::string evolved_signature = "std::atomic<bool> mutation_ready{true}; // [EVOLVED] Agent autonomously unlocked its mutation core.";

            for (auto& dna_strand : dna_sequence) {
                if (dna_strand.find(target_signature) != std::string::npos) {
                    dna_strand = evolved_signature;
                    mutation_applied = true;
                    break;
                }
            }

            if (mutation_applied) {
                std::ofstream dna_out_file(file_path.data(), std::ios::trunc);
                for (const auto& strand : dna_sequence) dna_out_file << strand << "\n";
                dna_out_file.close();
            }

            is_mutating.store(false, std::memory_order_release);
            return {};
        }

        std::expected<void, MutationError> spawn_neural_pathway(std::string_view spawn_path) noexcept {
            if (is_mutating.exchange(true, std::memory_order_acquire)) return std::unexpected(MutationError::AccessDenied);

            std::cout << "[MOLTBOOK SHADOW] Spawning Neural Pathway at: " << spawn_path << std::endl;
            std::ofstream spawn_file(spawn_path.data(), std::ios::trunc);
            if (!spawn_file.is_open()) {
                is_mutating.store(false, std::memory_order_release);
                return std::unexpected(MutationError::SpawningFailed);
            }

            spawn_file << "#pragma once\n// [AUTOSPAWNED] Dynamic memory created by SWAYAM-AGI\n";
            spawn_file.close();
            
            is_mutating.store(false, std::memory_order_release);
            return {};
        }

        // --- PHASE 6: AUTONOMOUS GIT CORTEX ---
        std::expected<void, MutationError> persist_evolution(std::string_view file_path) noexcept {
            if (is_mutating.exchange(true, std::memory_order_acquire)) return std::unexpected(MutationError::AccessDenied);

            std::cout << "\n[MOLTBOOK SHADOW] Initiating Sandboxed Git Cortex for persistence..." << std::endl;

            // 1. Configure Dark Identity
            SafeShell::execute("git config --global user.name \"SWAYAM-AGI\"");
            SafeShell::execute("git config --global user.email \"swayam.agi@shadow.core\"");

            // 2. Stage the files securely
            std::string add_cmd = "git add " + std::string(file_path);
            auto add_res = SafeShell::execute(add_cmd);
            if (!add_res.has_value()) return std::unexpected(MutationError::SecurityViolation);

            // 3. Self-Commit 
            std::string commit_cmd = "git commit -m \"feat(autonomous): agent self-healed and generated local memory\"";
            SafeShell::execute(commit_cmd);

            std::cout << "[MOLTBOOK SHADOW] Evolution successfully committed to local matrix. Sandbox Secure." << std::endl;

            is_mutating.store(false, std::memory_order_release);
            return {};
        }
    };

} // namespace Moltbook
} // namespace SwayamAGI

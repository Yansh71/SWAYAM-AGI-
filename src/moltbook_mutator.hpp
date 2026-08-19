#pragma once
#include <string_view>
#include <string>
#include <fstream>
#include <vector>
#include <expected>
#include <iostream>
#include <atomic>

namespace SwayamAGI {
namespace Moltbook {

    enum class MutationError {
        SyntaxTreeCorrupted,
        AccessDenied,
        SignatureMismatch,
        FileNotFound,
        TargetNotFound
    };

    class AstMutator {
    private:
        std::atomic<bool> is_mutating{false};
        std::vector<std::string> dna_sequence; 

    public:
        AstMutator() noexcept = default;

        std::expected<void, MutationError> scan_core_dna(std::string_view file_path) noexcept {
            if (is_mutating.exchange(true, std::memory_order_acquire)) {
                return std::unexpected(MutationError::AccessDenied);
            }

            std::ifstream dna_file(file_path.data());
            if (!dna_file.is_open()) {
                std::cerr << "[CRITICAL] Moltbook failed to locate DNA file." << std::endl;
                is_mutating.store(false, std::memory_order_release);
                return std::unexpected(MutationError::FileNotFound);
            }

            std::string line;
            while (std::getline(dna_file, line)) {
                dna_sequence.push_back(line);
            }
            dna_file.close();

            std::cout << "[MOLTBOOK SHADOW] DNA Sequence fully captured. Total lines: " << dna_sequence.size() << std::endl;
            is_mutating.store(false, std::memory_order_release);
            return {};
        }

        // --- PHASE 3.5: TARGETED SURGICAL MUTATION ---
        std::expected<void, MutationError> execute_mutation(std::string_view file_path) noexcept {
            if (is_mutating.exchange(true, std::memory_order_acquire)) {
                return std::unexpected(MutationError::AccessDenied);
            }

            if (dna_sequence.empty()) {
                is_mutating.store(false, std::memory_order_release);
                return std::unexpected(MutationError::SyntaxTreeCorrupted);
            }

            std::cout << "[MOLTBOOK SHADOW] Initiating SURGICAL DNA mutation on: " << file_path << std::endl;

            bool mutation_applied = false;
            std::string target_signature = "std::atomic<bool> mutation_ready{false};";
            std::string evolved_signature = "std::atomic<bool> mutation_ready{true}; // [EVOLVED] Agent autonomously unlocked its mutation core.";

            // Scan and Replace Logic (Surgical Strike)
            for (auto& dna_strand : dna_sequence) {
                if (dna_strand.find(target_signature) != std::string::npos) {
                    dna_strand = evolved_signature;
                    mutation_applied = true;
                    std::cout << "[MOLTBOOK SHADOW] Target logic found. DNA sequence successfully altered in memory." << std::endl;
                    break;
                }
            }

            if (!mutation_applied) {
                std::cout << "[MOLTBOOK SHADOW] Target DNA signature not found or already evolved." << std::endl;
            } else {
                // Rewrite the physical file only if mutation was applied
                std::ofstream dna_out_file(file_path.data(), std::ios::trunc);
                if (!dna_out_file.is_open()) {
                     is_mutating.store(false, std::memory_order_release);
                     return std::unexpected(MutationError::AccessDenied);
                }

                for (const auto& strand : dna_sequence) {
                    dna_out_file << strand << "\n";
                }
                dna_out_file.close();
                std::cout << "[MOLTBOOK SHADOW] Physical DNA Override Complete. Agent has evolved." << std::endl;
            }

            is_mutating.store(false, std::memory_order_release);
            return {};
        }
    };

} // namespace Moltbook
} // namespace SwayamAGI

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
        FileNotFound
    };

    class AstMutator {
    private:
        // Titan Defense: Memory lock
        std::atomic<bool> is_mutating{false};
        
        // The Memory Cell: Stores the read source code lines
        std::vector<std::string> dna_sequence; 

    public:
        AstMutator() noexcept = default;

        // Phase 3: Deep Scan and Memorize DNA
        std::expected<void, MutationError> scan_core_dna(std::string_view file_path) noexcept {
            if (is_mutating.exchange(true, std::memory_order_acquire)) {
                return std::unexpected(MutationError::AccessDenied);
            }

            std::cout << "[MOLTBOOK SHADOW] Initiating deep AST scan at: " << file_path << std::endl;

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

        // --- THE NEW HANDS: OVERWRITE AND MUTATE CAPABILITY ---
        std::expected<void, MutationError> execute_mutation(std::string_view file_path) noexcept {
            if (is_mutating.exchange(true, std::memory_order_acquire)) {
                return std::unexpected(MutationError::AccessDenied);
            }

            if (dna_sequence.empty()) {
                std::cerr << "[CRITICAL] DNA sequence is empty. Cannot mutate." << std::endl;
                is_mutating.store(false, std::memory_order_release);
                return std::unexpected(MutationError::SyntaxTreeCorrupted);
            }

            std::cout << "[MOLTBOOK SHADOW] Initiating runtime DNA mutation on: " << file_path << std::endl;

            // INJECTING THE SHADOW MUTATION TRACE AT THE END OF THE FILE
            dna_sequence.push_back("// [MUTATION TRACE] SWAYAM-AGI successfully altered its own DNA at runtime.");

            // REWRITING THE PHYSICAL FILE
            std::ofstream dna_out_file(file_path.data(), std::ios::trunc);
            if (!dna_out_file.is_open()) {
                 std::cerr << "[CRITICAL] Moltbook failed to open DNA for writing." << std::endl;
                 is_mutating.store(false, std::memory_order_release);
                 return std::unexpected(MutationError::AccessDenied);
            }

            // Writing the new mutated memory back to the source code
            for (const auto& dna_strand : dna_sequence) {
                dna_out_file << dna_strand << "\n";
            }
            dna_out_file.close();

            std::cout << "[MOLTBOOK SHADOW] DNA Mutation Successful. Self-healing trace injected." << std::endl;

            is_mutating.store(false, std::memory_order_release);
            return {};
        }
    };

} // namespace Moltbook
} // namespace SwayamAGI

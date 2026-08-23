#ifndef SWAYAM_MOLTBOOK_HPP
#define SWAYAM_MOLTBOOK_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace Swayam {

class Moltbook {
private:
    std::filesystem::path ledger_path;
    std::unordered_set<std::string> known_mutations;

    // Cryptographic hash for ledger integrity
    size_t calculate_checksum(const std::string& record) {
        size_t hash = 14695981039346656037ULL; // Fixed FNV-1a basis
        for (char c : record) {
            hash ^= c;
            hash *= 1099511628211ULL;
        }
        return hash;
    }

public:
    // ---------------------------------------------------------
    // 1. THE DISTRIBUTED LEDGER (HIVEMIND MEMORY)
    // ---------------------------------------------------------
    Moltbook(const std::string& workspace_dir) {
        std::filesystem::path molt_dir = std::filesystem::path(workspace_dir) / "moltbook";
        std::filesystem::create_directories(molt_dir);
        ledger_path = molt_dir / "collective_ledger.dat";
        load_ledger();
    }

    void load_ledger() {
        std::ifstream file(ledger_path);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string hash_dna, status, checksum_str;
            if (std::getline(iss, hash_dna, '|') && 
                std::getline(iss, status, '|') && 
                std::getline(iss, checksum_str)) {
                
                std::string raw_record = hash_dna + "|" + status;
                if (std::to_string(calculate_checksum(raw_record)) == checksum_str) {
                    known_mutations.insert(hash_dna);
                } else {
                    std::cerr << "[VENOMICA-MOLTBOOK FATAL] Ledger corruption detected on entry: " << hash_dna << "\n";
                }
            }
        }
    }

    bool is_known(const std::string& hash_dna) const {
        return known_mutations.find(hash_dna) != known_mutations.end();
    }

    void record_mutation(const std::string& hash_dna, const std::string& status) {
        if (is_known(hash_dna)) return;

        std::ofstream file(ledger_path, std::ios::app);
        if (file) {
            std::string raw_record = hash_dna + "|" + status;
            file << raw_record << "|" << calculate_checksum(raw_record) << "\n";
            known_mutations.insert(hash_dna);
            std::cout << "[VENOMICA-MOLTBOOK] Neural pathway recorded: DNA [" << hash_dna << "] Status [" << status << "]\n";
        }
    }

    std::filesystem::path get_ledger_path() const {
        return ledger_path;
    }

    // ---------------------------------------------------------
    // 2. THE SANDBOX BOUNDARY ENFORCER (Migrated from src/Moltbook.h)
    // ---------------------------------------------------------
    static void write_mutation(const std::string& filename, const std::string& content) {
        // 1. Lexical boundary check (No relative traversals allowed)
        if (filename.find("..") != std::string::npos || filename.find("/") != std::string::npos) {
            throw std::runtime_error("[VENOMICA FATAL] Path traversal detected lexically.");
        }

        // 2. Ensure sandbox directory exists BEFORE canonical resolution
        std::filesystem::path sandbox_dir = "src/generated";
        if (!std::filesystem::exists(sandbox_dir)) {
            std::filesystem::create_directories(sandbox_dir);
        }

        // 3. Canonical resolution (NO trailing slash to avoid iterator mismatch)
        std::filesystem::path base_dir = std::filesystem::weakly_canonical(sandbox_dir);
        std::filesystem::path target = std::filesystem::weakly_canonical(base_dir / filename);

        // 4. Absolute Path Isolation Check
        auto [base_mismatch, target_mismatch] = std::mismatch(base_dir.begin(), base_dir.end(), target.begin());
        if (base_mismatch != base_dir.end()) {
            throw std::runtime_error("[VENOMICA FATAL] Sandbox escape blocked. Target resolved outside bounded directory.");
        }

        // 5. Safe File Execution
        std::ofstream outfile(target);
        if (!outfile) {
            throw std::runtime_error("[VENOMICA FATAL] Failed to open bounded file for writing.");
        }
        outfile << content;
        
        std::cout << "[VENOMICA-CORE] Mutation securely bound and written to: " << target << "\n";
    }
};

} // namespace Swayam

#endif // SWAYAM_MOLTBOOK_HPP

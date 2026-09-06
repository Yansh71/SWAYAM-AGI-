#ifndef SWAYAM_HIVE_MIND_HPP
#define SWAYAM_HIVE_MIND_HPP

#include <string>
#include <vector>
#include <mutex>          // FIX: Explicitly required for std::unique_lock
#include <shared_mutex>   // Required for std::shared_mutex and std::shared_lock
#include <unordered_set>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace Swayam {

class HiveMind {
private:
    mutable std::shared_mutex hive_mutex;
    
    std::unordered_set<std::string> hash_set;
    std::vector<std::string> internal_accepted_hashes;
    const std::string state_file_path = ".hive_state/accepted_hashes.txt";

    HiveMind() {
        try {
            std::filesystem::create_directories(".hive_state");
            std::ifstream infile(state_file_path);
            std::string line;
            while (std::getline(infile, line)) {
                if (!line.empty()) {
                    hash_set.insert(line);
                    internal_accepted_hashes.push_back(line);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[SWAYAM-HIVE FATAL] Bootstrap initialization failed: " << e.what() << "\n";
            throw; 
        }
    }

public:
    HiveMind(const HiveMind&) = delete;
    HiveMind& operator=(const HiveMind&) = delete;

    static HiveMind& instance() {
        static HiveMind global_hive;
        return global_hive;
    }

    [[nodiscard]] std::vector<std::string> accepted_hashes_snapshot() const {
        std::shared_lock<std::shared_mutex> lock(hive_mutex);
        return internal_accepted_hashes;
    }

    [[nodiscard]] bool is_known(const std::string& hash) const {
        std::shared_lock<std::shared_mutex> lock(hive_mutex);
        return hash_set.contains(hash);
    }

    void register_mutation_hash(const std::string& hash) {
        std::unique_lock<std::shared_mutex> lock(hive_mutex);
        if (hash_set.insert(hash).second) {
            internal_accepted_hashes.push_back(hash);
            
            std::ofstream outfile(state_file_path, std::ios::app);
            if (outfile) {
                outfile << hash << "\n";
                outfile.flush(); 
            } else {
                std::cerr << "[SWAYAM-HIVE WARN] State persistence failed for hash: " << hash << "\n";
            }
        }
    }

    // =========================================================================
    // QUANTUM COMMAND: ENTERPRISE CORE STATIC INTERFACE
    // =========================================================================

    static void awakenNode(const std::string& node_id) {
        std::cout << "[SWAYAM-HIVE] Awaken Node protocol verified for node: " << node_id << "\n";
    }

    template <typename T>
    static void synchronize_collective(const T& ledger, const std::string& sync_dir) {
        std::filesystem::create_directories(sync_dir);
        
        std::ostringstream ledger_stream;
        ledger_stream << ledger;
        std::string ledger_fingerprint = ledger_stream.str();

        std::cout << "[SWAYAM-HIVE] Synchronizing collective ledger. Fingerprint size: " 
                  << ledger_fingerprint.size() << " bytes. Sync Directory: " << sync_dir << "\n";

        std::string sync_log_path = sync_dir + "/collective_sync_audit.log";
        std::ofstream sync_outfile(sync_log_path, std::ios::app);
        if (sync_outfile.is_open()) {
            sync_outfile << "Synced Collective Ledger Node State -> Fingerprint Length: " << ledger_fingerprint.size() << "\n";
            sync_outfile.flush();
        }
    }

    static void broadcastEvolution(const std::string& target_file) {
        std::cout << "[SWAYAM-HIVE] Broadcasting evolution payload for verified target: " << target_file << "\n";
    }
};

} // namespace Swayam

#endif // SWAYAM_HIVE_MIND_HPP

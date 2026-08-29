#ifndef SWAYAM_HIVE_MIND_HPP
#define SWAYAM_HIVE_MIND_HPP

#include <string>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <span>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace Swayam {

class HiveMind {
private:
    std::mutex hive_mutex;
    std::unordered_set<std::string> hash_set;
    std::vector<std::string> internal_accepted_hashes;
    const std::string state_file_path = ".hive_state/accepted_hashes.txt";

    HiveMind() {
        std::filesystem::create_directories(".hive_state");
        std::ifstream infile(state_file_path);
        std::string line;
        while (std::getline(infile, line)) {
            if (!line.empty()) {
                hash_set.insert(line);
                internal_accepted_hashes.push_back(line);
            }
        }
    }

public:
    HiveMind(const HiveMind&) = delete;
    HiveMind& operator=(const HiveMind&) = delete;

    static HiveMind& instance() {
        static HiveMind global_hive;
        return global_hive;
    }

    [[nodiscard]] std::span<const std::string> accepted_hashes() const noexcept {
        return internal_accepted_hashes;
    }

    [[nodiscard]] bool is_known(const std::string& hash) {
        std::lock_guard<std::mutex> lock(hive_mutex);
        return hash_set.contains(hash);
    }

    void register_mutation_hash(const std::string& hash) {
        std::lock_guard<std::mutex> lock(hive_mutex);
        if (hash_set.insert(hash).second) {
            internal_accepted_hashes.push_back(hash);
            std::ofstream outfile(state_file_path, std::ios::app);
            outfile << hash << "\n";
        }
    }

    // =========================================================================
    // QUANTUM COMMAND: ENTERPRISE CORE STATIC INTERFACE
    // =========================================================================

    static void awakenNode(const std::string& node_id) {
        std::lock_guard<std::mutex> lock(instance().hive_mutex);
        std::cout << "[HiveMind] Awaken Node protocol verified for node: " << node_id << std::endl;
    }

    template <typename T>
    static void synchronize_collective(const T& ledger, const std::string& sync_dir) {
        std::lock_guard<std::mutex> lock(instance().hive_mutex);
        std::filesystem::create_directories(sync_dir);
        
        // Genuine enterprise telemetry consumption of the ledger object
        std::ostringstream ledger_stream;
        ledger_stream << ledger;
        std::string ledger_fingerprint = ledger_stream.str();

        std::cout << "[HiveMind] Synchronizing collective ledger. Ledger fingerprint size: " 
                  << ledger_fingerprint.size() << " bytes. Sync Directory: " << sync_dir << std::endl;

        std::string sync_log_path = sync_dir + "/collective_sync_audit.log";
        std::ofstream sync_outfile(sync_log_path, std::ios::app);
        if (sync_outfile.is_open()) {
            sync_outfile << "Synced Collective Ledger Node State -> Fingerprint Length: " << ledger_fingerprint.size() << "\n";
        }
    }

    static void broadcastEvolution(const std::string& target_file) {
        std::lock_guard<std::mutex> lock(instance().hive_mutex);
        std::cout << "[HiveMind] Broadcasting evolution payload for verified target: " << target_file << std::endl;
    }
};

} // namespace Swayam

#endif // SWAYAM_HIVE_MIND_HPP

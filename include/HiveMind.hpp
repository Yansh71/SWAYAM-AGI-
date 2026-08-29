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

    // --- নোড ম্যানেজমেন্ট স্ট্যাটিক মেথডসমূহ ---

    static void awakenNode(const std::string& node_id) {
        std::lock_guard<std::mutex> lock(instance().hive_mutex);
        std::cout << "[HiveMind] Awaken Node protocol initialized for: " << node_id << std::endl;
    }

    // Unused parameter error রোধ করতে [[maybe_unused]] অ্যাট্রিবিউট যুক্ত করা হয়েছে
    template <typename T>
    static void synchronize_collective([[maybe_unused]] const T& ledger, const std::string& sync_dir) {
        std::lock_guard<std::mutex> lock(instance().hive_mutex);
        std::filesystem::create_directories(sync_dir);
        std::cout << "[HiveMind] Synchronizing collective ledger. Sync Directory: " << sync_dir << std::endl;
    }

    static void broadcastEvolution(const std::string& target_file) {
        std::lock_guard<std::mutex> lock(instance().hive_mutex);
        std::cout << "[HiveMind] Broadcasting evolution payload for target: " << target_file << std::endl;
    }
};

} // namespace Swayam

#endif // SWAYAM_HIVE_MIND_HPP

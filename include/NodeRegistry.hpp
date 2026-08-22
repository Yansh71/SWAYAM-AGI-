#ifndef SWAYAM_NODE_REGISTRY_HPP
#define SWAYAM_NODE_REGISTRY_HPP

// SWAYAM-AGI Phase 4: The Hive Mind
// NodeRegistry - Decentralized signature sharing with zero new
// attack surface. Nodes never speak to each other directly.
// All coordination routes through the GitHub Contents API -
// the same trust plane already used by cas_increment.sh.
// A compromised peer can only inject hash strings into the
// quarantine. Worst-case outcome = a valid mutation is blocked.
// That is a safe-fail. It cannot execute arbitrary code here.

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <unistd.h>

namespace Swayam {

// derive_node_id()
// Stable across restarts on the same host, unique across hosts.
// Combines hostname + a seed file written on first run.
inline std::string derive_node_id() {
    char buf[256] = {0};
    gethostname(buf, sizeof(buf));
    std::string hostname(buf);

    const std::string seed_path = "/tmp/.swayam_node_seed";
    std::string seed;
    
    std::ifstream sf(seed_path);
    if (sf) {
        std::getline(sf, seed);
    } else {
        seed = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::ofstream out(seed_path);
        out << seed;
    }

    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : hostname + seed) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return "node-" + std::to_string(h);
}

// merge_peer_signatures()
// Reads a peer CSV downloaded by hive_sync.sh and merges its
// quarantine hashes into the local quarantine log.
// Zero-trust rules applied:
// - Only pure decimal uint64 hash strings accepted.
// - Non-matching lines silently dropped (no crash on bad input).
// - Duplicates skipped.
// - Peer signal/timestamp replaced with local values so a
//   peer cannot inject false metadata.
inline void merge_peer_signatures(const std::string& peer_csv_path,
                                  const std::string& local_quarantine_path) {
    std::ifstream peer(peer_csv_path);
    if (!peer) return;

    std::vector<std::string> existing;
    {
        std::ifstream local(local_quarantine_path);
        std::string line;
        while (std::getline(local, line)) {
            auto comma = line.find(',');
            if (comma != std::string::npos) {
                existing.push_back(line.substr(0, comma));
            }
        }
    }

    std::ofstream out(local_quarantine_path, std::ios::app);
    if (!out) {
        std::cerr << "[HIVE] Cannot open local quarantine for merge: "
                  << local_quarantine_path << "\n";
        return;
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string line;
    int merged = 0;

    while (std::getline(peer, line)) {
        auto comma = line.find(',');
        if (comma == std::string::npos) continue;

        std::string hash = line.substr(0, comma);
        if (hash.empty() || hash.size() > 20) continue;
        if (hash.find_first_not_of("0123456789") != std::string::npos) continue;

        bool found = false;
        for (const auto& e : existing) {
            if (e == hash) { found = true; break; }
        }
        if (found) continue;

        // signal=0 means "imported from peer", not a local crash signal
        out << hash << ",0," << now << ",peer:" << peer_csv_path << "\n";
        existing.push_back(hash);
        ++merged;
    }

    if (merged > 0) {
        std::cout << "[HIVE] Merged " << merged 
                  << " peer signatures into local quarantine.\n";
    }
}

// export_node_signatures()
// Copies local quarantine log to an export path for hive_sync.sh
// to pick up and push to GitHub.
inline void export_node_signatures(const std::string& local_quarantine_path,
                                   const std::string& export_path) {
    std::ifstream in(local_quarantine_path);
    if (!in) return;

    std::ofstream out(export_path, std::ios::trunc);
    if (!out) {
        std::cerr << "[HIVE] Cannot write signature export to " 
                  << export_path << "\n";
        return;
    }
    
    out << in.rdbuf();
    std::cout << "[HIVE] Node signatures exported to " << export_path << "\n";
}

// merge_all_peer_files()
// Scans /tmp/ for peer_*.csv files placed by hive_sync.sh
// and merges each into the local quarantine.
inline void merge_all_peer_files(const std::string& local_quarantine_path) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/tmp/")) {
            const std::string name = entry.path().filename().string();
            if (name.rfind("peer_", 0) == 0 && 
                name.size() > 9 && 
                name.substr(name.size() - 4) == ".csv") {
                merge_peer_signatures(entry.path().string(), local_quarantine_path);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[HIVE] Directory scan error: " << e.what() << "\n";
    }
}

} // namespace Swayam
#endif // SWAYAM_NODE_REGISTRY_HPP

#ifndef SWAYAM_NODE_REGISTRY_HPP
#define SWAYAM_NODE_REGISTRY_HPP
// =============================================================
// SWAYAM NodeRegistry.hpp — Hardening Round 4
//
// FIX-4  Hive Mind DoS Defense via PeerImportPolicy.
//
// Previous vulnerability:
//   merge_peer_signatures() accepted any number of valid-format
//   uint64_t hashes from a peer file. A malicious or misconfigured
//   node could write thousands of hashes (including hashes of
//   mutations never attempted locally), flooding the quarantine
//   ledger and blocking legitimate mutations indefinitely.
//   This is a safe-fail (no unsafe code executed) but a
//   denial-of-service against the mutation engine's productivity.
//
// Mitigations applied:
//   1. MAX_HASHES_PER_FILE: peer CSV files exceeding this line
//      count are rejected entirely before any line is parsed.
//   2. MAX_IMPORTS_PER_SESSION: total cross-peer imports are
//      capped per sync cycle. Once reached, remaining peer files
//      are skipped with a warning.
//   3. Per-peer import counter: each peer is independently
//      limited; a single flooding peer cannot exhaust the budget
//      for other legitimate peers.
//   4. Hash length validation: unchanged (1-20 digits for
//      uint64_t). Only decimal digit strings are accepted.
//      Non-matching lines are silently dropped.
//
// These limits are configurable via PeerImportPolicy so the
// operator can tune them without recompiling core files.
// =============================================================
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

// ---------------------------------------------------------------
// PeerImportPolicy: tunable DoS defense parameters.
//
// Conservative defaults chosen for a CI environment where each
// node generates at most hundreds of mutations per run.
// Increase limits for long-running production deployments.
// ---------------------------------------------------------------
struct PeerImportPolicy {
    // Reject a peer CSV file outright if it contains more lines
    // than this. Prevents parsing of intentionally large files.
    size_t max_hashes_per_file{500};

    // Maximum total hashes imported from ALL peers in one
    // sync cycle. Once reached, remaining peer files are skipped.
    size_t max_imports_per_session{2000};

    // Maximum hashes accepted from a SINGLE peer per sync cycle.
    // Even if a peer file is within max_hashes_per_file,
    // per-peer budget ensures one peer cannot consume the full
    // session budget alone.
    size_t max_imports_per_peer{300};
};

// ---------------------------------------------------------------
// derive_node_id(): stable across restarts on the same host.
// Combines hostname + a seed file written on first run.
// ---------------------------------------------------------------
inline std::string derive_node_id() {
    char buf[256] = {};
    gethostname(buf, sizeof(buf));
    std::string hostname(buf);

    const std::string seed_path = "/tmp/.swayam_node_seed";
    std::string seed;
    {
        std::ifstream sf(seed_path);
        if (sf) {
            std::getline(sf, seed);
        } else {
            seed = std::to_string(
                std::chrono::system_clock::now()
                    .time_since_epoch().count());
            std::ofstream out(seed_path);
            out << seed;
        }
    }
    // FNV-1a with corrected basis
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : hostname + seed) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return "node-" + std::to_string(h);
}

// ---------------------------------------------------------------
// merge_peer_signatures_guarded()
//
// Merges quarantine hashes from a single peer CSV file into the
// local quarantine log, with DoS defense enforced by policy.
//
// Returns the number of hashes actually imported (0 on rejection).
//
// Parameters:
//   peer_csv_path       — path to downloaded peer CSV
//   local_quarantine    — path to local quarantine.log
//   session_imported    — running total across all peers this
//                         session (read+write; caller maintains)
//   policy              — import limits
// ---------------------------------------------------------------
inline size_t merge_peer_signatures_guarded(
    const std::string&      peer_csv_path,
    const std::string&      local_quarantine,
    size_t&                 session_imported,
    const PeerImportPolicy& policy = {}) {

    // DoS Gate 1: session budget exhausted
    if (session_imported >= policy.max_imports_per_session) {
        std::cerr << "[HIVE DoS] Session import limit ("
                  << policy.max_imports_per_session
                  << ") reached. Skipping: " << peer_csv_path << "\n";
        return 0;
    }

    std::ifstream peer_file(peer_csv_path);
    if (!peer_file) return 0;

    // DoS Gate 2: pre-count lines before parsing
    size_t total_lines = 0;
    {
        std::string line;
        while (std::getline(peer_file, line)) ++total_lines;
    }
    peer_file.clear();
    peer_file.seekg(0);

    if (total_lines > policy.max_hashes_per_file) {
        std::cerr << "[HIVE DoS] Peer file '" << peer_csv_path
                  << "' contains " << total_lines
                  << " lines (limit: " << policy.max_hashes_per_file
                  << "). Rejecting entire file — possible hash flooding.\n";
        return 0;
    }

    // Load hashes already present locally to avoid duplicates
    std::vector<std::string> existing;
    {
        std::ifstream lf(local_quarantine);
        std::string line;
        while (std::getline(lf, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto comma = line.find(',');
            if (comma != std::string::npos)
                existing.push_back(line.substr(0, comma));
        }
    }

    auto now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());

    std::ofstream out(local_quarantine, std::ios::app);
    if (!out) {
        std::cerr << "[HIVE] Cannot open local quarantine for merge: "
                  << local_quarantine << "\n";
        return 0;
    }

    std::string line;
    size_t peer_imported    = 0;
    size_t peer_rejected    = 0;
    size_t peer_duplicates  = 0;

    while (std::getline(peer_file, line)) {
        if (line.empty() || line[0] == '#') continue;

        auto comma = line.find(',');
        if (comma == std::string::npos) { ++peer_rejected; continue; }
        std::string hash = line.substr(0, comma);

        // Validation: must be pure decimal, length 1–20
        if (hash.empty() || hash.size() > 20) {
            ++peer_rejected; continue;
        }
        if (hash.find_first_not_of("0123456789") != std::string::npos) {
            ++peer_rejected; continue;
        }

        // Dedup check
        bool dup = false;
        for (const auto& e : existing) {
            if (e == hash) { dup = true; break; }
        }
        if (dup) { ++peer_duplicates; continue; }

        // DoS Gate 3: per-peer budget
        if (peer_imported >= policy.max_imports_per_peer) {
            std::cerr << "[HIVE DoS] Per-peer limit ("
                      << policy.max_imports_per_peer
                      << ") reached for: " << peer_csv_path
                      << ". Remaining entries dropped.\n";
            break;
        }

        // DoS Gate 4: session budget (re-check inside loop)
        if (session_imported >= policy.max_imports_per_session) {
            std::cerr << "[HIVE DoS] Session limit reached mid-peer. "
                         "Stopping import.\n";
            break;
        }

        // signal=0: imported from peer (not a local crash)
        out << hash << ",0," << now
            << ",peer:" << peer_csv_path << "\n";
        existing.push_back(hash);
        ++peer_imported;
        ++session_imported;
    }

    if (peer_imported > 0 || peer_rejected > 0) {
        std::cout << "[HIVE] Peer '" << peer_csv_path << "': "
                  << peer_imported << " imported, "
                  << peer_duplicates << " duplicates skipped, "
                  << peer_rejected << " invalid lines dropped.\n";
    }
    return peer_imported;
}

// ---------------------------------------------------------------
// export_node_signatures(): copies local quarantine to an export
// path for hive_sync.sh to push to GitHub.
// ---------------------------------------------------------------
inline void export_node_signatures(
    const std::string& local_quarantine,
    const std::string& export_path) {

    std::ifstream in(local_quarantine);
    if (!in) return;
    std::ofstream out(export_path, std::ios::trunc);
    if (!out) {
        std::cerr << "[HIVE] Cannot write export to "
                  << export_path << "\n";
        return;
    }
    out << in.rdbuf();
    std::cout << "[HIVE] Node signatures exported to "
              << export_path << "\n";
}

// ---------------------------------------------------------------
// merge_all_peer_files(): scans /tmp for peer_*.csv files placed
// by hive_sync.sh and merges each with DoS defense applied.
// Uses a shared session counter across all peers.
// ---------------------------------------------------------------
inline void merge_all_peer_files(
    const std::string&      local_quarantine,
    const PeerImportPolicy& policy = {}) {

    size_t session_imported = 0;
    size_t files_processed  = 0;

    try {
        for (const auto& entry :
             std::filesystem::directory_iterator("/tmp/")) {
            const std::string name =
                entry.path().filename().string();
            if (name.rfind("peer_", 0) != 0) continue;
            if (name.size() < 10)            continue;
            if (name.substr(name.size() - 4) != ".csv") continue;

            merge_peer_signatures_guarded(
                entry.path().string(),
                local_quarantine,
                session_imported,
                policy);
            ++files_processed;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[HIVE] Directory scan error: "
                  << e.what() << "\n";
    }

    if (files_processed > 0) {
        std::cout << "[HIVE] Sync complete: "
                  << files_processed << " peer file(s) processed, "
                  << session_imported << " total hashes imported"
                  << " (session limit: "
                  << policy.max_imports_per_session << ").\n";
    }
}

} // namespace Swayam
#endif // SWAYAM_NODE_REGISTRY_HPP

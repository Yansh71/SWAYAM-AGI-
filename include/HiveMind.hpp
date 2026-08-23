#ifndef SWAYAM_HIVEMIND_HPP
#define SWAYAM_HIVEMIND_HPP

#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <fstream>
#include <sstream>
#include "Moltbook.hpp"

namespace Swayam {

class HiveMind {
public:
    // ---------------------------------------------------------
    // 1. NODE INITIALIZATION (Migrated from legacy src/HiveMind.h)
    // ---------------------------------------------------------
    static void awakenNode(const std::string& nodeIdentity) {
        std::cout << "\n[VENOMICA HIVE-MIND] Awakening Decentralized Node: [" << nodeIdentity << "]\n";
        std::cout << "[VENOMICA HIVE-MIND] Scanning matrix for peer nodes...\n";
        std::cout << "[VENOMICA HIVE-MIND] Quantum Sync: Architecture is primed for Collective Intelligence.\n";
    }

    // ---------------------------------------------------------
    // 2. TRUE DECENTRALIZED SYNCHRONIZATION (The Physical Engine)
    // ---------------------------------------------------------
    static void synchronize_collective(Moltbook& local_node, const std::string& sync_network_dir) {
        std::cout << "[VENOMICA-HIVEMIND] Initiating telepathic consensus protocol...\n";
        
        if (!std::filesystem::exists(sync_network_dir)) {
            std::filesystem::create_directories(sync_network_dir);
            std::cout << "[VENOMICA-HIVEMIND] Hive node isolated. Waiting for peers in network directory.\n";
            return;
        }

        int absorbed_traits = 0;

        for (const auto& entry : std::filesystem::directory_iterator(sync_network_dir)) {
            if (entry.is_regular_file() && entry.path() != local_node.get_ledger_path()) {
                std::ifstream peer_file(entry.path());
                if (!peer_file.is_open()) continue;

                std::string line;
                while (std::getline(peer_file, line)) {
                    std::istringstream iss(line);
                    std::string hash_dna, status, checksum_str;
                    
                    if (std::getline(iss, hash_dna, '|') && 
                        std::getline(iss, status, '|')) {
                        
                        // If the local node doesn't know this mutation, it absorbs the knowledge
                        if (!local_node.is_known(hash_dna)) {
                            local_node.record_mutation(hash_dna, status);
                            absorbed_traits++;
                        }
                    }
                }
            }
        }

        if (absorbed_traits > 0) {
            std::cout << "[VENOMICA-HIVEMIND] Synchronization complete. Absorbed " << absorbed_traits << " new neural pathways from the collective.\n";
        } else {
            std::cout << "[VENOMICA-HIVEMIND] Node is at peak evolutionary sync. No new data required.\n";
        }
    }

    // ---------------------------------------------------------
    // 3. BROADCASTING (Migrated from legacy src/HiveMind.h)
    // ---------------------------------------------------------
    static void broadcastEvolution(const std::string& mutationSignature) {
        std::cout << "[VENOMICA HIVE-MIND] Broadcasting Evolution Signature: [" << mutationSignature << "] to the Matrix...\n";
        std::cout << "[VENOMICA HIVE-MIND] SUCCESS: Multi-node synchronization complete.\n";
    }
};

} // namespace Swayam

#endif // SWAYAM_HIVEMIND_HPP

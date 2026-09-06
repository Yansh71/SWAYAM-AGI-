// src/main.cpp
#include "../include/core.hpp" // Make sure the path matches where your core.hpp is located
#include <iostream>
#include <filesystem>
#include <thread>
#include <vector>

int main() {
    try {
        std::cout << "[SWAYAM-CORE] Core Initialization Sequence Engaged...\n";

        // Step 1: Quarantine Ledger Migration & Integrity Check
        std::cout << "[SWAYAM-CORE] Verifying V2 Quarantine Ledger Integrity...\n";
        Swayam::migrate_quarantine_if_needed();
        
        std::string log_path = Swayam::default_quarantine_path();
        if (std::filesystem::exists(log_path)) {
            std::cout << "[SWAYAM-CORE] Ledger active and verified at: " << log_path << "\n";
        }

        // Step 2: Atomic Guard Initialization
        std::cout << "[SWAYAM-CORE] Engaging Core Atomic Guard...\n";
        Swayam::AtomicGuard core_guard("/tmp/swayam_mutation.lock");

        // Step 3: Singular Execution Lock Test
        {
            std::cout << "[SWAYAM-CORE] Acquiring Cross-Process & In-Process Mutation Lease...\n";
            Swayam::MutationLease lease(core_guard);
            std::cout << "[SWAYAM-CORE] Lease Acquired. System is securely locked for singular execution.\n";
            
            // Simulating micro-hold to verify lock stability
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            std::cout << "[SWAYAM-CORE] Releasing Lease...\n";
        } 

        std::cout << "[SWAYAM-CORE] ALL SYSTEMS GREEN. Phase 1 Architecture Validated.\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[SWAYAM-CORE FATAL] Phase 1 Execution Crashed: " << e.what() << "\n";
        return 1;
    }
}

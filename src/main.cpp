// src/main.cpp
#include "../include/core.hpp" 
#include "../include/SafeShell.hpp"
#include <iostream>
#include <filesystem>
#include <thread>

int main() {
    try {
        std::cout << "[SWAYAM-CORE] Core Initialization Sequence Engaged...\n";

        std::cout << "[SWAYAM-CORE] Verifying V2 Quarantine Ledger Integrity...\n";
        Swayam::migrate_quarantine_if_needed();
        
        std::cout << "[SWAYAM-CORE] Engaging Core Atomic Guard...\n";
        Swayam::AtomicGuard core_guard("/tmp/swayam_mutation.lock");

        {
            std::cout << "[SWAYAM-CORE] Acquiring Lease...\n";
            Swayam::MutationLease lease(core_guard);
            
            // Testing the Sandbox Limits directly in the bootstrap 
            // (In production, this only runs in the child process)
            std::cout << "[SWAYAM-CORE] Validating Sandbox Limits...\n";
            Swayam::SafeShell::lockdown_process(2, 128); // 2 Sec CPU, 128 MB RAM
            
            std::cout << "[SWAYAM-CORE] Releasing Lease...\n";
        } 

        std::cout << "[SWAYAM-CORE] ALL SYSTEMS GREEN. Phase 2 Sandbox Validated.\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[SWAYAM-CORE FATAL] Execution Crashed: " << e.what() << "\n";
        return 1;
    }
}

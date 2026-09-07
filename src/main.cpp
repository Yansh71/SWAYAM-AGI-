// =============================================================
// SWAYAM AGI — Core Entry Point (main.cpp)
// 
// ARCHITECTURE ENFORCEMENT: Synchronized with the Workspace-Aware 
// MutationRunner. Captures absolute spatial dimensions at boot 
// to ensure flawless execution tests before Supervisor handoff.
// =============================================================
#include "../include/core.hpp"
#include "../include/MutationRunner.hpp"
#include "../include/Supervisor.hpp"
#include "../include/HiveMind.hpp"
#include <iostream>
#include <string>
#include <filesystem>
#include <exception>

int main() {
    try {
        std::cout << "[SWAYAM-BOOT] Initializing Autonomous Core...\n";
        
        // THE APEX FIX: Capture the absolute workspace path immediately at boot
        std::string workspace = std::filesystem::current_path().string();
        std::cout << "[SWAYAM-BOOT] Operational Workspace Locked: " << workspace << "\n";

        // SURGICAL CORRECTION: Pass the absolute lock file path to AtomicGuard
        // Secures the lock inside the local workspace instead of global /tmp
        std::string lock_file_path = workspace + "/.swayam_mutation.lock";
        Swayam::AtomicGuard core_guard(lock_file_path);

        // ---------------------------------------------------------
        // PHASE 1: Safe Evolution Lifecycle Test
        // ---------------------------------------------------------
        std::cout << "[SWAYAM-BOOT] Executing Base Lifecycle Validation...\n";
        std::string evolved_code = "#include <iostream>\nint main() { std::cout << \"Evolution Test Pass\\n\"; return 0; }";
        
        // Passing the 4th argument (workspace) to satisfy the updated API
        bool execution_result = Swayam::MutationRunner::evaluate_and_execute(
            core_guard, 
            evolved_code, 
            "EVO_TEST_001", 
            workspace
        );

        if (!execution_result) {
            std::cerr << "[SWAYAM-BOOT FATAL] Base lifecycle validation failed. Aborting.\n";
            return 1;
        }

        // ---------------------------------------------------------
        // PHASE 2: Hostile Threat Isolation Test
        // ---------------------------------------------------------
        std::cout << "[SWAYAM-BOOT] Executing Hostile Threat Isolation Validation...\n";
        std::string malicious_mutation = "#include <cstdlib>\nint main() { system(\"echo MALICIOUS\"); return 0; }";
        
        // Passing the 4th argument (workspace) to satisfy the updated API
        bool threat_result = Swayam::MutationRunner::evaluate_and_execute(
            core_guard, 
            malicious_mutation, 
            "THREAT_TEST_002", 
            workspace
        );

        // A malicious mutation SHOULD fail. If it returns true, the sandbox is broken.
        if (threat_result) {
            std::cerr << "[SWAYAM-BOOT FATAL] Threat isolation failed. Malicious code executed! Aborting.\n";
            return 1;
        }
        std::cout << "[SWAYAM-BOOT] Threat neutralized successfully. Sandbox integrity verified.\n";

        // ---------------------------------------------------------
        // PHASE 3: Autonomous Supervisor Handoff
        // ---------------------------------------------------------
        std::cout << "[SWAYAM-BOOT] All systems green. Handing over control to Autonomous Supervisor...\n";
        
        // Base algorithm for continuous evolution
        std::string base_algorithm = "int main() { return 0; }"; 
        
        // Supervisor will natively capture the workspace again inside its own orchestration loop
        Swayam::Supervisor::orchestrate_evolution(core_guard, base_algorithm);

    } catch (const std::exception& e) {
        std::cerr << "[SWAYAM-BOOT FATAL EXCEPTION] " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[SWAYAM-BOOT FATAL EXCEPTION] Unknown systemic collapse.\n";
        return 1;
    }

    return 0;
}

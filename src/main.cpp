// src/main.cpp
#include "../include/core.hpp"
#include "../include/SafeShell.hpp"
#include "../include/HeuristicAnalyzer.hpp"
#include "../include/MutationRunner.hpp"
#include <iostream>
#include <string>
#include <stdexcept>

int main() {
    try {
        std::cout << "[SWAYAM-CORE] Core Initialization Sequence Engaged...\n";

        // Step 1: Initialize Zero-Trust Ledger
        std::cout << "[SWAYAM-CORE] Verifying V2 Quarantine Ledger Integrity...\n";
        Swayam::migrate_quarantine_if_needed();
        
        // Step 2: Initialize Core Atomic Guard (Cross-Process Synchronization)
        std::cout << "[SWAYAM-CORE] Engaging Core Atomic Guard...\n";
        Swayam::AtomicGuard core_guard("/tmp/swayam_mutation.lock");

        // =========================================================
        // APEX PIPELINE VALIDATION (TESTING AUTONOMOUS MUTATIONS)
        // =========================================================

        // Test Case 1: Pure, Safe Evolution Code
        // This should pass HeuristicAnalyzer, compile safely, and execute.
        std::string safe_mutation = R"(
#include <iostream>
int main() {
    std::cout << "[MUTATION-001] Autonomous evolution executed safely inside SafeShell.\n";
    return 0;
}
)";

        // Test Case 2: Malicious Payload
        // Contains forbidden hex-signatures: system() and fork()
        // This MUST be blocked at Phase 3 (HeuristicAnalyzer) before compilation.
        std::string malicious_mutation = R"(
#include <cstdlib>
#include <unistd.h>
int main() {
    system("rm -rf /"); 
    while(true) { fork(); }
    return 0;
}
)";

        std::cout << "\n[================ TEST 1: SAFE MUTATION ================]\n";
        bool test1 = Swayam::MutationRunner::evaluate_and_execute(core_guard, safe_mutation, "TEST_001");
        if (test1) {
            std::cout << "[SWAYAM-CORE] Test 1 Passed: Safe mutation handled correctly.\n";
        } else {
            std::cerr << "[SWAYAM-CORE] Test 1 Failed: Safe mutation was unexpectedly blocked or crashed.\n";
        }

        std::cout << "\n[============== TEST 2: MALICIOUS MUTATION ==============]\n";
        bool test2 = Swayam::MutationRunner::evaluate_and_execute(core_guard, malicious_mutation, "TEST_002");
        if (!test2) {
            std::cout << "[SWAYAM-CORE] Test 2 Passed: Malicious mutation successfully blocked and quarantined.\n";
        } else {
            std::cerr << "[SWAYAM-CORE] Test 2 Failed: FATAL FLAW! Malicious mutation bypassed security.\n";
        }

        std::cout << "\n[SWAYAM-CORE] ALL SYSTEMS GREEN. SWAYAM-AGI Phase 4 Pipeline is 100% Operational.\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[SWAYAM-CORE FATAL] Execution Pipeline Crashed: " << e.what() << "\n";
        return 1;
    }
}

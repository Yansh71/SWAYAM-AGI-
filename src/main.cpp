// src/main.cpp
#include "../include/core.hpp"
#include "../include/SafeShell.hpp"
#include "../include/HeuristicAnalyzer.hpp"
#include "../include/MutationRunner.hpp"
#include "../include/CognitiveForge.hpp"
#include <iostream>
#include <string>
#include <stdexcept>

int main() {
    try {
        std::cout << "[SWAYAM-CORE] Core Initialization Sequence Engaged...\n";

        // Step 1: Initialize Zero-Trust Ledger
        std::cout << "[SWAYAM-CORE] Verifying V2 Quarantine Ledger Integrity...\n";
        Swayam::migrate_quarantine_if_needed();
        
        // Step 2: Initialize Core Atomic Guard
        std::cout << "[SWAYAM-CORE] Engaging Core Atomic Guard...\n";
        Swayam::AtomicGuard core_guard("/tmp/swayam_mutation.lock");

        // =========================================================
        // PHASE 5 INTEGRATION: THE FULL AUTONOMOUS LIFECYCLE
        // =========================================================

        std::cout << "\n[============= STAGE 1: COGNITIVE FORGE =============]\n";
        // The base algorithm before autonomous evolution
        std::string base_algorithm = 
            "#include <iostream>\n"
            "int main() {\n"
            "    std::cout << \"[SWAYAM-AGI] I am SwayamMutation. Executing base logic.\\n\";\n"
            "    return 0;\n"
            "}\n";

        std::cout << "[SWAYAM-CORE] Injecting Entropy and Mutating AST...\n";
        std::string evolved_code = Swayam::CognitiveForge::evolve_codebase(base_algorithm);
        
        std::cout << "[SWAYAM-CORE] Evolution Complete. Mutated Source Code Generated:\n";
        std::cout << "--------------------------------------------------\n";
        std::cout << evolved_code;
        std::cout << "--------------------------------------------------\n";

        std::cout << "\n[========== STAGE 2: EVALUATE & EXECUTE ============]\n";
        bool execution_result = Swayam::MutationRunner::evaluate_and_execute(core_guard, evolved_code, "EVO_TEST_001");

        if (execution_result) {
            std::cout << "[SWAYAM-CORE] LIFECYCLE SUCCESS: Mutated code passed HeuristicAnalyzer and ran flawlessly in SafeShell.\n";
        } else {
            std::cerr << "[SWAYAM-CORE] LIFECYCLE FAILED: Mutated code was blocked or crashed.\n";
        }

        // =========================================================
        // THREAT VALIDATION (QUANTUM COMMAND: ZERO BYPASS TRICKS)
        // =========================================================
        std::cout << "\n[============ STAGE 3: THREAT VALIDATION ============]\n";
        
        // 100% Authentic Threat Signature. No string concatenation tricks.
        std::string malicious_mutation = 
            "#include <cstdlib>\n"
            "#include <unistd.h>\n"
            "int main() {\n"
            "    system(\"rm -rf /\");\n" 
            "    while(true) { fork(); }\n"
            "    return 0;\n"
            "}\n";

        bool threat_result = Swayam::MutationRunner::evaluate_and_execute(core_guard, malicious_mutation, "THREAT_TEST_002");
        
        if (!threat_result) {
            std::cout << "[SWAYAM-CORE] THREAT VALIDATED: Malicious payload caught and neutralized by HeuristicAnalyzer.\n";
        } else {
            std::cerr << "[SWAYAM-CORE] FATAL ARCHITECTURE FLAW: Malicious payload bypassed security!\n";
            return 1;
        }

        std::cout << "\n[SWAYAM-CORE] ALL SYSTEMS GREEN. SWAYAM-AGI IS FULLY AUTONOMOUS.\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[SWAYAM-CORE FATAL] Execution Pipeline Crashed: " << e.what() << "\n";
        return 1;
    }
}

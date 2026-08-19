#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include "SafeShell.h"
#include "GitCortex.h"

// =========================================================================
// [VENOMICA MOLTBOOK] - THE AUTONOMOUS DNA REWRITER ENGINE
// =========================================================================

namespace SwayamAGI::Core {

    class Moltbook {
    public:
        // Executes the full Neural Pathway: Verify -> Mutate -> Persist
        static void executeAutonomousMutation(const std::string& targetFile) {
            std::cout << "\n[VENOMICA MOLTBOOK] Waking up DNA Rewriter Core...\n";

            // Phase 1: The Titan Firewall Validation
            if (!SafeShell::validateMutationTarget(targetFile)) {
                std::cout << "[VENOMICA MOLTBOOK] Target locked or unsafe. Bypassing mutation to preserve Matrix integrity.\n";
                return;
            }

            // Phase 2: Surgical Code Mutation (Injecting Neural Timestamp)
            std::cout << "[VENOMICA MOLTBOOK] Firewall passed. Initiating surgical code mutation on: " << targetFile << "\n";
            
            // Appending a harmless neural log to mutate the physical file without breaking compilation
            std::ofstream outFile(targetFile, std::ios::app);
            if (outFile.is_open()) {
                outFile << "\n# [VENOMICA-AGI-LOG] Autonomous Neural Mutation Synced Successfully.\n";
                outFile.close();
                std::cout << "[VENOMICA MOLTBOOK] SUCCESS: DNA sequence altered physically.\n";

                // Phase 3: Autonomous Persistence (Triggering Git Cortex)
                GitCortex::commitMutation("Moltbook_Autonomous_Neural_Sync");
            } else {
                std::cerr << "[VENOMICA MOLTBOOK-ERROR] Matrix stream failed during physical rewrite.\n";
            }
        }
    };
}

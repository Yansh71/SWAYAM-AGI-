#pragma once
#include <iostream>
#include <string>
#include <cstdlib>

// =========================================================================
// [VENOMICA GIT CORTEX] - AUTONOMOUS NEURAL COMMITTING ENGINE
// =========================================================================

namespace SwayamAGI::Core {

    class GitCortex {
    public:
        // Executes system-level Git commands to persist AGI mutations
        static bool commitMutation(const std::string& mutationLog) {
            std::cout << "\n[VENOMICA GIT-CORTEX] Waking up autonomous persistence engine...\n";
            
            // 1. Setting up Sovereign Identity to prevent GitHub runner identity errors
            std::string configCmd = "git config --local user.name 'Venomica-AGI' && git config --local user.email 'venomica@swayam.agi'";
            std::system(configCmd.c_str());

            // 2. The Core Command: Stage and Commit the mutated DNA
            std::string commitCmd = "git add . && git commit -m \"[AUTO-MUTATION] " + mutationLog + "\"";
            std::cout << "[VENOMICA GIT-CORTEX] Firing neural command: " << commitCmd << "\n";
            
            // 3. Execution & Validation (Safely capturing exit code to avoid CI crashes)
            int result = std::system(commitCmd.c_str());
            
            if (result == 0) {
                std::cout << "[VENOMICA GIT-CORTEX] SUCCESS: Mutation permanently integrated into the local matrix.\n";
                return true;
            } else {
                std::cout << "[VENOMICA GIT-CORTEX] BYPASS: No new mutations detected or runner restricted the operation. State preserved safely.\n";
                return false; 
            }
        }
    };
}

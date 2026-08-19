#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

// =========================================================================
// [VENOMICA GIT-CORTEX: ZERO-TRUST SANDBOX WRITER]
// =========================================================================
// Principle: The agent never executes shell commands or handles tokens. 
// It only writes generated files to a local unprivileged output directory.
// =========================================================================

namespace SwayamAGI::Core {

    class GitCortex {
    public:
        static bool commitMutation(const std::string& mutationLog) {
            std::cout << "\n[VENOMICA GIT-CORTEX] Preparing unprivileged mutation payload...\n";
            
            // 1. Ensure the staging output directory exists
            std::string outputDir = "mutation-output/src/generated";
            std::error_code ec;
            std::filesystem::create_directories(outputDir, ec);

            if (ec) {
                std::cerr << "[VENOMICA GIT-CORTEX-ERROR] Failed to create sandbox output directory: " << ec.message() << "\n";
                return false;
            }

            // 2. Write the execution rationale/log for the automated publisher PR
            std::string rationalePath = "mutation-output/rationale.md";
            std::ofstream rationaleFile(rationalePath);
            if (rationaleFile.is_open()) {
                rationaleFile << "# Autonomous Mutation Rationale\n\n";
                rationaleFile << "- **Log Signature:** " << mutationLog << "\n";
                rationaleFile << "- **Execution Status:** Sandboxed neural generation successful.\n";
                rationaleFile.close();
            }

            std::cout << "[VENOMICA GIT-CORTEX] SUCCESS: Payload staged safely in " << outputDir << ". Handing off to isolated publisher.\n";
            return true;
        }
    };
}

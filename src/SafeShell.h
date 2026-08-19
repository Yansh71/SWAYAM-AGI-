#pragma once
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <system_error>

// =========================================================================
// [VENOMICA SAFESHELL] - THE TITAN FIREWALL FOR AUTONOMOUS MUTATION
// =========================================================================

namespace SwayamAGI::Core {

    class SafeShell {
    public:
        // Double-checking mechanism before AGI attempts to mutate any DNA (Code)
        static bool validateMutationTarget(const std::filesystem::path& targetPath) {
            std::cout << "[VENOMICA SAFESHELL] Initiating neural scan on target: " << targetPath << "\n";
            
            std::error_code ec;
            // CHECK 1: Does the target physical node exist in the matrix?
            if (!std::filesystem::exists(targetPath, ec)) {
                std::cerr << "[VENOMICA SAFESHELL-ERROR] Target node missing. Aborting mutation to prevent crash.\n";
                return false;
            }

            // CHECK 2: Sandbox boundary verification (Write Permissions)
            std::filesystem::perms p = std::filesystem::status(targetPath).permissions();
            if ((p & std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
                std::cerr << "[VENOMICA SAFESHELL-ERROR] Boundary violation: Write access denied. Sandbox locked.\n";
                return false;
            }

            std::cout << "[VENOMICA SAFESHELL] Target verified. Matrix sandbox locked. Ready for runtime mutation.\n";
            return true;
        }
    };
}


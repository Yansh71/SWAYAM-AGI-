#ifndef SWAYAM_SAFESHELL_HPP
#define SWAYAM_SAFESHELL_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace Swayam {

class SafeShell {
public:
    // Statically inspects generated or incoming command payloads for blacklisted system calls
    static bool audit_command_payload(const std::string& payload) {
        // Enterprise blacklisted dangerous system calls / patterns
        const std::vector<std::string> blacklisted_signatures = {
            "rm -rf /",
            "mkfs",
            ":(){ :|:& };:", // Fork bomb
            "dd if=/dev/zero",
            "nc -e",
            "bash -i >&"
        };

        for (const auto& sig : blacklisted_signatures) {
            if (payload.find(sig) != std::string::npos) {
                std::cerr << "[VENOMICA-FIREWALL FATAL] Unsafe kernel payload signature detected: " << sig << "\n";
                return false;
            }
        }
        return true;
    }

    static void enforce_sandboxed_execution(const std::string& command) {
        if (!audit_command_payload(command)) {
            throw std::runtime_error("[VENOMICA-FIREWALL] Execution halted. Payload violated Titan Firewall perimeter.");
        }
        std::cout << "[VENOMICA-FIREWALL] Command payload verified and cleared through SafeShell guard.\n";
    }
};

} // namespace Swayam

#endif // SWAYAM_SAFESHELL_HPP

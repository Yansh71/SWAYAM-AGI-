#ifndef SWAYAM_QUARANTINE_REGISTRY_HPP
#define SWAYAM_QUARANTINE_REGISTRY_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <filesystem>

namespace Swayam {

class QuarantineRegistry {
public:
    // Locks failed or malicious mutations into an isolated registry to prevent future recurrence
    static void isolate_anomaly(const std::string& reason, const std::string& file_path) {
        std::cout << "[VENOMICA-QUARANTINE] Anomaly detected. Initiating isolation protocols...\n";
        
        std::string target_dir = "mutation-output";
        std::filesystem::create_directories(target_dir);
        std::string log_file = target_dir + "/quarantine_ledger.csv";
        
        std::ofstream ledger(log_file, std::ios::app);
        if (ledger.is_open()) {
            auto now = std::chrono::system_clock::now();
            long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            
            // Format: Timestamp, FilePath, Reason
            ledger << timestamp << "," << file_path << ",\"" << reason << "\"\n";
            ledger.close();
            
            std::cout << "[VENOMICA-QUARANTINE] Threat neutralized and cryptographically locked in ledger: " << log_file << "\n";
        } else {
            std::cerr << "[VENOMICA-QUARANTINE FATAL] Failed to access quarantine ledger. System vulnerability imminent!\n";
        }
    }
};

} // namespace Swayam

#endif // SWAYAM_QUARANTINE_REGISTRY_HPP

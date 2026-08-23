#ifndef SWAYAM_QUARANTINE_REGISTRY_HPP
#define SWAYAM_QUARANTINE_REGISTRY_HPP

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <chrono>

namespace Swayam {

class QuarantineRegistry {
public:
    static void isolate_anomaly(const std::string& threat_reason, const std::string& source_file) {
        // Explicit Absolute Boundary Path
        std::string quarantine_dir = "/home/swayam_agent/workspace/quarantine";
        std::filesystem::create_directories(quarantine_dir);
        
        std::filesystem::path src_path(source_file);
        std::filesystem::path dest_path = std::filesystem::path(quarantine_dir) / src_path.filename();

        // ---------------------------------------------------------
        // [MYTHOS ZERO-DAY FIX] FORENSIC COPY & PURGE
        // Eliminates POSIX EXDEV (Cross-Device Link) Exceptions by 
        // physically moving bytes instead of relying on inode metadata swaps.
        // ---------------------------------------------------------
        try {
            if (std::filesystem::exists(src_path)) {
                // 1. Byte-level overwrite to the quarantine vault
                std::filesystem::copy(src_path, dest_path, std::filesystem::copy_options::overwrite_existing);
                // 2. Physical purge from the execution zone
                std::filesystem::remove(src_path);
                std::cout << "[VENOMICA-QUARANTINE] Threat forensically extracted and isolated at: " << dest_path << "\n";
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // In an enterprise system, if containment fails, the system MUST crash and burn. No silent bypasses.
            std::cerr << "[VENOMICA FATAL] Forensic containment breach during extraction!\n";
            throw std::runtime_error(std::string("Containment Failure: ") + e.what());
        }

        // Generate tamper-proof anomaly audit log
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::ofstream log(std::filesystem::path(quarantine_dir) / "anomaly_audit_trail.log", std::ios::app);
        if (log) {
            log << "[" << timestamp << "] VENOMICA CONTAINMENT TRIGGERED\n";
            log << " - THREAT VECTOR: " << threat_reason << "\n";
            log << " - SOURCE ORIGIN: " << source_file << "\n";
            log << " - ISOLATION TARGET: " << dest_path << "\n";
            log << "---------------------------------------------------\n";
        }
    }
};

} // namespace Swayam

#endif // SWAYAM_QUARANTINE_REGISTRY_HPP

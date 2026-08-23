#ifndef SWAYAM_QUARANTINE_REGISTRY_HPP
#define SWAYAM_QUARANTINE_REGISTRY_HPP

#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/file.h>   // flock
#include <unistd.h>

namespace Swayam {

class QuarantineRegistry {
public:
    static void isolate_anomaly(const std::string& threat_reason, const std::string& source_file) {
        std::string quarantine_dir = "/home/swayam_agent/workspace/quarantine";
        std::filesystem::create_directories(quarantine_dir);

        std::filesystem::path src_path(source_file);
        std::filesystem::path dest_path  = std::filesystem::path(quarantine_dir) / src_path.filename();
        std::filesystem::path state_path = std::filesystem::path(quarantine_dir) / "registry.state";
        std::filesystem::path lock_path  = std::filesystem::path(quarantine_dir) / "registry.lock";

        // Exclusive advisory lock: guards the isolate+record sequence
        // against a second concurrent caller of isolate_anomaly() (a
        // future entry point outside AtomicGuard, or a shared mount
        // across nodes).
        int lock_fd = ::open(lock_path.c_str(), O_CREAT | O_RDWR, 0600);
        if (lock_fd < 0) {
            throw std::runtime_error("[VENOMICA FATAL] Could not open quarantine lock file.");
        }
        if (::flock(lock_fd, LOCK_EX) != 0) {
            ::close(lock_fd);
            throw std::runtime_error("[VENOMICA FATAL] Could not acquire quarantine lock.");
        }

        auto write_state = [&](const std::string& state) {
            std::ofstream state_file(state_path, std::ios::trunc);
            state_file << state << " " << source_file << "\n";
        };

        try {
            // Record intent BEFORE touching the filesystem. If this
            // process is killed mid-copy, the next startup finds
            // "RUNNING" here and knows this entry needs re-checking
            // rather than being silently lost.
            write_state("RUNNING");

            if (std::filesystem::exists(src_path)) {
                std::filesystem::copy(src_path, dest_path,
                                       std::filesystem::copy_options::overwrite_existing);
                std::filesystem::remove(src_path);
                std::cout << "[VENOMICA-QUARANTINE] Threat forensically extracted and isolated at: "
                          << dest_path << "\n";
            }

            write_state("QUARANTINED");
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[VENOMICA FATAL] Forensic containment breach during extraction: "
                      << e.what() << "\n";
            ::flock(lock_fd, LOCK_UN);
            ::close(lock_fd);
            throw std::runtime_error(std::string("Containment Failure: ") + e.what());
        }

        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now.time_since_epoch()).count();

        std::filesystem::path log_path = std::filesystem::path(quarantine_dir) / "anomaly_audit_trail.log";
        std::ofstream log(log_path, std::ios::app);
        if (log) {
            log << "[" << timestamp << "] VENOMICA CONTAINMENT TRIGGERED\n";
            log << " - THREAT VECTOR: " << threat_reason << "\n";
            log << " - SOURCE ORIGIN: " << source_file << "\n";
            log << " - ISOLATION TARGET: " << dest_path << "\n";
            log << "--------------------------------------------------\n";
        }

        ::flock(lock_fd, LOCK_UN);
        ::close(lock_fd);
    }
};

} // namespace Swayam
#endif // SWAYAM_QUARANTINE_REGISTRY_HPP

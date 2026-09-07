#ifndef SWAYAM_SECURE_ARTIFACT_HPP
#define SWAYAM_SECURE_ARTIFACT_HPP
// =============================================================
// SWAYAM SecureArtifact — The Anonymous Vault
// 
// Defends against Symlink Attacks (CWE-59) and Race Conditions 
// by utilizing anonymous O_TMPFILE creation, O_NOFOLLOW bounds, 
// and atomic physical linking via /proc/self/fd/.
// =============================================================
#include <string>
#include <iostream>
#include <stdexcept>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

namespace Swayam {

class SecureArtifact {
public:
    // Writes generated autonomous code to disk with absolute POSIX safety
    static bool write_securely(const std::string& directory, const std::string& filename, const std::string& content) noexcept {
        
        // 1. Directory Isolation
        // O_NOFOLLOW ensures we do not traverse malicious symlinks when opening the target directory.
        int dir_fd = open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (dir_fd == -1) {
            std::cerr << "[SWAYAM-ARTIFACT FATAL] Directory boundary breach or missing path.\n";
            return false;
        }

        // 2. The Anonymous Vault (O_TMPFILE)
        // openat creates a nameless, invisible file in the filesystem. Zero race-condition window.
        int tmp_fd = openat(dir_fd, ".", O_TMPFILE | O_RDWR | O_CLOEXEC, 0600);
        if (tmp_fd == -1) {
            std::cerr << "[SWAYAM-ARTIFACT FATAL] Failed to generate anonymous memory-mapped file.\n";
            close(dir_fd);
            return false;
        }

        // 3. Raw Memory Transfer
        ssize_t written = write(tmp_fd, content.c_str(), content.size());
        if (written != static_cast<ssize_t>(content.size())) {
            std::cerr << "[SWAYAM-ARTIFACT FATAL] Incomplete memory transfer to vault.\n";
            close(tmp_fd);
            close(dir_fd);
            return false;
        }

        // 4. Absolute Data Integrity
        // fsync forces the OS to physically write the buffer to disk before proceeding.
        if (fsync(tmp_fd) == -1) {
            std::cerr << "[SWAYAM-ARTIFACT FATAL] fsync integrity lock failed.\n";
            close(tmp_fd);
            close(dir_fd);
            return false;
        }

        // 5. Atomic Materialization
        // Safely link the invisible file descriptor into the physical filesystem hierarchy.
        std::string proc_fd_path = "/proc/self/fd/" + std::to_string(tmp_fd);
        
        if (linkat(AT_FDCWD, proc_fd_path.c_str(), dir_fd, filename.c_str(), AT_SYMLINK_FOLLOW) == -1) {
            std::cerr << "[SWAYAM-ARTIFACT FATAL] linkat materialization blocked.\n";
            close(tmp_fd);
            close(dir_fd);
            return false;
        }

        std::cout << "[SWAYAM-ARTIFACT] Vault materialized securely: " << filename << "\n";

        // 6. Memory Cleanup
        close(tmp_fd);
        close(dir_fd);
        return true;
    }
};

} // namespace Swayam
#endif // SWAYAM_SECURE_ARTIFACT_HPP

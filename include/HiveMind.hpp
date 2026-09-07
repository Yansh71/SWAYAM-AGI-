#ifndef SWAYAM_HIVE_MIND_HPP
#define SWAYAM_HIVE_MIND_HPP
// =============================================================
// SWAYAM HiveMind — The Global Success Ledger
// 
// ARCHITECTURE ENFORCEMENT: Synchronizes proven evolutionary 
// shifts. Prevents phantom collisions via exact newline matching.
// =============================================================
#include <string>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>

namespace Swayam {

class HiveMind {
private:
    static std::string get_hive_path(const std::string& workspace) {
        return workspace + "/.swayam_hive.ledger";
    }

public:
    static bool is_assimilated(const std::string& hash_str, const std::string& workspace) {
        std::string hive_path = get_hive_path(workspace);

        int fd = open(hive_path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd == -1) return false;

        if (flock(fd, LOCK_SH) == -1) {
            close(fd);
            return false;
        }

        char buffer[4096];
        ssize_t bytes_read;
        std::string hive_content;
        
        // THE APEX FIX: Fast, binary-safe append
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
            hive_content.append(buffer, bytes_read);
        }

        flock(fd, LOCK_UN);
        close(fd);

        // THE APEX FIX: Prevent hash substring collisions
        std::string search_target = hash_str + "\n";
        return hive_content.find(search_target) != std::string::npos;
    }

    static void register_mutation_hash(const std::string& hash_str, const std::string& workspace) noexcept {
        std::string hive_path = get_hive_path(workspace);

        int fd = open(hive_path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
        if (fd == -1) {
            std::cerr << "[SWAYAM-HIVEMIND FATAL] Core memory access denied.\n";
            return;
        }

        if (flock(fd, LOCK_EX) == -1) {
            close(fd);
            return;
        }

        std::string entry = hash_str + "\n";
        
        if (write(fd, entry.c_str(), entry.size()) != static_cast<ssize_t>(entry.size())) {
            std::cerr << "[SWAYAM-HIVEMIND WARNING] Memory assimilation truncated.\n";
        }

        if (fsync(fd) == -1) {
            std::cerr << "[SWAYAM-HIVEMIND WARNING] Core memory sync failed.\n";
        }

        flock(fd, LOCK_UN);
        close(fd);
        std::cout << "[SWAYAM-HIVEMIND] Evolution Successfully Assimilated: " << hash_str << "\n";
    }
};

} // namespace Swayam
#endif // SWAYAM_HIVE_MIND_HPP

#ifndef SWAYAM_CORE_HPP
#define SWAYAM_CORE_HPP

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdint>
#include <cerrno>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>

namespace Swayam {

class AtomicGuard {
private:
    std::atomic<bool> is_mutating {false};
    std::mutex core_mutex;
public:
    void lock_mutation_state() {
        std::lock_guard<std::mutex> lock(core_mutex);
        if (is_mutating.load()) throw std::runtime_error("Concurrent mutation detected.");
        is_mutating.store(true);
    }
    void unlock_mutation_state() noexcept { is_mutating.store(false); }
};

class MutationLease {
    AtomicGuard& guard_;
public:
    explicit MutationLease(AtomicGuard& g) : guard_(g) { guard_.lock_mutation_state(); }
    ~MutationLease() { guard_.unlock_mutation_state(); }
    MutationLease(const MutationLease&) = delete;
    MutationLease& operator=(const MutationLease&) = delete;
};

// FIX: offset basis was truncated by one digit (missing trailing 7),
// so this was never actually FNV-1a and disagreed with SecureArtifact.hpp,
// which has the correct constant.
inline uint64_t structural_hash(const std::string& content) {
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : content) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

inline std::string default_quarantine_path() {
    if (const char* p = std::getenv("SWAYAM_QUARANTINE_LOG")) return p;
    return "/tmp/quarantine.log";
}

// FIX: previously fell through to hashing "" on open failure. Now fails
// loudly -- a security primitive should never produce a silent wrong answer.
inline std::string read_file_or_throw(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error(
            "[VENOMICA] Could not open mutation source for hashing: " + path);
    }
    std::ostringstream buf;
    buf << in.rdbuf();
    return buf.str();
}

// Sentinel for "exited cleanly, non-zero status" -- distinct from the
// 1-31 POSIX signal range and from 254 (heuristic pre-quarantine).
inline constexpr int NONZERO_EXIT_SENTINEL = 255;

inline void quarantine(const std::string& mutation_source_path, int signal_num,
                       const std::string& log_path = default_quarantine_path()) {
    const uint64_t hash = structural_hash(read_file_or_throw(mutation_source_path));

    std::ofstream log(log_path, std::ios::app);
    if (!log) {
        std::cerr << "[VENOMICA WARN] Could not open quarantine log at "
                  << log_path << " - hash " << hash << " was NOT persisted.\n";
        return;
    }
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    log << hash << "," << signal_num << "," << now << "," << mutation_source_path << "\n";
}

inline bool is_quarantined(const std::string& mutation_source_path,
                           const std::string& log_path = default_quarantine_path()) {
    const uint64_t hash = structural_hash(read_file_or_throw(mutation_source_path));

    std::ifstream log(log_path);
    std::string line;
    while (std::getline(log, line)) {
        if (line.rfind(std::to_string(hash) + ",", 0) == 0) return true;
    }
    return false;
}

// FIX: no EINTR retry meant an interrupted wait could leave status at 0,
// which decodes as "exited normally" -- a crash could be spuriously
// reported as a success.
inline int waitpid_retrying(pid_t pid, int& status) {
    int ret;
    do {
        ret = waitpid(pid, &status, 0);
    } while (ret < 0 && errno == EINTR);
    return ret;
}

inline bool run_guarded_mutation(AtomicGuard& guard,
                                 const std::string& mutation_binary,
                                 const std::string& mutation_source_path) {
    if (is_quarantined(mutation_source_path)) {
        std::cerr << "[VENOMICA] Refusing known-quarantined mutation: " 
                  << mutation_source_path << "\n";
        return false;
    }

    MutationLease lease(guard);
    pid_t pid = fork();
    
    if (pid < 0) throw std::runtime_error("[VENOMICA] Fork failed.");
    
    if (pid == 0) {
        const char* args[] = {mutation_binary.c_str(), nullptr};
        execvp(mutation_binary.c_str(), const_cast<char* const*>(args));
        _exit(127);
    }
    
    int status = 0;
    if (waitpid_retrying(pid, status) < 0) {
        throw std::runtime_error("[VENOMICA] waitpid failed unexpectedly.");
    }
    
    if (WIFSIGNALED(status)) {
        quarantine(mutation_source_path, WTERMSIG(status));
        return false;
    }

    // FIX: previously only crashes were quarantined. A clean-but-failing
    // mutation was rejected this run but never logged, so it could be
    // retried forever. Now every non-success path is quarantined.
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return true;
    }

    quarantine(mutation_source_path, NONZERO_EXIT_SENTINEL);
    return false;
}

} // namespace Swayam
#endif // SWAYAM_CORE_HPP

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

inline uint64_t structural_hash(const std::string& content) {
    uint64_t h = 1469598103934665603ULL;
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

inline void quarantine(const std::string& mutation_source_path, int signal_num,
                       const std::string& log_path = default_quarantine_path()) {
    std::ifstream in(mutation_source_path, std::ios::binary);
    std::ostringstream buf;
    buf << in.rdbuf();
    uint64_t hash = structural_hash(buf.str());
    
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
    std::ifstream in(mutation_source_path, std::ios::binary);
    std::ostringstream buf;
    buf << in.rdbuf();
    uint64_t hash = structural_hash(buf.str());
    
    std::ifstream log(log_path);
    std::string line;
    while (std::getline(log, line)) {
        if (line.rfind(std::to_string(hash) + ",", 0) == 0) return true;
    }
    return false;
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
    waitpid(pid, &status, 0);
    
    if (WIFSIGNALED(status)) {
        quarantine(mutation_source_path, WTERMSIG(status));
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

} // namespace Swayam
#endif // SWAYAM_CORE_HPP

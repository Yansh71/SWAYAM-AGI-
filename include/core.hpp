#ifndef SWAYAM_CORE_HPP
#define SWAYAM_CORE_HPP
// =============================================================
// SWAYAM core.hpp — Hardening Round 4
//
// Changes from v3:
//   FIX-1  CrossProcessGuard: flock()-based RAII exclusive lock.
//          MutationLease now holds both the in-process atomic
//          flag AND the cross-process flock, so two independent
//          GitHub Actions runners cannot run concurrent mutations.
//
//   FIX-2  atomic_quarantine_append(): write to a PID-namespaced
//          temp file → fsync → rename(). rename() is atomic on
//          POSIX — the log is never in a partial state. A SIGKILL
//          mid-write orphans the temp file, not the live log.
//          is_quarantined() acquires a shared flock before read
//          to close the TOCTOU window between check and fork.
//
//   FIX-3  FNV-1a constant corrected to the canonical 64-bit
//          basis (14695981039346656037ULL). migrate_quarantine()
//          detects legacy logs (missing version header) and
//          backs them up rather than silently mismatching hashes.
//
//   Quarantine log format (V2):
//     Line 0: #SWAYAM-QUARANTINE-V2   (version sentinel)
//     Line N: hash,signal,timestamp,source_label
//     signal=254 → pre-quarantined by HeuristicAnalyzer
//     signal=0   → imported from Hive Mind peer
//     signal=1-31 → POSIX signal from sandbox crash
// =============================================================
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/file.h>   // flock()
#include <sys/stat.h>

namespace Swayam {

// ---------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------

// FNV-1a 64-bit canonical basis (corrected from truncated v1 value).
// BREAKING CHANGE: logs computed with the old basis are incompatible.
// Call migrate_quarantine() on startup to detect and handle this.
static constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
static constexpr uint64_t FNV_PRIME        = 1099511628211ULL;

// First line written to every V2 quarantine log.
// Absence of this header means legacy (V1) log — migrate.
static constexpr std::string_view QUARANTINE_VERSION_HEADER =
    "#SWAYAM-QUARANTINE-V2";

// ---------------------------------------------------------------
// structural_hash(content): FNV-1a over a string.
// structural_hash_file(path): same, reading from a file.
// ---------------------------------------------------------------
inline uint64_t structural_hash(const std::string& content) noexcept {
    uint64_t h = FNV_OFFSET_BASIS;
    for (unsigned char c : content) {
        h ^= c;
        h *= FNV_PRIME;
    }
    return h;
}

inline uint64_t structural_hash_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buf;
    buf << in.rdbuf();
    return structural_hash(buf.str());
}

// ---------------------------------------------------------------
// default_quarantine_path(): respects SWAYAM_QUARANTINE_LOG.
// ---------------------------------------------------------------
inline std::string default_quarantine_path() {
    if (const char* p = std::getenv("SWAYAM_QUARANTINE_LOG")) return p;
    return "/tmp/quarantine.log";
}

// ---------------------------------------------------------------
// FIX-1: CrossProcessGuard
//
// RAII exclusive flock() on a lock file. Blocks until the lock
// is available (LOCK_EX). The lock is automatically released
// when the guard object is destroyed — on normal return, C++
// exception, or child-process crash in the supervisor frame
// (supervisor's stack is intact; the child that crashed never
// held this lock).
//
// The lock file path is lock_path + ".lock" by convention.
// ---------------------------------------------------------------
class CrossProcessGuard {
    int  fd_{-1};
    bool locked_{false};

public:
    explicit CrossProcessGuard(const std::string& lock_path) {
        fd_ = ::open(lock_path.c_str(),
                     O_CREAT | O_RDWR | O_CLOEXEC, 0600);
        if (fd_ < 0) {
            throw std::runtime_error(
                "[XPGUARD] Cannot open lock file '" + lock_path +
                "': " + std::strerror(errno));
        }

        // LOCK_EX: exclusive. Blocks until no other process holds it.
        // flock() is released automatically if the process exits or
        // the fd is closed — no manual cleanup needed on crash.
        if (::flock(fd_, LOCK_EX) != 0) {
            ::close(fd_);
            fd_ = -1;
            throw std::runtime_error(
                "[XPGUARD] flock(LOCK_EX) failed on '" + lock_path +
                "': " + std::strerror(errno));
        }
        locked_ = true;
    }

    ~CrossProcessGuard() noexcept { release(); }

    void release() noexcept {
        if (locked_ && fd_ >= 0) {
            ::flock(fd_, LOCK_UN);
            ::close(fd_);
            fd_     = -1;
            locked_ = false;
        }
    }

    CrossProcessGuard(const CrossProcessGuard&)            = delete;
    CrossProcessGuard& operator=(const CrossProcessGuard&) = delete;
};

// ---------------------------------------------------------------
// AtomicGuard: in-process mutual exclusion.
// Combined with CrossProcessGuard in MutationLease for full
// cross-runner serialization.
// lock_file_path_ is the base path; CrossProcessGuard appends
// ".lock" to get the actual lock file.
// ---------------------------------------------------------------
class AtomicGuard {
    std::atomic<bool> is_mutating_{false};
    std::mutex        core_mutex_;
    std::string       lock_file_path_;

public:
    explicit AtomicGuard(
        const std::string& lock_path = "/tmp/swayam_mutation.lock")
        : lock_file_path_(lock_path) {}

    void lock_mutation_state() {
        std::lock_guard<std::mutex> lock(core_mutex_);
        if (is_mutating_.load())
            throw std::runtime_error(
                "[ATOMICGUARD] Concurrent in-process mutation detected.");
        is_mutating_.store(true);
    }

    void unlock_mutation_state() noexcept {
        is_mutating_.store(false);
    }

    const std::string& lock_file_path() const noexcept {
        return lock_file_path_;
    }
};

// ---------------------------------------------------------------
// MutationLease: RAII wrapper that acquires BOTH guards.
//
// Acquisition order (to avoid deadlock):
//   1. CrossProcessGuard (flock — blocks other runners)
//   2. AtomicGuard (in-process atomic — blocks same-process threads)
//
// Release order (RAII, reverse of acquisition):
//   2. AtomicGuard released in destructor
//   1. CrossProcessGuard released when xp_guard_ is destroyed
//
// This means on a SIGSEGV in the supervisor frame, both locks
// are released — the supervisor never executed the crashing code.
// ---------------------------------------------------------------
class MutationLease {
    AtomicGuard&      guard_;
    CrossProcessGuard xp_guard_;  // acquired first, released last

public:
    explicit MutationLease(AtomicGuard& g)
        : guard_(g)
        , xp_guard_(g.lock_file_path() + ".lock") { // flock acquired here
        guard_.lock_mutation_state();                // atomic acquired here
    }

    ~MutationLease() {
        guard_.unlock_mutation_state(); // atomic released first
        // xp_guard_ destructor releases flock next
    }

    MutationLease(const MutationLease&)            = delete;
    MutationLease& operator=(const MutationLease&) = delete;
};

// ---------------------------------------------------------------
// FIX-3: migrate_quarantine_if_needed()
//
// Detects V1 logs (missing version header, computed with the
// wrong FNV-1a basis) and backs them up. The backup is preserved
// for forensic reference. A fresh V2 log is created.
//
// Call this ONCE at startup before any quarantine read/write.
// ---------------------------------------------------------------
inline void migrate_quarantine_if_needed(
    const std::string& log_path = default_quarantine_path()) {
    std::ifstream in(log_path);
    if (!in) {
        // No existing log — create fresh V2 log.
        std::ofstream out(log_path, std::ios::trunc);
        if (out) out << QUARANTINE_VERSION_HEADER << "\n";
        return;
    }

    std::string first_line;
    std::getline(in, first_line);
    in.close();

    if (first_line == QUARANTINE_VERSION_HEADER) return; // already V2

    // Legacy log detected. Back up and reinitialize.
    const std::string backup = log_path + ".v1-legacy.bak";
    if (::rename(log_path.c_str(), backup.c_str()) == 0) {
        std::cerr
            << "[QUARANTINE MIGRATION] V1 log backed up to: "
            << backup << "\n"
            << "[QUARANTINE MIGRATION] Legacy entries were computed with "
               "truncated FNV-1a basis and are incompatible with V2 hashes.\n"
            << "[QUARANTINE MIGRATION] A fresh V2 ledger has been created. "
               "Previously quarantined mutations may be retried once.\n";
    } else {
        std::cerr
            << "[QUARANTINE MIGRATION WARN] Could not back up V1 log ("
            << std::strerror(errno) << "). Proceeding may re-run "
               "previously quarantined mutations.\n";
    }

    // Write fresh V2 header
    std::ofstream fresh(log_path, std::ios::trunc);
    if (fresh) fresh << QUARANTINE_VERSION_HEADER << "\n";
}

// ---------------------------------------------------------------
// FIX-2A: atomic_quarantine_append()
//
// Writes one quarantine entry atomically:
//   1. Acquires exclusive flock on log_path + ".lock"
//   2. Reads existing log content
//   3. Writes existing + new_entry to a PID-namespaced temp file
//   4. fsync() the temp file (flush OS buffers to storage)
//   5. rename() temp → log_path (atomic on POSIX)
//   6. Releases flock
//
// A SIGKILL at step 3 or 4 leaves an orphaned .tmp file.
// The live log is untouched. On next run, orphaned temps can
// be cleaned by looking for log_path + ".tmp.*" patterns.
// ---------------------------------------------------------------
inline void atomic_quarantine_append(
    const std::string& log_path,
    uint64_t           hash,
    int                signal_num,
    const std::string& source_label) {

    const std::string lock_path = log_path + ".lock";
    const std::string tmp_path  = log_path + ".tmp." +
                                  std::to_string(::getpid());

    try {
        CrossProcessGuard write_lock(lock_path); // exclusive flock

        // Read existing content (preserving version header)
        std::string existing;
        {
            std::ifstream ef(log_path);
            if (ef) {
                std::ostringstream ss;
                ss << ef.rdbuf();
                existing = ss.str();
            }
        }
        // Ensure version header is present even if log was empty
        if (existing.empty() ||
            existing.substr(0, QUARANTINE_VERSION_HEADER.size()) !=
                QUARANTINE_VERSION_HEADER) {
            existing = std::string(QUARANTINE_VERSION_HEADER) + "\n" + existing;
        }

        // Build new entry line
        auto now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        std::string entry = std::to_string(hash) + "," +
                            std::to_string(signal_num) + "," +
                            std::to_string(now) + "," +
                            source_label + "\n";

        // Write existing + new entry to temp file
        {
            std::ofstream tmp(tmp_path,
                              std::ios::trunc | std::ios::binary);
            if (!tmp) {
                std::cerr << "[QUARANTINE] Cannot open temp file: "
                          << tmp_path << " — " << std::strerror(errno) << "\n";
                return;
            }
            tmp << existing << entry;
        } // ofstream destructor flushes to OS buffers

        // fsync: flush OS buffers to storage before rename
        {
            int tfd = ::open(tmp_path.c_str(),
                             O_RDONLY | O_CLOEXEC);
            if (tfd >= 0) {
                ::fsync(tfd);
                ::close(tfd);
            }
        }

        // Atomic rename — POSIX guarantees this is atomic
        if (::rename(tmp_path.c_str(), log_path.c_str()) != 0) {
            std::cerr << "[QUARANTINE] rename() failed: "
                      << std::strerror(errno) << "\n";
            ::unlink(tmp_path.c_str()); // clean up orphaned temp
        }

    } catch (const std::exception& e) {
        std::cerr << "[QUARANTINE WARN] Atomic write failed: "
                  << e.what() << "\n";
        ::unlink(tmp_path.c_str());
    }
}

// ---------------------------------------------------------------
// quarantine(): log a crash by source file path.
// ---------------------------------------------------------------
inline void quarantine(
    const std::string& mutation_source_path,
    int                signal_num,
    const std::string& log_path = default_quarantine_path()) {

    uint64_t hash = structural_hash_file(mutation_source_path);
    atomic_quarantine_append(log_path, hash, signal_num,
                              mutation_source_path);
}

// ---------------------------------------------------------------
// quarantine_by_content(): log a hash before the source file
// is written to disk (used by CognitiveForge pre-quarantine).
// signal=254 by convention: blocked by HeuristicAnalyzer.
// ---------------------------------------------------------------
inline void quarantine_by_content(
    const std::string& content,
    const std::string& source_label,
    int                signal_num = 254,
    const std::string& log_path = default_quarantine_path()) {

    uint64_t hash = structural_hash(content);
    atomic_quarantine_append(log_path, hash, signal_num,
                              "pre-quarantine:" + source_label);
}

// ---------------------------------------------------------------
// FIX-2B: is_quarantined()
//
// Acquires a SHARED flock (LOCK_SH) before reading the log.
// Multiple readers can hold LOCK_SH simultaneously; a writer
// holding LOCK_EX blocks until all readers release.
// This closes the TOCTOU window between is_quarantined() and
// the subsequent fork() in run_guarded_mutation().
//
// Note: the TOCTOU window is reduced but not to zero — flock
// is released before fork(). The remaining window is:
//   release LOCK_SH → fork() child → child executes
// To close this to zero would require holding LOCK_SH across
// fork(), which would then block concurrent quarantine writes
// indefinitely. The current approach is the practical optimum.
// ---------------------------------------------------------------
inline bool is_quarantined(
    const std::string& mutation_source_path,
    const std::string& log_path = default_quarantine_path()) {

    uint64_t hash = structural_hash_file(mutation_source_path);
    const std::string hash_str  = std::to_string(hash);
    const std::string lock_path = log_path + ".lock";

    // Shared (read) lock — multiple readers allowed simultaneously
    int lock_fd = ::open(lock_path.c_str(),
                         O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (lock_fd >= 0) ::flock(lock_fd, LOCK_SH);

    bool found = false;
    {
        std::ifstream log(log_path);
        std::string line;
        while (std::getline(log, line)) {
            // Skip version header and empty lines
            if (line.empty() || line[0] == '#') continue;
            auto comma = line.find(',');
            if (comma == std::string::npos) continue;
            if (line.substr(0, comma) == hash_str) {
                found = true;
                break;
            }
        }
    }

    if (lock_fd >= 0) {
        ::flock(lock_fd, LOCK_UN);
        ::close(lock_fd);
    }
    return found;
}

// ---------------------------------------------------------------
// run_guarded_mutation(): unchanged in logic, now benefits from
// the hardened MutationLease (cross-process flock + in-process
// atomic) and the atomic quarantine write on crash.
// ---------------------------------------------------------------
inline bool run_guarded_mutation(
    AtomicGuard&       guard,
    const std::string& mutation_binary,
    const std::string& mutation_source_path) {

    if (is_quarantined(mutation_source_path)) {
        std::cerr << "[VENOMICA] Pre-flight: known-quarantined mutation "
                  << mutation_source_path << " — refusing fork.\n";
        return false;
    }

    MutationLease lease(guard); // cross-process flock + in-process atomic

    pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error("[VENOMICA] fork() failed.");

    if (pid == 0) {
        const char* args[] = {mutation_binary.c_str(), nullptr};
        execvp(mutation_binary.c_str(),
               const_cast<char* const*>(args));
        _exit(127);
    }

    // waitpid retry loop: handles EINTR from signals delivered to
    // the supervisor (e.g. SIGCHLD from unrelated children)
    int status = 0;
    pid_t result;
    do {
        result = ::waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        std::cerr << "[VENOMICA] waitpid() failed: "
                  << std::strerror(errno) << "\n";
        return false;
    }

    if (WIFSIGNALED(status)) {
        quarantine(mutation_source_path, WTERMSIG(status));
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

} // namespace Swayam
#endif // SWAYAM_CORE_HPP

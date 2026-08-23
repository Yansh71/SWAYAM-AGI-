// QuarantineRegistry.hpp
#pragma once

#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

namespace Swayam {

enum class ClaimResult {
    Claimed,
    AlreadyQuarantined,
    AlreadyRunning,
    Error
};

class ScopedLock {
    int fd_{-1};

public:
    explicit ScopedLock(const std::filesystem::path& lock_path) {
        fd_ = ::open(lock_path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
        if (fd_ < 0) throw std::runtime_error("cannot open quarantine lock");

        for (;;) {
            if (::flock(fd_, LOCK_EX) == 0) break;
            if (errno == EINTR) continue;
            ::close(fd_);
            fd_ = -1;
            throw std::runtime_error("cannot acquire quarantine lock");
        }
    }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

    ~ScopedLock() {
        if (fd_ >= 0) {
            (void)::flock(fd_, LOCK_UN);
            (void)::close(fd_);
        }
    }
};

class QuarantineRegistry {
private:
    std::filesystem::path state_path_;
    std::filesystem::path lock_path_;

    enum class State { Unknown, Running, Quarantined };

    static State parse_state(const std::string& line, std::uint64_t hash) {
        std::ostringstream prefix;
        prefix << std::hex << std::setw(16) << std::setfill('0') << hash;

        if (line.rfind(prefix.str() + ",QUARANTINED", 0) == 0) return State::Quarantined;
        if (line.rfind(prefix.str() + ",RUNNING", 0) == 0) return State::Running;
        return State::Unknown;
    }

    State read_state(std::uint64_t hash) {
        std::ifstream in(state_path_);
        if (!in) return State::Unknown;

        std::string line;
        State result = State::Unknown;
        while (std::getline(in, line)) {
            const State s = parse_state(line, hash);
            if (s != State::Unknown) result = s;
        }
        return result;
    }

    void append_state(std::uint64_t hash, const char* state) {
        const int fd = ::open(state_path_.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
        if (fd < 0) throw std::runtime_error("cannot open quarantine state");

        std::ostringstream line;
        line << std::hex << std::setw(16) << std::setfill('0') << hash << ',' << state << '\n';
        const std::string text = line.str();

        std::size_t written = 0;
        while (written < text.size()) {
            const ssize_t n = ::write(fd, text.data() + written, text.size() - written);
            if (n < 0) {
                if (errno == EINTR) continue;
                ::close(fd);
                throw std::runtime_error("quarantine state write failed");
            }
            written += static_cast<std::size_t>(n);
        }

        if (::fsync(fd) != 0) {
            ::close(fd);
            throw std::runtime_error("quarantine state fsync failed");
        }
        ::close(fd);
    }

public:
    QuarantineRegistry(std::filesystem::path state, std::filesystem::path lock)
        : state_path_(std::move(state)), lock_path_(std::move(lock)) {}

    ClaimResult claim(std::uint64_t mutation_hash) {
        try {
            ScopedLock lock(lock_path_);
            const State current = read_state(mutation_hash);

            if (current == State::Quarantined) return ClaimResult::AlreadyQuarantined;
            if (current == State::Running) return ClaimResult::AlreadyRunning;

            append_state(mutation_hash, "RUNNING");
            return ClaimResult::Claimed;
        } catch (...) {
            return ClaimResult::Error;
        }
    }

    void quarantine(std::uint64_t mutation_hash) {
        ScopedLock lock(lock_path_);
        append_state(mutation_hash, "QUARANTINED");
    }

    void release_success(std::uint64_t mutation_hash) {
        ScopedLock lock(lock_path_);
        append_state(mutation_hash, "PASSED");
    }
};

} // namespace Swayam

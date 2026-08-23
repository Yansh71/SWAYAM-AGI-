#pragma once

#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

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
private:
    int fd_{-1};

public:
    explicit ScopedLock(
        const std::filesystem::path& lock_path)
    {
        fd_ = ::open(
            lock_path.c_str(),
            O_CREAT | O_RDWR | O_CLOEXEC,
            0600);

        if (fd_ < 0) {
            throw std::runtime_error(
                "cannot open quarantine lock");
        }

        for (;;) {
            if (::flock(fd_, LOCK_EX) == 0) {
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            ::close(fd_);
            fd_ = -1;

            throw std::runtime_error(
                "cannot acquire quarantine lock");
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

    enum class State {
        Unknown,
        Running,
        Quarantined,
        Passed
    };


    static std::string hash_string(
        std::uint64_t hash)
    {
        std::ostringstream out;

        out << std::hex
            << std::setw(16)
            << std::setfill('0')
            << hash;

        return out.str();
    }


    static std::uint64_t hash_identity(
        const std::string& value)
    {
        constexpr std::uint64_t offset =
            14695981039346656037ULL;

        constexpr std::uint64_t prime =
            1099511628211ULL;

        std::uint64_t hash = offset;

        for (const unsigned char c : value) {
            hash ^= static_cast<std::uint64_t>(c);
            hash *= prime;
        }

        return hash;
    }


    static State parse_state(
        const std::string& line,
        std::uint64_t hash)
    {
        const std::string prefix =
            hash_string(hash);

        if (line.rfind(
                prefix + ",QUARANTINED",
                0) == 0)
        {
            return State::Quarantined;
        }

        if (line.rfind(
                prefix + ",RUNNING",
                0) == 0)
        {
            return State::Running;
        }

        if (line.rfind(
                prefix + ",PASSED",
                0) == 0)
        {
            return State::Passed;
        }

        return State::Unknown;
    }


    State read_state(
        std::uint64_t hash)
    {
        std::ifstream in(state_path_);

        if (!in) {
            return State::Unknown;
        }

        std::string line;
        State result = State::Unknown;

        while (std::getline(in, line)) {
            const State current =
                parse_state(line, hash);

            if (current != State::Unknown) {
                result = current;
            }
        }

        return result;
    }


    void append_state(
        std::uint64_t hash,
        const char* state)
    {
        const int fd =
            ::open(
                state_path_.c_str(),
                O_WRONLY |
                O_CREAT |
                O_APPEND |
                O_CLOEXEC,
                0600);

        if (fd < 0) {
            throw std::runtime_error(
                "cannot open quarantine state");
        }

        std::ostringstream line;

        line << hash_string(hash)
             << ','
             << state
             << '\n';

        const std::string text =
            line.str();

        std::size_t written = 0;

        while (written < text.size()) {
            const ssize_t n =
                ::write(
                    fd,
                    text.data() + written,
                    text.size() - written);

            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }

                ::close(fd);

                throw std::runtime_error(
                    "quarantine state write failed");
            }

            if (n == 0) {
                ::close(fd);

                throw std::runtime_error(
                    "quarantine state write made no progress");
            }

            written +=
                static_cast<std::size_t>(n);
        }

        if (::fsync(fd) != 0) {
            ::close(fd);

            throw std::runtime_error(
                "quarantine state fsync failed");
        }

        if (::close(fd) != 0) {
            throw std::runtime_error(
                "quarantine state close failed");
        }
    }


public:
    QuarantineRegistry(
        std::filesystem::path state,
        std::filesystem::path lock)
        : state_path_(std::move(state)),
          lock_path_(std::move(lock))
    {
        if (state_path_.empty()) {
            throw std::invalid_argument(
                "empty quarantine state path");
        }

        if (lock_path_.empty()) {
            throw std::invalid_argument(
                "empty quarantine lock path");
        }
    }


    /*
     * Atomic Check-and-Claim operation.
     *
     * The lock covers both the state read and the state transition,
     * preventing two local processes from simultaneously observing
     * UNKNOWN and both transitioning to RUNNING.
     */
    ClaimResult claim(
        std::uint64_t mutation_hash)
    {
        try {
            ScopedLock lock(lock_path_);

            const State current =
                read_state(mutation_hash);

            if (current == State::Quarantined) {
                return ClaimResult::AlreadyQuarantined;
            }

            if (current == State::Running) {
                return ClaimResult::AlreadyRunning;
            }

            /*
             * A PASSED artifact is eligible for a future execution
             * cycle under this state model.
             */
            append_state(
                mutation_hash,
                "RUNNING");

            return ClaimResult::Claimed;
        }
        catch (...) {
            return ClaimResult::Error;
        }
    }


    void quarantine(
        std::uint64_t mutation_hash)
    {
        ScopedLock lock(lock_path_);

        const State current =
            read_state(mutation_hash);

        if (current == State::Quarantined) {
            return;
        }

        append_state(
            mutation_hash,
            "QUARANTINED");
    }


    void release_success(
        std::uint64_t mutation_hash)
    {
        ScopedLock lock(lock_path_);

        append_state(
            mutation_hash,
            "PASSED");
    }


    /*
     * Compatibility entry point required by src/main.cpp.
     *
     * The existing caller supplies an error description and target
     * filename. The filename becomes the deterministic mutation
     * identity for the registry.
     */
    static void isolate_anomaly(
        const std::string& reason,
        const std::string& target_file)
    {
        if (reason.empty()) {
            throw std::invalid_argument(
                "quarantine reason is empty");
        }

        if (target_file.empty()) {
            throw std::invalid_argument(
                "quarantine target is empty");
        }

        /*
         * These filenames are deliberately kept local to the
         * registry API. Adjust them if your existing deployment
         * supplies dedicated registry paths.
         */
        QuarantineRegistry registry(
            "quarantine.state",
            "quarantine.lock");

        const std::uint64_t mutation_hash =
            hash_identity(target_file);

        const ClaimResult result =
            registry.claim(mutation_hash);

        switch (result) {
            case ClaimResult::Claimed:
                registry.quarantine(
                    mutation_hash);
                return;

            case ClaimResult::AlreadyQuarantined:
                return;

            case ClaimResult::AlreadyRunning:
                /*
                 * Another worker already owns the execution claim.
                 * Do not create a competing quarantine transition.
                 */
                return;

            case ClaimResult::Error:
            default:
                throw std::runtime_error(
                    "unable to establish quarantine state");
        }
    }
};

} // namespace Swayam

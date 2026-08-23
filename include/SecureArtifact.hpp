// SecureArtifact.hpp
#pragma once

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Swayam {

class UniqueFd {
    int fd_{-1};

public:
    UniqueFd() = default;
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept
        : fd_(std::exchange(other.fd_, -1)) {}

    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    ~UniqueFd() { reset(); }

    int get() const noexcept { return fd_; }
    bool valid() const noexcept { return fd_ >= 0; }

    int release() noexcept { return std::exchange(fd_, -1); }

    void reset(int fd = -1) noexcept {
        if (fd_ >= 0) ::close(fd_);
        fd_ = fd;
    }
};

struct StagedArtifact {
    UniqueFd fd;
    std::uint64_t size{};
    std::uint64_t hash{};
    std::string proc_path;
};

class SecureArtifact {
private:
    static std::uint64_t fnv1a_update(std::uint64_t hash, const std::uint8_t* data, std::size_t size) {
        constexpr std::uint64_t prime = 1099511628211ULL;
        for (std::size_t i = 0; i < size; ++i) {
            hash ^= data[i];
            hash *= prime;
        }
        return hash;
    }

public:
    static StagedArtifact open_and_hash(int trusted_dir_fd, const std::string& relative_name) {
        if (trusted_dir_fd < 0) throw std::invalid_argument("invalid trusted directory fd");
        if (relative_name.empty()) throw std::invalid_argument("empty artifact name");

        if (relative_name == "." || relative_name == ".." || relative_name.find('/') != std::string::npos)
            throw std::invalid_argument("artifact name must be a single path component");

        if (relative_name.front() == '-')
            throw std::invalid_argument("option-like artifact name");

        // Use O_NOFOLLOW to prevent symlink traversal attacks at descriptor level
        const int fd = ::openat(trusted_dir_fd, relative_name.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
        if (fd < 0) throw std::runtime_error("failed to open artifact via openat");

        UniqueFd artifact_fd(fd);
        struct stat st{};
        if (::fstat(artifact_fd.get(), &st) != 0)
            throw std::runtime_error("failed to retrieve artifact metadata via fstat");

        if (!S_ISREG(st.st_mode)) throw std::runtime_error("artifact is not a regular file");
        if (st.st_size < 0) throw std::runtime_error("negative artifact size");

        constexpr std::uint64_t MAX_SOURCE_SIZE = 8ULL * 1024ULL * 1024ULL;
        const auto size = static_cast<std::uint64_t>(st.st_size);
        if (size > MAX_SOURCE_SIZE) throw std::runtime_error("artifact exceeds maximum source size");

        if (::lseek(artifact_fd.get(), 0, SEEK_SET) == -1)
            throw std::runtime_error("failed to lseek artifact descriptor");

        constexpr std::size_t BUFFER_SIZE = 64 * 1024;
        std::vector<std::uint8_t> buffer(BUFFER_SIZE);
        std::uint64_t hash = 14695981039346656037ULL;
        std::uint64_t total = 0;

        for (;;) {
            const ssize_t n = ::read(artifact_fd.get(), buffer.data(), buffer.size());
            if (n == 0) break;
            if (n < 0) {
                if (errno == EINTR) continue;
                throw std::runtime_error("failed to read artifact data");
            }
            total += static_cast<std::uint64_t>(n);
            if (total > MAX_SOURCE_SIZE) throw std::runtime_error("artifact grew beyond maximum size");
            hash = fnv1a_update(hash, buffer.data(), static_cast<std::size_t>(n));
        }

        if (total != size) throw std::runtime_error("artifact size mismatch during read phase");

        if (::lseek(artifact_fd.get(), 0, SEEK_SET) == -1)
            throw std::runtime_error("failed to reset artifact descriptor lseek");

        const int flags = ::fcntl(artifact_fd.get(), F_GETFD);
        if (flags == -1) throw std::runtime_error("failed to get descriptor flags via fcntl");

        if (::fcntl(artifact_fd.get(), F_SETFD, flags & ~FD_CLOEXEC) == -1)
            throw std::runtime_error("failed to clear close-on-exec flag via fcntl");

        return StagedArtifact{
            std::move(artifact_fd),
            size,
            hash,
            "/proc/self/fd/" + std::to_string(artifact_fd.get())
        };
    }
};

} // namespace Swayam

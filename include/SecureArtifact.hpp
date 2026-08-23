#pragma once

#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

namespace Swayam {

/*
 * Owns one POSIX descriptor.
 *
 * Copying is disabled because ownership must remain unique.
 * Moving transfers ownership.
 */
class UniqueFd {
private:
    int fd_{-1};

public:
    UniqueFd() noexcept = default;

    explicit UniqueFd(int fd) noexcept
        : fd_(fd) {}

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

    ~UniqueFd() {
        reset();
    }

    [[nodiscard]]
    int get() const noexcept {
        return fd_;
    }

    [[nodiscard]]
    bool valid() const noexcept {
        return fd_ >= 0;
    }

    int release() noexcept {
        return std::exchange(fd_, -1);
    }

    void reset(int fd = -1) noexcept {
        if (fd_ >= 0) {
            (void)::close(fd_);
        }

        fd_ = fd;
    }
};


/*
 * Descriptor-backed immutable-in-practice compilation artifact.
 *
 * The compiler receives proc_fd_path rather than the original pathname.
 *
 * Therefore:
 *
 *     source pathname
 *          |
 *          v
 *       openat()
 *          |
 *          v
 *    descriptor-backed copy
 *          |
 *       hash bytes
 *          |
 *          v
 *   /proc/self/fd/N
 *          |
 *          v
 *       compiler
 */
struct StagedArtifact {
    UniqueFd descriptor;

    std::uint64_t byte_count{0};

    std::uint64_t content_hash{0};

    std::string proc_fd_path;
};


class SecureArtifact {
private:
    static constexpr std::uint64_t FNV_OFFSET =
        14695981039346656037ULL;

    static constexpr std::uint64_t FNV_PRIME =
        1099511628211ULL;

    static constexpr std::uint64_t MAX_ARTIFACT_SIZE =
        8ULL * 1024ULL * 1024ULL;

    static constexpr std::size_t BUFFER_SIZE =
        64ULL * 1024ULL;


    /*
     * Only a single filename component is accepted.
     *
     * The trusted directory is supplied separately as dir_fd.
     * This prevents this primitive from accepting:
     *
     *     ../x
     *     ../../x
     *     /absolute/path
     *     a/b
     *     -
     *
     * Directory traversal is therefore not represented by the
     * artifact-name argument.
     */
    static bool valid_component(
        const std::string& name)
    {
        if (name.empty()) {
            return false;
        }

        if (name == "." || name == "..") {
            return false;
        }

        if (name.front() == '-') {
            return false;
        }

        for (const unsigned char c : name) {
            const bool allowed =
                (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '_' ||
                c == '-' ||
                c == '.';

            if (!allowed) {
                return false;
            }
        }

        return true;
    }


    static std::uint64_t update_hash(
        std::uint64_t hash,
        const std::uint8_t* data,
        std::size_t size)
    {
        for (std::size_t i = 0; i < size; ++i) {
            hash ^= data[i];
            hash *= FNV_PRIME;
        }

        return hash;
    }


    static void seek_start(int fd) {
        if (::lseek(fd, 0, SEEK_SET) ==
            static_cast<off_t>(-1))
        {
            throw std::runtime_error(
                "artifact seek failed");
        }
    }


    static struct stat metadata(int fd) {
        struct stat st{};

        if (::fstat(fd, &st) != 0) {
            throw std::runtime_error(
                "artifact metadata query failed");
        }

        if (!S_ISREG(st.st_mode)) {
            throw std::runtime_error(
                "artifact is not a regular file");
        }

        if (st.st_size < 0) {
            throw std::runtime_error(
                "artifact has invalid size");
        }

        return st;
    }


    /*
     * Create an anonymous staging object.
     *
     * Linux O_TMPFILE prevents a pathname from being introduced for
     * the staged artifact. The descriptor is subsequently exposed
     * only through /proc/self/fd/N.
     */
    static int create_staging_fd(
        int directory_fd)
    {
#ifdef O_TMPFILE
        const int fd =
            ::openat(
                directory_fd,
                ".",
                O_RDWR |
                O_TMPFILE |
                O_CLOEXEC,
                0600);

        if (fd >= 0) {
            return fd;
        }
#endif

        /*
         * There is deliberately no pathname fallback here.
         *
         * A named temporary file would reintroduce a filesystem
         * namespace race into the staging boundary.
         */
        throw std::runtime_error(
            "anonymous artifact staging unavailable");
    }


    static void make_inheritable(int fd) {
        const int flags =
            ::fcntl(fd, F_GETFD);

        if (flags < 0) {
            throw std::runtime_error(
                "descriptor flag query failed");
        }

        if (::fcntl(
                fd,
                F_SETFD,
                flags & ~FD_CLOEXEC) < 0)
        {
            throw std::runtime_error(
                "descriptor inheritance setup failed");
        }
    }


public:
    SecureArtifact() = delete;


    /*
     * Opens the source relative to a trusted directory descriptor,
     * copies the exact consumed bytes into an anonymous descriptor,
     * hashes that descriptor-backed copy, and returns a compiler-safe
     * /proc/self/fd/N reference.
     *
     * The important invariant is:
     *
     *     bytes hashed == bytes compiled
     *
     * because both operations refer to the same staged descriptor.
     */
    [[nodiscard]]
    static StagedArtifact open_and_stage(
        int source_directory_fd,
        const std::string& source_name,
        int staging_directory_fd)
    {
        if (source_directory_fd < 0) {
            throw std::invalid_argument(
                "invalid source directory descriptor");
        }

        if (staging_directory_fd < 0) {
            throw std::invalid_argument(
                "invalid staging directory descriptor");
        }

        if (!valid_component(source_name)) {
            throw std::invalid_argument(
                "invalid artifact name");
        }


        /*
         * O_NOFOLLOW prevents the final source component from being
         * substituted with a symlink.
         */
        UniqueFd source(
            ::openat(
                source_directory_fd,
                source_name.c_str(),
                O_RDONLY |
                O_NOFOLLOW |
                O_CLOEXEC));

        if (!source.valid()) {
            throw std::runtime_error(
                "secure artifact open failed");
        }


        const struct stat source_initial =
            metadata(source.get());

        const std::uint64_t source_size =
            static_cast<std::uint64_t>(
                source_initial.st_size);

        if (source_size >
            MAX_ARTIFACT_SIZE)
        {
            throw std::runtime_error(
                "artifact exceeds configured size limit");
        }


        /*
         * The staged object is anonymous and descriptor-backed.
         */
        UniqueFd staged(
            create_staging_fd(
                staging_directory_fd));


        std::vector<std::uint8_t> buffer(
            BUFFER_SIZE);

        std::uint64_t total = 0;

        std::uint64_t hash =
            FNV_OFFSET;


        /*
         * Copy and hash in the same pass.
         *
         * The bytes written to the staging descriptor are exactly
         * the bytes incorporated into the hash.
         */
        for (;;) {
            const ssize_t count =
                ::read(
                    source.get(),
                    buffer.data(),
                    buffer.size());

            if (count == 0) {
                break;
            }

            if (count < 0) {
                if (errno == EINTR) {
                    continue;
                }

                throw std::runtime_error(
                    "artifact read failed");
            }

            const std::uint64_t amount =
                static_cast<std::uint64_t>(
                    count);

            if (amount >
                MAX_ARTIFACT_SIZE - total)
            {
                throw std::runtime_error(
                    "artifact size limit exceeded");
            }

            std::size_t offset = 0;

            while (offset <
                   static_cast<std::size_t>(count))
            {
                const ssize_t written =
                    ::write(
                        staged.get(),
                        buffer.data() + offset,
                        static_cast<std::size_t>(count) -
                            offset);

                if (written < 0) {
                    if (errno == EINTR) {
                        continue;
                    }

                    throw std::runtime_error(
                        "artifact staging write failed");
                }

                if (written == 0) {
                    throw std::runtime_error(
                        "artifact staging made no progress");
                }

                offset +=
                    static_cast<std::size_t>(
                        written);
            }

            hash =
                update_hash(
                    hash,
                    buffer.data(),
                    static_cast<std::size_t>(
                        count));

            total += amount;
        }


        /*
         * The original source descriptor is checked again.
         *
         * This does not matter for the TOCTOU property of the staged
         * artifact—the compiler will never consume the source file.
         *
         * It does provide useful detection telemetry if the original
         * artifact was replaced while it was being copied.
         */
        const struct stat source_final =
            metadata(source.get());

        if (source_final.st_dev !=
                source_initial.st_dev ||
            source_final.st_ino !=
                source_initial.st_ino)
        {
            throw std::runtime_error(
                "source artifact identity changed");
        }

        if (source_final.st_size !=
            source_initial.st_size)
        {
            throw std::runtime_error(
                "source artifact size changed");
        }


        if (total != source_size) {
            throw std::runtime_error(
                "artifact length invariant failed");
        }


        /*
         * Flush the staged bytes before they become an execution
         * input.
         */
        if (::fsync(staged.get()) != 0) {
            throw std::runtime_error(
                "artifact staging sync failed");
        }


        /*
         * Rewind the descriptor so the compiler starts at byte zero.
         */
        seek_start(staged.get());


        /*
         * The compiler is started by a separate exec() process.
         * Remove close-on-exec only after the artifact has been
         * completely validated and staged.
         */
        make_inheritable(staged.get());


        const std::string proc_path =
            "/proc/self/fd/" +
            std::to_string(staged.get());


        return StagedArtifact{
            std::move(staged),
            total,
            hash,
            proc_path
        };
    }
};

} // namespace Swayam

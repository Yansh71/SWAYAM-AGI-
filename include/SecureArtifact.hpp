#ifndef SWAYAM_SECURE_ARTIFACT_HPP
#define SWAYAM_SECURE_ARTIFACT_HPP

#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

namespace Swayam {

/*
 * RAII wrapper for a POSIX file descriptor.
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

    void reset(int replacement = -1) noexcept {
        if (fd_ >= 0) {
            (void)::close(fd_);
        }

        fd_ = replacement;
    }

    [[nodiscard]]
    int release() noexcept {
        return std::exchange(fd_, -1);
    }
};


/*
 * Immutable descriptor-backed artifact.
 *
 * The descriptor remains open for the lifetime of this object.
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

    /*
     * Maximum source artifact accepted by this primitive.
     */
    static constexpr std::uint64_t MAX_ARTIFACT_SIZE =
        8ULL * 1024ULL * 1024ULL;

    static constexpr std::size_t BUFFER_SIZE =
        64ULL * 1024ULL;


    /*
     * Only a single filename component is accepted.
     *
     * This intentionally rejects:
     *
     *   .
     *   ..
     *   /
     *   ../
     *   absolute paths
     *   nested paths
     *
     * The trusted directory is supplied separately as dir_fd.
     */
    static bool valid_component(
        const std::string& name)
    {
        if (name.empty()) {
            return false;
        }

        if (name == "." ||
            name == "..")
        {
            return false;
        }

        if (name.front() == '-') {
            return false;
        }

        for (const unsigned char c : name) {
            const bool permitted =
                (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '_' ||
                c == '-' ||
                c == '.';

            if (!permitted) {
                return false;
            }
        }

        return true;
    }


    /*
     * FNV-1a update.
     */
    static std::uint64_t update_hash(
        std::uint64_t hash,
        const std::uint8_t* data,
        std::size_t length)
    {
        for (std::size_t i = 0;
             i < length;
             ++i)
        {
            hash ^= data[i];
            hash *= FNV_PRIME;
        }

        return hash;
    }


    /*
     * Reposition the descriptor at the beginning.
     */
    static void rewind_descriptor(
        int fd)
    {
        if (::lseek(
                fd,
                static_cast<off_t>(0),
                SEEK_SET) ==
            static_cast<off_t>(-1))
        {
            throw std::runtime_error(
                "artifact seek failed");
        }
    }


    /*
     * Remove close-on-exec after validation.
     *
     * The compiler process must inherit the descriptor so that
     * /proc/self/fd/N remains usable after exec.
     */
    static void make_inheritable(
        int fd)
    {
        const int flags =
            ::fcntl(
                fd,
                F_GETFD);

        if (flags < 0) {
            throw std::runtime_error(
                "descriptor flag query failed");
        }

        const int updated =
            flags & ~FD_CLOEXEC;

        if (::fcntl(
                fd,
                F_SETFD,
                updated) < 0)
        {
            throw std::runtime_error(
                "descriptor inheritance setup failed");
        }
    }


    /*
     * Verify that the descriptor still represents a regular file.
     */
    static struct stat read_metadata(
        int fd)
    {
        struct stat metadata{};

        if (::fstat(
                fd,
                &metadata) != 0)
        {
            throw std::runtime_error(
                "artifact metadata query failed");
        }

        if (!S_ISREG(metadata.st_mode)) {
            throw std::runtime_error(
                "artifact is not a regular file");
        }

        if (metadata.st_size < 0) {
            throw std::runtime_error(
                "invalid artifact size");
        }

        return metadata;
    }


public:
    SecureArtifact() = delete;


    /*
     * Open and hash an artifact relative to a trusted directory
     * descriptor.
     *
     * Security properties:
     *
     * 1. No pathname traversal is accepted by this primitive.
     * 2. Resolution is relative to dir_fd.
     * 3. O_NOFOLLOW prevents the final component from being a symlink.
     * 4. The opened descriptor, not the pathname, is hashed.
     * 5. The descriptor remains open after validation.
     * 6. /proc/self/fd/N identifies that descriptor.
     */
    [[nodiscard]]
    static StagedArtifact open_and_hash(
        int dir_fd,
        const std::string& name)
    {
        if (dir_fd < 0) {
            throw std::invalid_argument(
                "invalid directory descriptor");
        }

        if (!valid_component(name)) {
            throw std::invalid_argument(
                "invalid artifact name");
        }


        /*
         * O_CLOEXEC is initially enabled while validation is performed.
         *
         * It is explicitly removed only after the descriptor has been
         * completely validated and hashed.
         */
        const int raw_fd =
            ::openat(
                dir_fd,
                name.c_str(),
                O_RDONLY |
                O_NOFOLLOW |
                O_CLOEXEC);

        if (raw_fd < 0) {
            throw std::runtime_error(
                "artifact open failed");
        }

        UniqueFd descriptor(raw_fd);


        /*
         * First metadata snapshot.
         */
        const struct stat initial =
            read_metadata(
                descriptor.get());


        const std::uint64_t expected_size =
            static_cast<std::uint64_t>(
                initial.st_size);


        if (expected_size >
            MAX_ARTIFACT_SIZE)
        {
            throw std::runtime_error(
                "artifact exceeds configured size limit");
        }


        /*
         * Hash exactly what is read through the descriptor.
         */
        rewind_descriptor(
            descriptor.get());


        std::vector<std::uint8_t> buffer(
            BUFFER_SIZE);

        std::uint64_t total = 0;

        std::uint64_t hash =
            FNV_OFFSET;


        for (;;) {
            const ssize_t count =
                ::read(
                    descriptor.get(),
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


            const auto current =
                static_cast<std::uint64_t>(
                    count);


            if (current >
                MAX_ARTIFACT_SIZE - total)
            {
                throw std::runtime_error(
                    "artifact exceeded size limit");
            }


            total += current;


            hash =
                update_hash(
                    hash,
                    buffer.data(),
                    static_cast<std::size_t>(
                        count));
        }


        /*
         * The number of bytes actually consumed must agree with the
         * initial descriptor metadata.
         */
        if (total != expected_size) {
            throw std::runtime_error(
                "artifact length invariant failed");
        }


        /*
         * Second metadata snapshot.
         *
         * This detects ordinary changes to the descriptor's underlying
         * inode while the artifact was being consumed.
         */
        const struct stat final =
            read_metadata(
                descriptor.get());


        if (final.st_dev != initial.st_dev ||
            final.st_ino != initial.st_ino)
        {
            throw std::runtime_error(
                "artifact identity changed");
        }


        if (final.st_size != initial.st_size) {
            throw std::runtime_error(
                "artifact size changed");
        }


        /*
         * Reset the descriptor so the subsequent consumer starts from
         * byte zero.
         */
        rewind_descriptor(
            descriptor.get());


        /*
         * The descriptor must survive exec().
         */
        make_inheritable(
            descriptor.get());


        const std::string proc_fd_path =
            "/proc/self/fd/" +
            std::to_string(
                descriptor.get());


        return StagedArtifact{
            std::move(descriptor),
            total,
            hash,
            proc_fd_path
        };
    }
};

} // namespace Swayam

#endif // SWAYAM_SECURE_ARTIFACT_HPP

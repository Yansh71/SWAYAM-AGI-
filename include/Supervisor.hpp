// Supervisor.hpp
#pragma once

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <stdexcept>
#include <thread>

#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace Swayam {

enum class SupervisorResult {
    Success,
    ChildExitFailure,
    ChildSignaled,
    Timeout,
    WaitFailure,
    TerminationFailure
};

struct SupervisorOutcome {
    SupervisorResult result{};
    int status{};
    bool child_reaped{false};
};

class ProcessSupervisor {
private:
    using Clock = std::chrono::steady_clock;

    static bool wait_until(pid_t pid, int& status, Clock::time_point deadline) {
        for (;;) {
            const pid_t r = ::waitpid(pid, &status, WNOHANG);
            if (r == pid) return true;
            if (r == -1) {
                if (errno == EINTR) continue;
                return false;
            }
            if (Clock::now() >= deadline) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    static bool reap_blocking(pid_t pid, int& status) {
        for (;;) {
            const pid_t r = ::waitpid(pid, &status, 0);
            if (r == pid) return true;
            if (r == -1 && errno == EINTR) continue;
            return false;
        }
    }

    static bool terminate_group(pid_t pgid, std::chrono::milliseconds grace) {
        if (::killpg(pgid, SIGTERM) == -1) {
            if (errno != ESRCH) return false;
        }

        const auto deadline = Clock::now() + grace;
        while (Clock::now() < deadline) {
            if (::killpg(pgid, 0) == -1) {
                if (errno == ESRCH) return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        if (::killpg(pgid, SIGKILL) == -1) {
            if (errno != ESRCH) return false;
        }
        return true;
    }

public:
    static SupervisorOutcome supervise(
        pid_t pid,
        pid_t pgid,
        std::chrono::milliseconds timeout,
        std::chrono::milliseconds grace = std::chrono::milliseconds(250))
    {
        if (pid <= 0 || pgid <= 0) throw std::invalid_argument("invalid process identifiers");

        int status = 0;
        const auto deadline = Clock::now() + timeout;

        if (wait_until(pid, status, deadline)) {
            if (WIFEXITED(status)) {
                return {
                    WEXITSTATUS(status) == 0 ? SupervisorResult::Success : SupervisorResult::ChildExitFailure,
                    status,
                    true
                };
            }
            if (WIFSIGNALED(status)) {
                return { SupervisorResult::ChildSignaled, status, true };
            }
            return { SupervisorResult::WaitFailure, status, true };
        }

        if (Clock::now() < deadline) {
            return { SupervisorResult::WaitFailure, status, false };
        }

        if (!terminate_group(pgid, grace)) {
            return { SupervisorResult::TerminationFailure, status, false };
        }

        if (!reap_blocking(pid, status)) {
            return { SupervisorResult::WaitFailure, status, false };
        }

        return { SupervisorResult::Timeout, status, true };
    }
};

} // namespace Swayam


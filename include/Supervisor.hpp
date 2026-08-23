#pragma once

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace Swayam {

enum class SupervisorResult {
    ExitedZero,
    ExitedNonZero,
    Signaled,
    TimedOut,
    Failed
};


struct SupervisorConfig {
    std::chrono::milliseconds timeout{
        std::chrono::seconds(10)
    };

    std::chrono::milliseconds termination_grace{
        std::chrono::milliseconds(500)
    };

    std::chrono::milliseconds poll_interval{
        std::chrono::milliseconds(10)
    };
};


class Supervisor {
private:
    static std::chrono::steady_clock::time_point deadline_after(
        std::chrono::milliseconds duration)
    {
        return std::chrono::steady_clock::now() +
               duration;
    }


    static bool wait_once(
        pid_t pid,
        int& status)
    {
        for (;;) {
            const pid_t result =
                ::waitpid(
                    pid,
                    &status,
                    WNOHANG);

            if (result == pid) {
                return true;
            }

            if (result == 0) {
                return false;
            }

            if (result < 0 &&
                errno == EINTR)
            {
                continue;
            }

            throw std::runtime_error(
                "waitpid supervision failed");
        }
    }


    static void terminate_group(
        pid_t process_group,
        const SupervisorConfig& config)
    {
        /*
         * First stage: request cooperative termination.
         */
        if (::killpg(
                process_group,
                SIGTERM) != 0)
        {
            if (errno != ESRCH) {
                /*
                 * The process may already have exited. ESRCH is
                 * therefore treated as an ordinary termination race.
                 */
            }
        }


        const auto grace_deadline =
            deadline_after(
                config.termination_grace);


        /*
         * Give the group a bounded opportunity to exit.
         */
        for (;;) {
            if (::killpg(
                    process_group,
                    0) != 0)
            {
                if (errno == ESRCH) {
                    return;
                }

                if (errno == EINTR) {
                    continue;
                }

                break;
            }


            if (std::chrono::steady_clock::now() >=
                grace_deadline)
            {
                break;
            }


            std::this_thread::sleep_for(
                config.poll_interval);
        }


        /*
         * Second stage: force termination.
         */
        for (;;) {
            if (::killpg(
                    process_group,
                    SIGKILL) == 0)
            {
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            if (errno == ESRCH) {
                break;
            }

            break;
        }
    }


    static SupervisorResult classify(
        int status)
    {
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) == 0) {
                return SupervisorResult::ExitedZero;
            }

            return SupervisorResult::ExitedNonZero;
        }

        if (WIFSIGNALED(status)) {
            return SupervisorResult::Signaled;
        }

        return SupervisorResult::Failed;
    }


public:
    Supervisor() = delete;


    /*
     * Execute a caller-supplied child body under a bounded
     * process-group supervisor.
     *
     * The child must never return to the parent's control flow.
     */
    template <typename ChildFunction>
    static SupervisorResult run(
        ChildFunction&& child_function,
        const SupervisorConfig& config = {})
    {
        if (config.timeout.count() <= 0) {
            throw std::invalid_argument(
                "supervisor timeout must be positive");
        }

        if (config.termination_grace.count() < 0) {
            throw std::invalid_argument(
                "termination grace cannot be negative");
        }

        if (config.poll_interval.count() <= 0) {
            throw std::invalid_argument(
                "poll interval must be positive");
        }


        const pid_t child =
            ::fork();

        if (child < 0) {
            throw std::runtime_error(
                "supervisor fork failed");
        }


        if (child == 0) {
            /*
             * Establish a dedicated process group.
             *
             * The child becomes the group leader. Descendants created
             * normally by the mutation inherit this process group.
             */
            if (::setpgid(0, 0) != 0) {
                _exit(125);
            }


            try {
                child_function();
                _exit(0);
            }
            catch (...) {
                _exit(126);
            }
        }


        /*
         * Parent-side race closure:
         *
         * The child may have executed setpgid() already, or the parent
         * may reach this point first. Either result is acceptable.
         */
        for (;;) {
            if (::setpgid(child, child) == 0) {
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            if (errno == EACCES ||
                errno == ESRCH)
            {
                break;
            }

            break;
        }


        int status = 0;

        const auto deadline =
            deadline_after(
                config.timeout);


        /*
         * Bounded WNOHANG supervision loop.
         */
        for (;;) {
            if (wait_once(
                    child,
                    status))
            {
                return classify(status);
            }


            if (std::chrono::steady_clock::now() >=
                deadline)
            {
                /*
                 * The child exceeded its execution budget.
                 *
                 * Terminate the entire process group rather than only
                 * the direct child.
                 */
                terminate_group(
                    child,
                    config);


                /*
                 * Final reap.
                 *
                 * This is intentionally blocking only after the
                 * process group has been forcefully terminated.
                 */
                for (;;) {
                    const pid_t result =
                        ::waitpid(
                            child,
                            &status,
                            0);

                    if (result == child) {
                        return SupervisorResult::TimedOut;
                    }

                    if (result < 0 &&
                        errno == EINTR)
                    {
                        continue;
                    }

                    return SupervisorResult::Failed;
                }
            }


            std::this_thread::sleep_for(
                config.poll_interval);
        }
    }
};

} // namespace Swayam

#include "../include/core.hpp"
#include "../include/Supervisor.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <fstream>
#include <system_error>

namespace Swayam {
    volatile sig_atomic_t daemon_running = 1;
    void signal_handler(int) { daemon_running = 0; }
}

static void write_status(const std::string& outcome, 
                          const std::string& reason = "") {
    std::ofstream f("/home/swayam_agent/workspace/meta/status.json");
    if (!f) return;
    f << "{\n";
    f << "  \"outcome\": \"" << outcome << "\"";
    if (!reason.empty())
        f << ",\n  \"reason\": \"" << reason << "\"";
    f << "\n}\n";
}

int main() {
    std::cout << "[SWAYAM-AGI] Initializing Sovereign Core...\n";

    struct sigaction sa;
    sa.sa_handler = Swayam::signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1 ||
        sigaction(SIGTERM, &sa, nullptr) == -1) {
        std::cerr << "[SWAYAM-AGI] FATAL: Failed to hook OS signals.\n";
        write_status("quarantined", "signal hook failure");
        return EXIT_FAILURE;
    }

    // FIX: Use /tmp — only writable path in read-only container
    int lock_fd = open("/tmp/.swayam_daemon.lock",
                       O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (lock_fd == -1) {
        std::cerr << "[SWAYAM-AGI] FATAL: Cannot access lock directory.\n";
        write_status("quarantined", "lock file inaccessible");
        return EXIT_FAILURE;
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) == -1) {
        std::cerr << "[SWAYAM-AGI] FATAL: Another instance running.\n";
        close(lock_fd);
        write_status("quarantined", "concurrent instance detected");
        return EXIT_FAILURE;
    }

    Swayam::AtomicGuard core_guard;
    const std::string base_algorithm =
        "int main() {\n    // [SWAYAM_BASE_GENOME]\n    return 0;\n}";

    std::cout << "[SWAYAM-AGI] Core Online. Entering Evolution Loop...\n";

    bool evolution_succeeded = false;

    while (Swayam::daemon_running) {
        try {
            Swayam::Supervisor::orchestrate_evolution(
                core_guard, base_algorithm);
            evolution_succeeded = true;
            break; // One cycle per container run
        } catch (const std::exception& e) {
            std::cerr << "[SWAYAM-AGI] Exception: " << e.what() << "\n";
            write_status("quarantined", e.what());
            flock(lock_fd, LOCK_UN);
            close(lock_fd);
            return EXIT_FAILURE;
        } catch (...) {
            std::cerr << "[SWAYAM-AGI] Unknown anomaly.\n";
            write_status("quarantined", "unknown exception");
            flock(lock_fd, LOCK_UN);
            close(lock_fd);
            return EXIT_FAILURE;
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    if (evolution_succeeded) {
        write_status("published");
    } else {
        write_status("quarantined", "daemon stopped before completion");
    }

    flock(lock_fd, LOCK_UN);
    close(lock_fd);

    std::cout << "[SWAYAM-AGI] Daemon Offline. Trust Seal Intact.\n";
    return EXIT_SUCCESS;
}

// =============================================================
// SWAYAM-AGI: The Apex Cybernetic Organism
// ENTRY POINT: main.cpp
// 
// ARCHITECTURE ENFORCEMENT: Perpetual Autonomous Daemon.
// KILLS DEADLOCKS: Strict POSIX Async-Signal-Safe shutdown logic.
// KILLS CLONE WARS: Single-instance hardware-level file locking.
// =============================================================
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
#include <system_error>

namespace Swayam {
    // THE APEX FIX: The ONLY mathematically safe way to handle OS signals in C++
    volatile sig_atomic_t daemon_running = 1;

    // Async-Signal-Safe Handler: No locks, no heap allocation
    void signal_handler(int /*signum*/) {
        daemon_running = 0; 
    }
}

int main() {
    std::cout << "[SWAYAM-AGI] Initializing Sovereign Core...\n";

    // 1. KERNEL SIGNAL HOOKS (Graceful Shutdown)
    struct sigaction sa;
    sa.sa_handler = Swayam::signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // Immediate interrupt, no auto-restart
    
    if (sigaction(SIGINT, &sa, nullptr) == -1 || sigaction(SIGTERM, &sa, nullptr) == -1) {
        std::cerr << "[SWAYAM-AGI] FATAL: Failed to hook OS signals.\n";
        return EXIT_FAILURE;
    }

    // 2. THE SINGLETON ENCLAVE LOCK (Prevents multiple AGI instances)
    int lock_fd = open(".swayam_daemon.lock", O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (lock_fd == -1) {
        std::cerr << "[SWAYAM-AGI] FATAL: Cannot access lock directory.\n";
        return EXIT_FAILURE;
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) == -1) {
        std::cerr << "[SWAYAM-AGI] FATAL: Another AGI Instance is already mutating reality. Aborting.\n";
        close(lock_fd);
        return EXIT_FAILURE;
    }

    // 3. CORE IGNITION
    Swayam::AtomicGuard core_guard;
    const std::string base_algorithm = "int main() {\n    // [SWAYAM_BASE_GENOME]\n    return 0;\n}";

    std::cout << "[SWAYAM-AGI] Core Online. Entering Perpetual Evolution Loop...\n";
    std::cout << "========================================================\n";

    // 4. THE PERPETUAL EVOLUTION DAEMON
    while (Swayam::daemon_running) {
        try {
            // Initiate the cognitive mutation & execution pipeline
            Swayam::Supervisor::orchestrate_evolution(core_guard, base_algorithm);
            
        } catch (const std::exception& e) {
            std::cerr << "[SWAYAM-AGI] Core Exception Intercepted: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[SWAYAM-AGI] Unknown Anomaly Intercepted in Evolution Engine.\n";
        }
        
        // Rate-Limiting Backoff: Prevents 100% CPU exhaustion & GitHub API bans
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    // 5. GRACEFUL DEMATERIALIZATION
    std::cout << "\n========================================================\n";
    std::cout << "[SWAYAM-AGI] OS Termination Signal Received.\n";
    std::cout << "[SWAYAM-AGI] Commencing Graceful Shutdown Sequence...\n";
    
    // Release the physical hardware lock
    flock(lock_fd, LOCK_UN);
    close(lock_fd);
    
    std::cout << "[SWAYAM-AGI] Daemon Offline. Trust Seal Intact.\n";
    return EXIT_SUCCESS;
}

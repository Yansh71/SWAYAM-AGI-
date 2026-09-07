    // KILLS WEAKNESS 4: Dynamic Timeout to prevent False-Positive DoS
    static void secure_watchdog_monitor(pid_t monitored_pid, int max_timeout_ms) noexcept {
        int status = 0;
        pid_t wpid;
        int timeout_counter = 0;
        const int sleep_interval_ms = 100; 
        const int max_ticks = max_timeout_ms / sleep_interval_ms;

        do {
            wpid = waitpid(monitored_pid, &status, WNOHANG);
            
            if (wpid == 0) {
                usleep(sleep_interval_ms * 1000); 
                timeout_counter++;

                if (timeout_counter > max_ticks) {
                    std::cerr << "[SWAYAM-WATCHDOG] ALERT: Timeout exceeded. Terminating rogue process...\n";
                    killpg(monitored_pid, SIGTERM);
                    usleep(100000);
                    killpg(monitored_pid, SIGKILL);
                    break;
                }
            }
        } while (wpid == 0 || (wpid == -1 && errno == EINTR));
    }

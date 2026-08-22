// scripts/testing/supervisor_main.cpp
// CLI wrapper around Swayam::run_guarded_mutation so run_sandbox.sh can
// validate a compiled mutation through the fork/waitpid crash-safe
// supervisor instead of running it directly and losing the lock on a crash.

#include "core.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "usage: supervisor <mutation_binary> <mutation_source_path>\n";
        return 2;
    }

    Swayam::AtomicGuard guard;
    try {
        bool ok = Swayam::run_guarded_mutation(guard, argv[1], argv[2]);
        return ok ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "[VENOMICA FATAL] " << e.what() << "\n";
        return 2;
    }
}

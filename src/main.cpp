#include <iostream>
#include "swayam/core.hpp"

int main() {
    swayam::SovereignAgenticCore core;

    std::cout << "[SWAYAM-AGI Core] System Online.\n";

    auto result = core.execute_agentic_task([]() {
        std::cout << "[SWAYAM-AGI Core] Executing agentic task logic...\n";
    });

    if (result.has_value()) {
        std::cout << "[SWAYAM-AGI Core] Task executed successfully!\n";
    } else {
        std::cout << "[SWAYAM-AGI Core] Execution failed with error code.\n";
    }

    return 0;
}

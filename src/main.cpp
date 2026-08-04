#include "swayam_core.hpp"

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "   SWAYAM-AGI Sovereign Kernel Runtime    " << std::endl;
    std::cout << "==========================================" << std::endl;

    auto engine = std::make_unique<Swayam::CoreEngine>();
    engine->executeSovereignKernel();

    return 0;
}

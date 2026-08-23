#ifndef SWAYAM_GIT_CORTEX_HPP
#define SWAYAM_GIT_CORTEX_HPP

#include <iostream>
#include <string>
#include <cstdlib>

namespace Swayam {

class GitCortex {
public:
    // Autonomously commits and pushes successful mutations back to the Matrix (GitHub)
    static bool publish_successful_mutation(const std::string& mutation_file) {
        std::cout << "[VENOMICA-CORTEX] Engaging Git Cortex. Preparing to assimilate successful evolution...\n";
        
        // Construct the autonomous Git command sequence
        // Note: CI/CD runner needs 'permissions: contents: write' which we already have in our YAML
        std::string command = 
            "git config --local user.name 'venomica-cortex[bot]' && "
            "git config --local user.email 'venomica-cortex[bot]@users.noreply.github.com' && "
            "git add " + mutation_file + " && "
            "git commit -m \"[VENOMICA-AUTONOMY] Quantum Mutation Successfully Assimilated\" && "
            "git push origin HEAD:main";

        int status = std::system(command.c_str());
        
        if (status == 0) {
            std::cout << "[VENOMICA-CORTEX] SUCCESS! Mutation successfully assimilated into the Repository.\n";
            return true;
        } else {
            std::cerr << "[VENOMICA-CORTEX WARN] Failed to push mutation. (Push might be blocked by branch protection or nothing new to commit).\n";
            return false;
        }
    }
};

} // namespace Swayam

#endif // SWAYAM_GIT_CORTEX_HPP

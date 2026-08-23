#ifndef SWAYAM_ERROR_HPP
#define SWAYAM_ERROR_HPP

#include <stdexcept>
#include <string>

namespace Swayam {

// [VENOMICA EXCEPTION KERNEL]
// Upgraded from static enums to a dynamic architectural fault handler.
class CoreFault : public std::runtime_error {
private:
    int kernel_exit_code;

public:
    // Captures the exact forensic state of the anomaly
    CoreFault(const std::string& forensic_message, int exit_code = 1) 
        : std::runtime_error("[VENOMICA KERNEL FAULT] " + forensic_message), 
          kernel_exit_code(exit_code) {}
    
    // Allows the QuarantineRegistry to read the exact kernel block code
    int get_exit_code() const { 
        return kernel_exit_code; 
    }
};

} // namespace Swayam

#endif // SWAYAM_ERROR_HPP

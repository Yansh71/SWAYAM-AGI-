#ifndef SWAYAM_ERROR_HPP
#ifndef SWAYAM_ERROR_HPP
#define SWAYAM_ERROR_HPP

#include <cstdint>

namespace swayam {

enum class CoreError : std::uint32_t {
    Success = 0x0,
    Busy = 0x403,
    TaskThrew = 0x500
};

} // namespace swayam

#endif // SWAYAM_ERROR_HPP

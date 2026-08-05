#ifndef SWAYAM_CORE_HPP
#define SWAYAM_CORE_HPP

#include <atomic>
#include <concepts>
#include <expected>
#include <iostream>
#include "swayam/error.hpp"

namespace swayam {

class alignas(64) SovereignAgenticCore {
private:
    std::atomic<bool> is_ready_{true};

public:
    SovereignAgenticCore() = default;
    ~SovereignAgenticCore() = default;

    template <typename Task>
    requires std::invocable<Task>
    auto execute_agentic_task(Task&& task) -> std::expected<bool, CoreError> {
        bool expected = true;
        // Atomic lock-free compare-and-swap guard
        if (!is_ready_.compare_exchange_strong(expected, false, std::memory_order_acquire)) {
            return std::unexpected(CoreError::Busy);
        }

        try {
            task();
            is_ready_.store(true, std::memory_order_release);
            return true;
        } catch (...) {
            is_ready_.store(true, std::memory_order_release);
            return std::unexpected(CoreError::TaskThrew);
        }
    }
};

} // namespace swayam

#endif // SWAYAM_CORE_HPP

#pragma once

#include <cstdint>

constexpr uint32_t kPrayerInputIdleTimeoutMs = 3U * 60U * 1000U;

enum class PrayerInputWaitDecision : uint8_t {
    Continue,
    Refresh,
    Sleep,
};

// Selects page refresh or sleep without resetting the user-idle timer.
// 在不重置用户空闲计时的前提下选择刷新或休眠。
inline PrayerInputWaitDecision prayer_input_wait_decision(
    uint32_t refresh_elapsed_ms,
    uint32_t idle_elapsed_ms,
    uint32_t refresh_interval_ms)
{
    if (idle_elapsed_ms >= kPrayerInputIdleTimeoutMs) {
        return PrayerInputWaitDecision::Sleep;
    }
    if (refresh_interval_ms > 0U &&
        refresh_elapsed_ms >= refresh_interval_ms) {
        return PrayerInputWaitDecision::Refresh;
    }
    return PrayerInputWaitDecision::Continue;
}

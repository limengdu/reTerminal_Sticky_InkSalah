#pragma once

#include <cstdint>

constexpr uint32_t kPrayerInputIdleTimeoutMs = 3U * 60U * 1000U;

enum class PrayerInputWaitDecision : uint8_t {
    Continue,
    Refresh,
    Sleep,
};

// Returns elapsed time while treating a timestamp sampled just after "now" as
// zero elapsed time. This also preserves normal uint32_t timer wraparound.
// 计算经过时间；若事件时间刚好晚于当前采样时间，则按零毫秒处理，同时保留计时器回绕能力。
inline uint32_t prayer_input_elapsed_ms(uint32_t now_ms,
                                        uint32_t earlier_ms)
{
    const uint32_t elapsed_ms = now_ms - earlier_ms;
    return static_cast<int32_t>(elapsed_ms) < 0 ? 0U : elapsed_ms;
}

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

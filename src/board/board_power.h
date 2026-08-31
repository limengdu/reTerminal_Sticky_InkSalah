#pragma once

#include <cstdint>

#include "esp_err.h"

// Keeps the Sticky power latch active after startup.
// 在启动后保持 Sticky 的电源锁存状态。
esp_err_t board_power_init();

// Holds the board rails, powers down peripherals, and enters deep sleep.
// A zero timer disables timed wakeup; the second parameter adds GPIO7.
// 保持板级电源锁存、关闭外设并进入深睡；定时器为0时关闭定时唤醒，第二个参数用于加入GPIO7。
[[noreturn]] void board_power_enter_deep_sleep(
    uint64_t timer_wakeup_us,
    bool orientation_wakeup_enabled);

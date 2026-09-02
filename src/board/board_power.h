#pragma once

#include <cstdint>

#include "esp_err.h"

// Keeps the Sticky power latch active after startup.
// 在启动后保持 Sticky 的电源锁存状态。
esp_err_t board_power_init();

// Reports whether the current boot was caused by the IMU orientation pin.
// 返回本次启动是否由IMU姿态中断引脚唤醒。
bool board_power_woke_from_orientation();

// Holds the board rails and enters deep sleep with selected wake sources.
// A zero timer disables timed wakeup; the booleans enable IMU and touch wake.
// 保持板级电源锁存并进入深睡；定时器为0时关闭定时唤醒，两个布尔值分别开启IMU与触摸唤醒。
[[noreturn]] void board_power_enter_deep_sleep(
    uint64_t timer_wakeup_us,
    bool orientation_wakeup_enabled,
    bool touch_wakeup_enabled,
    uint8_t touch_wakeup_level);

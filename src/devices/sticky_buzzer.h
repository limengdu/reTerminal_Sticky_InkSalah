#pragma once

#include <cstdint>

#include "esp_err.h"
#include "sticky_buzzer_pattern.h"

// Initializes the onboard buzzer in a silent state.
// 初始化板载蜂鸣器，并保持静音状态。
esp_err_t sticky_buzzer_init();

// Plays one gentle rising three-note prayer reminder in the background.
// 在后台播放一次轻柔的三音上行礼拜提醒。
esp_err_t sticky_buzzer_play_prayer_chime();

// Stops the active reminder immediately.
// 立即停止当前提醒音。
esp_err_t sticky_buzzer_stop();

#pragma once

#include "canvas.h"
#include "prayer_settings_pages.h"
#include "settings_model.h"

enum class DeviceSettingsResult {
    Saved,
    Cancelled,
    WifiRequested,
    Failed,
};

// Opens the requested native settings entry and returns after a terminal action.
// 打开指定的设备原生设置入口，并在用户完成最终操作后返回。
DeviceSettingsResult device_settings_run(Canvas &canvas,
                                         SalahSettings &settings,
                                         bool first_time,
                                         DeviceSettingsPage initial_page = DeviceSettingsPage::Main);

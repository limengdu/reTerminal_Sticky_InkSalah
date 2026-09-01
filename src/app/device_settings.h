#pragma once

#include "canvas.h"
#include "settings_model.h"

enum class DeviceSettingsResult {
    Saved,
    Cancelled,
    WifiRequested,
    Failed,
};

// Runs the portrait touch settings flow and returns after a terminal action.
// 运行竖屏触摸设置流程，并在用户完成最终操作后返回。
DeviceSettingsResult device_settings_run(Canvas &canvas,
                                         SalahSettings &settings,
                                         bool first_time);

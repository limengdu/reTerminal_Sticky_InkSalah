#pragma once

#include "esp_err.h"
#include "settings_store.h"

// Runs the local setup page until one validated configuration is saved.
// 运行本地设置页面，直到保存一份通过校验的配置。
esp_err_t provisioning_portal_run(SalahSettings &saved_settings);

#pragma once

#include "esp_err.h"
#include "settings_store.h"

// Runs the Wi-Fi-only captive portal until validated credentials are saved.
// 运行仅用于Wi-Fi的强制门户，直到保存通过校验的凭据。
esp_err_t provisioning_portal_run(const SalahSettings *existing_settings,
                                  SalahSettings &saved_settings);

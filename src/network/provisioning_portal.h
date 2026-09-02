#pragma once

#include "esp_err.h"
#include "settings_store.h"

enum class ProvisioningPortalExit {
    Saved,
    Cancelled,
};

using ProvisioningPortalCancelCheck = bool (*)(void *context);

// Runs the Wi-Fi-only captive portal until credentials are saved or cancelled.
// 运行仅用于Wi-Fi的强制门户，直到保存凭据或由设备端取消。
esp_err_t provisioning_portal_run(const SalahSettings *existing_settings,
                                  SalahSettings &saved_settings,
                                  ProvisioningPortalExit &exit,
                                  ProvisioningPortalCancelCheck cancel_check,
                                  void *cancel_context);

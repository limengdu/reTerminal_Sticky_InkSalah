#pragma once

#include "esp_err.h"
#include "settings_model.h"

esp_err_t settings_store_load(SalahSettings &settings);
// Saves either Wi-Fi-only first-run state or a complete prayer profile.
// 保存仅完成配网的首次状态，或保存完整礼拜配置。
esp_err_t settings_store_save(const SalahSettings &settings);

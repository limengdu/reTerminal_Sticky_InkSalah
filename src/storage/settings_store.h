#pragma once

#include "esp_err.h"
#include "settings_model.h"

esp_err_t settings_store_load(SalahSettings &settings);
esp_err_t settings_store_save(const SalahSettings &settings);

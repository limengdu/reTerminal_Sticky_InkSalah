#pragma once

#include <cstdint>

struct SalahSettings {
    bool configured = false;
    char wifi_ssid[33] = {};
    char wifi_password[65] = {};
    char city[48] = {};
    char country[48] = {};
    int16_t utc_offset_minutes = 420;
    uint8_t calculation_method = 20U;
    uint8_t asr_school = 0U;
    bool sound_enabled = false;
};

void settings_store_defaults(SalahSettings &settings);
bool settings_store_valid(const SalahSettings &settings);

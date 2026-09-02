#pragma once

#include <cstdint>

#include "canvas.h"
#include "prayer_time.h"
#include "settings_model.h"

struct PrayerPageModel {
    const SalahSettings *settings = nullptr;
    const PrayerDay *yesterday = nullptr;
    const PrayerDay *today = nullptr;
    const PrayerDay *tomorrow = nullptr;
    PrayerNext next = {};
    PrayerWindow window = {};
    uint16_t current_minutes = 0U;
    bool online = false;
};

void prayer_page_render_setup(Canvas &canvas,
                              const char *ap_ssid,
                              const char *ap_password,
                              bool can_return);
bool prayer_page_setup_back_action_at(int x, int y, bool can_return);
void prayer_page_render_error(Canvas &canvas,
                              const char *title,
                              const char *message);
void prayer_page_render_schedule(Canvas &canvas,
                                 const PrayerPageModel &model);

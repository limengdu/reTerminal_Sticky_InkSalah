#include "battery_status_overlay.h"

#include <cstdio>
#include <cstring>

#include "app_log.h"
#include "battery_status_render.h"
#include "board_charger.h"
#include "canvas.h"
#include "esp_timer.h"
#include "sticky_battery.h"

namespace {

constexpr char kTag[] = "battery_overlay";
constexpr int64_t kReadIntervalUs = 60LL * 1000000LL;
constexpr int64_t kRetryIntervalUs = 10LL * 1000000LL;

bool s_valid = false;
int s_percent = 0;
int s_last_logged_percent = -1;
bool s_last_logged_external_power = false;
bool s_has_logged_external_power = false;
bool s_log_tag_registered = false;
int64_t s_next_read_us = 0;

void update_reading_if_due()
{
    const int64_t now_us = esp_timer_get_time();
    if (now_us < s_next_read_us) {
        return;
    }

    StickyBatteryReading reading = {};
    const esp_err_t result = sticky_battery_read(reading);
    s_valid = result == ESP_OK;
    if (s_valid) {
        s_percent = reading.percent;
    }
    s_next_read_us = now_us +
                     (s_valid ? kReadIntervalUs : kRetryIntervalUs);

    const bool external_power = board_charger_external_power_present();
    if ((s_valid && s_percent != s_last_logged_percent) ||
        !s_has_logged_external_power ||
        external_power != s_last_logged_external_power) {
        STICKY_LOGI(kTag,
                    "battery=display valid=%d percent=%d external_power=%d next_read_s=%u",
                    s_valid,
                    s_percent,
                    external_power,
                    static_cast<unsigned>(
                        (s_valid ? kReadIntervalUs : kRetryIntervalUs) /
                        1000000LL));
        s_last_logged_percent = s_percent;
        s_last_logged_external_power = external_power;
        s_has_logged_external_power = true;
    }
}

}  // namespace

void battery_status_overlay_clear(Canvas &canvas, bool sleep_layout)
{
    battery_status_render_clear(canvas, sleep_layout);
}

void battery_status_overlay_draw(Canvas &canvas, bool sleep_layout)
{
    if (!s_log_tag_registered) {
        app_log_register_tag(kTag);
        s_log_tag_registered = true;
    }
    update_reading_if_due();

    battery_status_render(canvas, s_valid, s_percent,
                           board_charger_external_power_present(), sleep_layout);
}

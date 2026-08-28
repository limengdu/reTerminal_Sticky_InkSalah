#include "app_log.h"
#include "board_charger.h"
#include "board_power.h"
#include "board_sensor_bus.h"
#include "board_shared_spi.h"
#include "canvas.h"
#include "prayer_app.h"
#include "sticky_battery.h"
#include "sticky_button.h"
#include "sticky_buzzer.h"
#include "sticky_display.h"
#include "sticky_imu.h"
#include "sticky_rtc.h"

#include "esp_err.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#ifndef STICKY_SALAH_VERSION
#define STICKY_SALAH_VERSION "unknown"
#endif

#ifndef STICKY_BUILD_PROFILE
#define STICKY_BUILD_PROFILE "unknown"
#endif

namespace {

constexpr char kTag[] = "sticky_salah";

[[noreturn]] void halt_after_error(const char *component, esp_err_t result)
{
    STICKY_LOGE(kTag,
                "boot=halt component=%s result=%s",
                component,
                esp_err_to_name(result));
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void require_component(const char *name, esp_err_t result)
{
    if (result != ESP_OK) {
        halt_after_error(name, result);
    }
}

}  // namespace

extern "C" void app_main()
{
    app_log_init();
    app_log_register_tag(kTag);
    STICKY_LOGI(kTag,
                "boot=start firmware=%s profile=%s",
                STICKY_SALAH_VERSION,
                STICKY_BUILD_PROFILE);

    require_component("nvs", nvs_flash_init());
    require_component("board_power", board_power_init());
    require_component("board_charger", board_charger_init());
    require_component("board_shared_spi", board_shared_spi_prepare());
    require_component("board_sensor_bus", board_sensor_bus_init());
    require_component("sticky_rtc",
                      sticky_rtc_init(board_sensor_i2c_bus()));
    require_component("sticky_battery",
                      sticky_battery_init(board_sensor_i2c_bus()));
    require_component("sticky_display", sticky_display_init());
    require_component("sticky_button", sticky_button_init());
    require_component("sticky_buzzer", sticky_buzzer_init());
    require_component("sticky_imu",
                      sticky_imu_init(board_sensor_i2c_bus()));

    Canvas *canvas = sticky_display_canvas();
    if (canvas == nullptr) {
        halt_after_error("canvas", ESP_ERR_INVALID_STATE);
    }
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        require_component("display_clear", sticky_display_clear());
    }
    prayer_app_run(*canvas);
}

#include "epaper_panel_priv.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// SSD2677 command, waveform and packed-pixel sequences follow the supplied Seeed reference.
// 新屏控制器命令、温度波形及2位像素编码沿用Seeed提供的参考驱动。
#define SSD2677_WIDTH 800U
#define SSD2677_HEIGHT 480U
#define SSD2677_1BPP_LEN (SSD2677_WIDTH * SSD2677_HEIGHT / 8U)
#define SSD2677_2BPP_LEN (SSD2677_1BPP_LEN * 2U)
#define SSD2677_STREAM_IN_BYTES 512U

static const char *TAG = "ssd2677";

static esp_err_t write_reg8(seeed_epaper_panel_t *panel, uint8_t command, uint8_t value)
{
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, command), TAG, "write command failed");
    return seeed_epaper_panel_write_data(panel, &value, 1);
}

static void pack_interleave(uint8_t high_plane, uint8_t low_plane, uint8_t *out)
{
    out[0] = 0;
    out[1] = 0;

    for (uint8_t i = 0; i < 8U; i++) {
        const uint8_t pair = (uint8_t)((((high_plane >> (7U - i)) & 0x01U) << 1U) |
                                       ((low_plane >> (7U - i)) & 0x01U));
        if (i < 4U) {
            out[0] |= (uint8_t)(pair << (6U - i * 2U));
        } else {
            out[1] |= (uint8_t)(pair << (14U - i * 2U));
        }
    }
}

static void pack_mono_byte_to_2bpp(uint8_t mono, uint8_t *out)
{
    out[0] = 0;
    out[1] = 0;

    for (uint8_t i = 0; i < 8U; i++) {
        const uint8_t bit = (uint8_t)((mono >> (7U - i)) & 0x01U);
        const uint8_t pair = bit != 0U ? 0x03U : 0x00U;
        if (i < 4U) {
            out[0] |= (uint8_t)(pair << (6U - i * 2U));
        } else {
            out[1] |= (uint8_t)(pair << (14U - i * 2U));
        }
    }
}

static uint8_t ssd2677_mono_full_waveform_for_temperature(uint8_t temp)
{
    /* 3D97 V2: EPD_Slow_Display_1BIT(). */
    if (temp <= 10U) {
        return 0xE8U;
    }
    if (temp <= 20U) {
        return 0xEBU;
    }
    if (temp <= 30U) {
        return 0xEEU;
    }
    if (temp <= 127U) {
        return 0xF1U;
    }
    return 0xE8U;
}

static uint8_t ssd2677_gray4_waveform_for_temperature(uint8_t temp)
{
    /* 3D97 V2: EPD_Four_Gray_Display(). */
    if (temp <= 15U) {
        return 0xF1U;
    }
    if (temp <= 127U) {
        return 0xFAU;
    }
    return 0xF1U;
}

static uint8_t ssd2677_partial_waveform_for_temperature(uint8_t temp)
{
    /* 3D97 V2: dis_partial_img() and Fast_WHITE_TO_A(). */
    if (temp <= 10U) {
        return 0x08U;
    }
    if (temp <= 20U) {
        return 0x0FU;
    }
    if (temp <= 30U) {
        return 0x19U;
    }
    if (temp <= 127U) {
        return 0x23U;
    }
    return 0x08U;
}

static esp_err_t ssd2677_read_temperature_raw(seeed_epaper_panel_t *panel, uint8_t *ret_temp)
{
    uint8_t temp = 0;

    ESP_RETURN_ON_FALSE(ret_temp != NULL, ESP_ERR_INVALID_ARG, TAG, "ret_temp is null");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, 0x40), TAG, "write temperature command failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_wait_ready(panel), TAG, "wait temperature failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_read_data(panel, &temp), TAG, "read temperature failed");

    *ret_temp = temp;
    return ESP_OK;
}

static esp_err_t ssd2677_init_otp(seeed_epaper_panel_t *panel)
{
    static const uint8_t psr[] = {0x2F, 0x0E};
    static const uint8_t booster[] = {0x0F, 0x8B, 0x93, 0xC1};
    static const uint8_t timing[] = {0x76, 0x76, 0x76, 0x5A, 0x9D, 0x8A, 0x76, 0x62};
    static const uint8_t resolution[] = {0x03, 0x20, 0x02, 0xA8};
    static const uint8_t gsst[] = {0x00, 0x00, 0x00, 0x00};

    ESP_RETURN_ON_ERROR(seeed_epaper_panel_reset(panel), TAG, "reset failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_wait_ready(panel), TAG, "wait after reset failed");

    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, 0x00), TAG, "write PSR failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_data(panel, psr, sizeof(psr)), TAG, "write PSR data failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_wait_ready(panel), TAG, "wait PSR failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, 0x06), TAG, "write booster failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_data(panel, booster, sizeof(booster)), TAG, "write booster data failed");
    ESP_RETURN_ON_ERROR(write_reg8(panel, 0xE7, 0xC1), TAG, "write E7 failed");
    ESP_RETURN_ON_ERROR(write_reg8(panel, 0x30, 0x08), TAG, "write PLL failed");
    ESP_RETURN_ON_ERROR(write_reg8(panel, 0x50, 0x77), TAG, "write CDI failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, 0x62), TAG, "write timing failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_data(panel, timing, sizeof(timing)), TAG, "write timing data failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, 0x61), TAG, "write resolution failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_data(panel, resolution, sizeof(resolution)), TAG, "write resolution data failed");
    ESP_RETURN_ON_ERROR(write_reg8(panel, 0xE0, 0x10), TAG, "write E0 failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, 0x65), TAG, "write GSST failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_data(panel, gsst, sizeof(gsst)), TAG, "write GSST data failed");
    ESP_RETURN_ON_ERROR(write_reg8(panel, 0xE9, 0x01), TAG, "write E9 failed");
    return seeed_epaper_panel_wait_ready(panel);
}

static esp_err_t ssd2677_power_off(seeed_epaper_panel_t *panel)
{
    if (panel->refresh_powered) {
        ESP_RETURN_ON_ERROR(write_reg8(panel, 0x02, 0x00), TAG, "power off failed");
        ESP_RETURN_ON_ERROR(seeed_epaper_panel_wait_ready(panel), TAG, "wait power off failed");
        panel->refresh_powered = false;
    }
    return ESP_OK;
}

static esp_err_t ssd2677_init_mode(seeed_epaper_panel_t *panel, seeed_epaper_refresh_mode_t mode)
{
    (void)mode;
    ESP_RETURN_ON_ERROR(ssd2677_power_off(panel), TAG, "power off before reset failed");
    return ssd2677_init_otp(panel);
}

static esp_err_t ssd2677_set_window(seeed_epaper_panel_t *panel, const seeed_epaper_area_t *area)
{
    (void)panel;
    (void)area;
    // This controller streams the complete framebuffer during commit.
    // 此控制器在commit阶段传输完整影子帧，局部写入只更新内存。
    return ESP_OK;
}

static esp_err_t ssd2677_write_image(seeed_epaper_panel_t *panel,
                                    const uint8_t *buffer, size_t len, bool previous_plane)
{
    (void)panel;
    (void)buffer;
    (void)len;
    (void)previous_plane;
    return ESP_OK;
}

static esp_err_t ssd2677_latch_waveform(seeed_epaper_panel_t *panel, seeed_epaper_refresh_mode_t mode)
{
    uint8_t temperature = 0;
    ESP_RETURN_ON_ERROR(ssd2677_read_temperature_raw(panel, &temperature), TAG, "read temperature failed");
    uint8_t waveform;
    switch (mode) {
    case SEEED_EPAPER_REFRESH_PARTIAL:
        waveform = ssd2677_partial_waveform_for_temperature(temperature);
        break;
    case SEEED_EPAPER_REFRESH_GRAY4:
        waveform = ssd2677_gray4_waveform_for_temperature(temperature);
        break;
    default:
        waveform = ssd2677_mono_full_waveform_for_temperature(temperature);
        break;
    }
    ESP_RETURN_ON_ERROR(write_reg8(panel, 0xE0, 0x12), TAG, "write E0 failed");
    ESP_RETURN_ON_ERROR(write_reg8(panel, 0xE6, waveform), TAG, "write E6 failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, 0xA5), TAG, "latch waveform failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_wait_ready(panel), TAG, "wait waveform failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}

static esp_err_t ssd2677_write_framebuffer(seeed_epaper_panel_t *panel, seeed_epaper_refresh_mode_t mode)
{
    ESP_RETURN_ON_FALSE(panel->framebuffer != NULL && panel->framebuffer_len >= SSD2677_1BPP_LEN,
                        ESP_ERR_INVALID_STATE, TAG, "missing framebuffer");
    if (mode == SEEED_EPAPER_REFRESH_GRAY4) {
        ESP_RETURN_ON_FALSE(panel->gray4_valid && panel->gray4_framebuffer != NULL &&
                            panel->gray4_framebuffer_len >= SSD2677_2BPP_LEN,
                            ESP_ERR_INVALID_STATE, TAG, "missing gray4 framebuffer");
        ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, 0x10), TAG, "write DTM1 failed");
        return seeed_epaper_panel_write_data(panel, panel->gray4_framebuffer, SSD2677_2BPP_LEN);
    }
    if (mode == SEEED_EPAPER_REFRESH_PARTIAL) {
        ESP_RETURN_ON_FALSE(panel->previous_framebuffer != NULL &&
                            panel->previous_framebuffer_len >= SSD2677_1BPP_LEN,
                            ESP_ERR_INVALID_STATE, TAG, "missing previous framebuffer");
    }
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, 0x10), TAG, "write DTM1 failed");
    uint8_t out[SSD2677_STREAM_IN_BYTES * 2U];
    for (size_t offset = 0; offset < SSD2677_1BPP_LEN;) {
        size_t chunk = SSD2677_1BPP_LEN - offset;
        if (chunk > SSD2677_STREAM_IN_BYTES) {
            chunk = SSD2677_STREAM_IN_BYTES;
        }
        for (size_t i = 0; i < chunk; ++i) {
            if (mode == SEEED_EPAPER_REFRESH_PARTIAL) {
                pack_interleave(panel->previous_framebuffer[offset + i], panel->framebuffer[offset + i], &out[i * 2U]);
            } else {
                pack_mono_byte_to_2bpp(panel->framebuffer[offset + i], &out[i * 2U]);
            }
        }
        ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_data(panel, out, chunk * 2U), TAG, "write framebuffer failed");
        offset += chunk;
    }
    return ESP_OK;
}

static esp_err_t ssd2677_refresh(seeed_epaper_panel_t *panel, seeed_epaper_refresh_mode_t mode)
{
    ESP_RETURN_ON_ERROR(ssd2677_latch_waveform(panel, mode), TAG, "latch waveform failed");
    ESP_RETURN_ON_ERROR(ssd2677_write_framebuffer(panel, mode), TAG, "stream framebuffer failed");
    if (!panel->refresh_powered) {
        // Mark power as possibly on before waiting, so recovery attempts power-off on errors.
        // 上电等待出错时也记录可能已上电，保证下一次恢复会先关闭高压。
        ESP_RETURN_ON_ERROR(seeed_epaper_panel_write_command(panel, 0x04), TAG, "power on failed");
        panel->refresh_powered = true;
        ESP_RETURN_ON_ERROR(seeed_epaper_panel_wait_ready(panel), TAG, "wait power on failed");
    }
    ESP_RETURN_ON_ERROR(write_reg8(panel, 0x12, 0x00), TAG, "refresh failed");
    ESP_RETURN_ON_ERROR(seeed_epaper_panel_wait_ready(panel), TAG, "wait refresh failed");
    // InkSalah updates infrequently, so release high voltage after every refresh.
    // InkSalah不是连续手写应用，每次刷新后关闭高压以保持低功耗。
    return ssd2677_power_off(panel);
}

static esp_err_t ssd2677_sleep(seeed_epaper_panel_t *panel)
{
    ESP_RETURN_ON_ERROR(ssd2677_power_off(panel), TAG, "power off before sleep failed");
    return write_reg8(panel, 0x07, 0xA5);
}

const seeed_epaper_driver_t *seeed_epaper_ssd2677_driver(void)
{
    static const seeed_epaper_driver_t driver = {
        .name = "SSD2677",
        .info = {
            .width = SSD2677_WIDTH,
            .height = SSD2677_HEIGHT,
            .native_bpp = 1,
            .supports_partial = true,
            .supports_gray4 = true,
            .supports_color = false,
        },
        .init_mode = ssd2677_init_mode,
        .set_window = ssd2677_set_window,
        .write_image = ssd2677_write_image,
        .refresh = ssd2677_refresh,
        .sleep = ssd2677_sleep,
    };
    return &driver;
}

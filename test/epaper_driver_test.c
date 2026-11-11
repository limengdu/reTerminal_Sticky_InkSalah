/* Host integration tests compile the real shared layer and controller drivers.
 * SPI/GPIO/RTOS doubles record wire bytes, simulate BUSY, and inject failures. */
#include "epaper_panel_priv.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t id, command, temperature = 25, wire[256][100000];
static size_t sizes[256], commands[256];
static int levels[64], fail_spi, spi_calls, fail_alloc, alloc_calls, allocations;
static unsigned elapsed, last_wait, edge, delays[1000], delay_count;
static bool release_busy;

static void reset_trace(void)
{
    memset(sizes, 0, sizeof sizes);
    memset(commands, 0, sizeof commands);
    spi_calls = 0;
    fail_spi = 0;
    delay_count = 0;
}

void *heap_caps_malloc(size_t n, unsigned caps)
{
    if (++alloc_calls == fail_alloc) {
        return NULL;
    }
    void *p = malloc(n);
    if (p) {
        allocations++;
    }
    return p;
}

void *heap_caps_malloc_prefer(size_t n, size_t count, ...)
{
    return heap_caps_malloc(n, 0);
}

void heap_caps_free(void *p)
{
    if (p) {
        allocations--;
    }
    free(p);
}

int64_t esp_timer_get_time(void)
{
    return (int64_t) elapsed * 1000;
}

const char *esp_err_to_name(int e)
{
    return "host error";
}

int gpio_config(const gpio_config_t *c)
{
    return ESP_OK;
}

int gpio_set_level(int pin, int level)
{
    levels[pin] = level;
    return ESP_OK;
}

int gpio_get_level(int pin)
{
    return levels[pin];
}

int gpio_set_intr_type(int pin, int type)
{
    if (type) {
        edge = type;
    }
    return ESP_OK;
}

int gpio_install_isr_service(int flags)
{
    return ESP_OK;
}

int gpio_isr_handler_add(int pin, void (*fn)(void *), void *arg)
{
    return ESP_OK;
}

int gpio_isr_handler_remove(int pin)
{
    return ESP_OK;
}

int spi_device_transmit(void *h, spi_transaction_t *t)
{
    if (++spi_calls == fail_spi) {
        return ESP_FAIL;
    }
    if (t->flags & SPI_TRANS_USE_RXDATA) {
        assert(levels[1] == 1);
        assert(t->rxlength == 8);
        t->rx_data[0] = command == 0x70 ? id : temperature;
        return ESP_OK;
    }
    size_t n = t->length / 8;
    assert(n <= 1024);
    if (!levels[1]) {
        assert(n == 1);
        command = *(uint8_t *) t->tx_buffer;
        commands[command]++;
    }
    else {
        assert(sizes[command] + n <= sizeof wire[0]);
        memcpy(wire[command] + sizes[command], t->tx_buffer, n);
        sizes[command] += n;
    }
    return ESP_OK;
}

void vTaskDelay(unsigned n)
{
    elapsed += n;
    assert(delay_count < 1000);
    delays[delay_count++] = n;
}

void *xSemaphoreCreateBinary(void)
{
    return malloc(1);
}

int xSemaphoreTake(void *sem, unsigned n)
{
    if (!n) {
        return pdFALSE;
    }
    assert(seeed_epaper_panel_busy_wait_active());
    last_wait = n;
    elapsed += n;
    if (release_busy) {
        levels[3] = !levels[3];
        return pdTRUE;
    }
    return pdFALSE;
}

void xSemaphoreGiveFromISR(void *sem, int *woken)
{
}

void vSemaphoreDelete(void *sem)
{
    free(sem);
}

static seeed_epaper_panel_config_t config(bool mirror)
{
    return (seeed_epaper_panel_config_t){
        .spi_handle = (void *)1,
        .pin_dc = 1,
        .pin_rst = 2,
        .pin_busy = 3,
        .pin_enable = -1,
        .mirror_x = mirror,
    };
}

static seeed_epaper_panel_t *create(uint8_t chip, bool mirror)
{
    reset_trace();
    id = chip;
    levels[3] = chip == 7;
    alloc_calls = 0;
    fail_alloc = 0;
    seeed_epaper_panel_t *p = NULL;
    seeed_epaper_panel_config_t c = config(mirror);
    assert(seeed_epaper_new_panel(SEEED_EPAPER_PANEL_SSD1677, &c, &p) == ESP_OK);
    assert(commands[0x70] == 1);
    assert(delays[0] == 20 && delays[1] == 10);
    return p;
}

static void destroy(seeed_epaper_panel_t *p)
{
    assert(seeed_epaper_panel_del(p) == ESP_OK);
    assert(allocations == 0);
}

static void test_detection(void)
{
    const uint8_t ids[] =
    {
        7, 0, 255
    };
    for (size_t i = 0; i < sizeof ids; i++) {
        seeed_epaper_panel_t *p = create(ids[i], false);
        seeed_epaper_panel_model_t model;
        assert(seeed_epaper_panel_get_model(p, &model) == ESP_OK);
        assert(model == (ids[i] == 7 ? SEEED_EPAPER_PANEL_SSD2677 : SEEED_EPAPER_PANEL_SSD1677));
        assert(p->config.busy_level == (ids[i] == 7 ? 0 : 1));
        assert(p->config.busy_timeout_ms == (ids[i] == 7 ? 30000 : 5000));
        assert(p->framebuffer_len == 48000);
        assert((p->previous_framebuffer != NULL) == (ids[i] == 7));
        destroy(p);
    }
    for (int failure = 1; failure <= 2; failure++) {
        reset_trace();
        fail_spi = failure;
        id = 7;
        seeed_epaper_panel_t *p = (void *) 1;
        seeed_epaper_panel_config_t c = config(false);
        assert(seeed_epaper_new_panel(SEEED_EPAPER_PANEL_SSD1677, &c, &p) == ESP_FAIL);
        assert(!p);
        assert(!allocations);
    }
    for (int failure = 1; failure <= 3; failure++) {
        reset_trace();
        alloc_calls = 0;
        fail_alloc = failure;
        id = 7;
        seeed_epaper_panel_t *p = NULL;
        seeed_epaper_panel_config_t c = config(false);
        assert(seeed_epaper_new_panel(SEEED_EPAPER_PANEL_SSD1677, &c, &p) == ESP_ERR_NO_MEM);
        assert(!p);
        assert(!allocations);
    }
    fail_alloc = 0;
}

static void test_uc8179_and_invalid_model(void)
{
    reset_trace();
    alloc_calls = 0;
    fail_alloc = 0;
    seeed_epaper_panel_config_t c = config(false);
    seeed_epaper_panel_t *p = NULL;
    assert(seeed_epaper_new_panel(SEEED_EPAPER_PANEL_UC8179, &c, &p) == ESP_OK);
    assert(p->model == SEEED_EPAPER_PANEL_UC8179);
    assert(commands[0x70] == 0);
    assert(spi_calls == 0);
    assert(p->previous_framebuffer == NULL);
    destroy(p);

    p = (void *)1;
    assert(seeed_epaper_new_panel((seeed_epaper_panel_model_t)999, &c, &p) == ESP_ERR_NOT_SUPPORTED);
    assert(p == NULL);
    assert(spi_calls == 0);
    assert(allocations == 0);
}

static void test_busy(void)
{
    for (int old = 0; old <= 1; old++) {
        seeed_epaper_panel_t *p = create(old ? 0 : 7, false);
        levels[3] = p->config.busy_level;
        release_busy = false;
        last_wait = 0;
        assert(seeed_epaper_panel_wait_ready(p) == ESP_ERR_TIMEOUT);
        assert(last_wait == (old ? 5000 : 30000));
        assert(edge == (old ? GPIO_INTR_NEGEDGE : GPIO_INTR_POSEDGE));
        assert(!seeed_epaper_panel_busy_wait_active());
        release_busy = true;
        assert(seeed_epaper_panel_wait_ready(p) == ESP_OK);
        assert(!seeed_epaper_panel_busy_wait_active());
        last_wait = 0;
        assert(seeed_epaper_panel_wait_ready(p) == ESP_OK);
        assert(last_wait == 0);
        destroy(p);
    }
    release_busy = false;
}

static void test_gray_and_partial(bool mirror)
{
    seeed_epaper_panel_t *p = create(7, mirror);
    seeed_epaper_area_t area =
    {
        0, 0, 8, 1
    };
    const uint8_t gray[] =
    {
        0x1B, 0x1B
    };
    assert(seeed_epaper_panel_write_bitmap_gray4(p, &area, gray, 0) == ESP_OK);
    size_t mono = mirror ? 99 : 0, two = mirror ? 198 : 0;
    assert(p->gray4_framebuffer[two] == (mirror ? 0xE4 : 0x1B));
    assert(p->gray4_framebuffer[two + 1] == (mirror ? 0xE4 : 0x1B));
    assert(p->framebuffer[mono] == (mirror ? 0xCC : 0x33));
    assert(p->previous_framebuffer[mono] == 255);
    reset_trace();
    assert(seeed_epaper_panel_commit(p, NULL, SEEED_EPAPER_REFRESH_GRAY4) == ESP_OK);
    assert(sizes[0x10] == 96000);
    assert(wire[0x10][two] == (mirror ? 0xE4 : 0x1B));
    assert(p->previous_framebuffer[mono] == p->framebuffer[mono]);
    assert(p->partial_ready);
    assert(!p->refresh_powered);
    assert(wire[0xE6][0] == 0xFA);
    assert(commands[0x04] == 1 && commands[0x12] == 1 && commands[0x02] == 1);
    uint8_t current = 0x55;
    assert(seeed_epaper_panel_write_bitmap(p, &area, &current, 1, SEEED_EPAPER_PIXEL_FORMAT_MONO1_MSB,
        SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
    reset_trace();
    assert(seeed_epaper_panel_commit(p, &area, SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
    assert(sizes[0x10] == 96000);
    // previous 00110011 + current 01010101 -> 00011011 twice (mirrored: 11100100).
    assert(wire[0x10][two] == (mirror ? 0xE4 : 0x1B));
    assert(wire[0x10][two + 1] == (mirror ? 0xE4 : 0x1B));
    assert(wire[0xE6][0] == 0x19);
    current = 0;
    assert(seeed_epaper_panel_write_bitmap(p, &area, &current, 1, SEEED_EPAPER_PIXEL_FORMAT_MONO1_MSB,
        SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
    uint8_t previous = p->previous_framebuffer[mono];
    reset_trace();
    fail_spi = 10;
    assert(seeed_epaper_panel_commit(p, &area, SEEED_EPAPER_REFRESH_PARTIAL) == ESP_FAIL);
    assert(p->previous_framebuffer[mono] == previous);
    assert(!p->prepared && !p->partial_ready);
    reset_trace();
    assert(seeed_epaper_panel_commit(p, &area, SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
    assert(p->active_mode == SEEED_EPAPER_REFRESH_FULL);
    assert(sizes[0x10] == 96000);
    assert(wire[0x10][two] == 0);
    assert(p->previous_framebuffer[mono] == 0);
    reset_trace();
    assert(seeed_epaper_panel_sleep(p) == ESP_OK);
    assert(commands[7] == 1 && wire[7][0] == 0xA5);
    assert(!p->prepared && !p->partial_ready && p->sleeping);
    assert(seeed_epaper_panel_sleep(p) == ESP_OK);
    assert(commands[7] == 1);
    assert(seeed_epaper_panel_prepare(p, SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
    assert(p->active_mode == SEEED_EPAPER_REFRESH_FULL);
    assert(!p->sleeping);
    destroy(p);
}

static void test_legacy(void)
{
    seeed_epaper_panel_t *p = create(0, false);
    seeed_epaper_area_t a =
    {
        0, 0, 8, 1
    };
    uint8_t bits = 0xA5;
    assert(seeed_epaper_panel_write_bitmap(p, &a, &bits, 1, SEEED_EPAPER_PIXEL_FORMAT_MONO1_MSB,
        SEEED_EPAPER_REFRESH_FULL) == ESP_OK);
    assert(commands[0x24] && wire[0x24][sizes[0x24] - 1] == 0xA5);
    assert(commands[0x10] == 0);
    assert(seeed_epaper_panel_commit(p, NULL, SEEED_EPAPER_REFRESH_FULL) == ESP_OK);
    reset_trace();
    bits = 0x5A;
    assert(seeed_epaper_panel_write_bitmap(p, &a, &bits, 1, SEEED_EPAPER_PIXEL_FORMAT_MONO1_MSB,
        SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
    assert(wire[0x26][sizes[0x26] - 1] == 0xA5);
    assert(wire[0x24][sizes[0x24] - 1] == 0x5A);
    assert(seeed_epaper_panel_commit(p, &a, SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
    assert(commands[0x20]);
    assert(!commands[0x10]);
    destroy(p);
}

static void test_waveforms(void)
{
    const uint8_t temps[] =
    {
        0, 10, 11, 15, 16, 20, 21, 30, 31, 127, 128, 255
    };
    const uint8_t full[] =
    {
        0xE8, 0xE8, 0xEB, 0xEB, 0xEB, 0xEB, 0xEE, 0xEE, 0xF1, 0xF1, 0xE8, 0xE8
    };
    const uint8_t partial[] =
    {
        8, 8, 15, 15, 15, 15, 25, 25, 35, 35, 8, 8
    };
    const uint8_t gray[] =
    {
        0xF1, 0xF1, 0xF1, 0xF1, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xF1, 0xF1
    };
    seeed_epaper_panel_t *p = create(7, false);
    seeed_epaper_area_t a =
    {
        0, 0, 8, 1
    };
    uint8_t pixels[] =
    {
        0x1B, 0xE4
    };
    assert(seeed_epaper_panel_write_bitmap_gray4(p, &a, pixels, 2) == ESP_OK);
    for (unsigned i = 0; i < sizeof temps; i++) {
        temperature = temps[i];
        for (int mode = 0; mode < 3; mode++) {
            reset_trace();
            assert(p->driver->refresh(p, mode) == ESP_OK);
            assert(wire[0xE6][0] == (mode == 0 ? full[i] : mode == 1 ? partial[i] : gray[i]));
            assert(sizes[0x10] == 96000);
            assert(delay_count == 1 && delays[0] == 10);
        }
    }
    temperature = 25;
    destroy(p);
}

static void test_commit_failure_sweep(void)
{
    seeed_epaper_panel_t *p = create(7, false);
    seeed_epaper_area_t a =
    {
        0, 0, 8, 1
    };
    uint8_t value = 0;
    assert(seeed_epaper_panel_clear(p, true, SEEED_EPAPER_REFRESH_FULL) == ESP_OK);
    assert(seeed_epaper_panel_write_bitmap(p, &a, &value, 1, SEEED_EPAPER_PIXEL_FORMAT_MONO1_MSB,
        SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
    reset_trace();
    assert(seeed_epaper_panel_commit(p, &a, SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
    int total = spi_calls;
    destroy(p);
    // Fail every transaction, including payload chunks, power-on, refresh and power-off.
    for (int n = 1; n <= total; n++) {
        p = create(7, false);
        assert(seeed_epaper_panel_clear(p, true, SEEED_EPAPER_REFRESH_FULL) == ESP_OK);
        assert(seeed_epaper_panel_write_bitmap(p, &a, &value, 1, SEEED_EPAPER_PIXEL_FORMAT_MONO1_MSB,
            SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
        reset_trace();
        fail_spi = n;
        assert(seeed_epaper_panel_commit(p, &a, SEEED_EPAPER_REFRESH_PARTIAL) == ESP_FAIL);
        assert(p->previous_framebuffer[0] == 255);
        assert(!p->prepared && !p->partial_ready);
        destroy(p);
    }
}

static void test_unaligned_shadow(void)
{
    for (int mirror = 0; mirror < 2; mirror++) {
        seeed_epaper_panel_t *p = create(7, mirror);
        assert(seeed_epaper_panel_clear(p, false, SEEED_EPAPER_REFRESH_FULL) == ESP_OK);
        seeed_epaper_area_t a =
        {
            3, 0, 2, 1
        };
        uint8_t white = 255;
        assert(seeed_epaper_panel_write_bitmap(p, &a, &white, 1, SEEED_EPAPER_PIXEL_FORMAT_MONO1_MSB,
            SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
        assert(p->framebuffer[mirror ? 99 : 0] == 0x18);
        assert(p->previous_framebuffer[mirror ? 99 : 0] == 0);
        reset_trace();
        assert(seeed_epaper_panel_commit(p, &a, SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
        destroy(p);
    }
}

static void test_clipped_default_stride(void)
{
    for (int mirror = 0; mirror < 2; ++mirror) {
        seeed_epaper_panel_t *p = create(7, mirror);
        const seeed_epaper_area_t area = {792, 0, 16, 2};
        const uint8_t mono[] = {0xA5, 0x00, 0x5A, 0xFF};
        const size_t column = mirror ? 0 : 99;
        assert(seeed_epaper_panel_write_bitmap(p, &area, mono, 0,
            SEEED_EPAPER_PIXEL_FORMAT_MONO1_MSB, SEEED_EPAPER_REFRESH_FULL) == ESP_OK);
        assert(p->framebuffer[column] == 0xA5);
        assert(p->framebuffer[100 + column] == 0x5A);

        // Each source row is four bytes, even though only two bytes are visible.
        const uint8_t gray[] = {0x1B, 0x1B, 0x00, 0x00, 0xE4, 0xE4, 0xFF, 0xFF};
        assert(seeed_epaper_panel_write_bitmap_gray4(p, &area, gray, 0) == ESP_OK);
        const size_t gray_column = mirror ? 0 : 198;
        assert(p->gray4_framebuffer[gray_column] == (mirror ? 0xE4 : 0x1B));
        assert(p->gray4_framebuffer[200 + gray_column] == (mirror ? 0x1B : 0xE4));
        destroy(p);
    }
}

static void test_gray_shadow_reconstruction(void)
{
    for (int mirror = 0; mirror < 2; ++mirror) {
        seeed_epaper_panel_t *p = create(7, mirror);
        const seeed_epaper_area_t area = {0, 0, 4, 1};
        const uint8_t gray = 0x55;
        assert(seeed_epaper_panel_write_bitmap_gray4(p, &area, &gray, 0) == ESP_OK);
        reset_trace();
        assert(seeed_epaper_panel_commit(p, NULL, SEEED_EPAPER_REFRESH_GRAY4) == ESP_OK);
        reset_trace();
        assert(seeed_epaper_panel_clear(p, false, SEEED_EPAPER_REFRESH_FULL) == ESP_OK);
        assert(!p->gray4_valid);

        const seeed_epaper_area_t small = {4, 1, 4, 1};
        const uint8_t new_gray = 0x1B;
        assert(seeed_epaper_panel_write_bitmap_gray4(p, &small, &new_gray, 0) == ESP_OK);
        const size_t changed = 200 + (mirror ? 198 : 1);
        for (size_t i = 0; i < p->gray4_framebuffer_len; ++i) {
            assert(p->gray4_framebuffer[i] == (i == changed ? (mirror ? 0xE4 : 0x1B) : 0));
        }
        reset_trace();
        assert(seeed_epaper_panel_commit(p, NULL, SEEED_EPAPER_REFRESH_GRAY4) == ESP_OK);
        assert(sizes[0x10] == 96000);
        assert(wire[0x10][changed] == (mirror ? 0xE4 : 0x1B));
        destroy(p);
    }
}

static void test_explicit_diff_regions(void)
{
    for (int mirror = 0; mirror < 2; ++mirror) {
        seeed_epaper_panel_t *p = create(7, mirror);
        assert(seeed_epaper_panel_clear(p, false, SEEED_EPAPER_REFRESH_FULL) == ESP_OK);
        const seeed_epaper_area_t area = {3, 1, 2, 2};
        const uint8_t previous[] = {0x80, 0x40};
        const uint8_t current[] = {0x40, 0x80};
        assert(seeed_epaper_panel_write_bitmap_diff(p, &area, previous, current, 0,
            SEEED_EPAPER_PIXEL_FORMAT_MONO1_MSB, SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
        const size_t column = mirror ? 99 : 0;
        assert(p->previous_framebuffer[100 + column] == (mirror ? 0x08 : 0x10));
        assert(p->framebuffer[100 + column] == (mirror ? 0x10 : 0x08));
        assert(p->previous_framebuffer[200 + column] == (mirror ? 0x10 : 0x08));
        assert(p->framebuffer[200 + column] == (mirror ? 0x08 : 0x10));
        reset_trace();
        assert(seeed_epaper_panel_commit(p, &area, SEEED_EPAPER_REFRESH_PARTIAL) == ESP_OK);
        const size_t offset = (100 + column) * 2;
        assert(wire[0x10][offset] == (mirror ? 0x01 : 0x02));
        assert(wire[0x10][offset + 1] == (mirror ? 0x80 : 0x40));
        assert(p->previous_framebuffer[100 + column] == p->framebuffer[100 + column]);
        assert(p->previous_framebuffer[200 + column] == p->framebuffer[200 + column]);
        destroy(p);
    }
}

int main(void)
{
    test_detection();
    test_uc8179_and_invalid_model();
    test_busy();
    test_gray_and_partial(false);
    test_gray_and_partial(true);
    test_legacy();
    test_waveforms();
    test_commit_failure_sweep();
    test_unaligned_shadow();
    test_clipped_default_stride();
    test_gray_shadow_reconstruction();
    test_explicit_diff_regions();
    puts("e-paper host integration tests passed");
}

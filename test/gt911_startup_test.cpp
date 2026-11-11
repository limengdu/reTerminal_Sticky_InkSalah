#include "gt911.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{
struct Event
{
    char kind;
    int pin;
    int value;
    uint64_t ms;
};
std::vector<Event> events;
std::map<uint16_t, std::vector<uint8_t>> registers;
std::map<int, int> modes, levels;
uint64_t now_ms = 0;
uint16_t accepted_address = 0x14;
bool fail_id = false, fail_resolution = false;
unsigned resolution_reads = 0, id_reads = 0, status_clears = 0;
constexpr int kInt = 21, kReset = 41;
void check(bool ok, const char *message)
{
    if (!ok)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
void setup(uint16_t address = 0x14)
{
    events.clear();
    registers.clear();
    modes.clear();
    levels.clear();
    now_ms = 0;
    accepted_address = address;
    fail_id = false;
    fail_resolution = false;
    resolution_reads = 0;
    id_reads = 0;
    status_clears = 0;
    registers[GT911_REG_ID] = {'9', '1', '1', 0};
    registers[GT911_REG_COORD_RES] = {0xe0, 1, 0x20, 3};
    registers[GT911_REG_STATUS] = {0};
}
i2c_master_bus_handle_t bus()
{
    return reinterpret_cast<void *>(1);
}
void check_address_selection(uint16_t address)
{
    unsigned releases = 0;
    std::map<int, int> seen_levels, seen_modes;
    for (const auto &e : events)
    {
        if (e.kind == 'm')
            seen_modes[e.pin] = e.value;
        if (e.kind == 'l')
        {
            seen_levels[e.pin] = e.value;
            if (e.pin == kReset && e.value == 1)
            {
                check(seen_modes[kInt] == GPIO_MODE_OUTPUT,
                      "INT must be output at reset release");
                ++releases;
                if (releases == 1)
                    check(seen_levels[kInt] == 1, "first candidate selects 0x14");
                if (releases == 2)
                    check(seen_levels[kInt] == 0, "second candidate selects 0x5D");
            }
        }
    }
    check(releases == (address == 0x14 ? 1U : 2U), "reset candidate count");
}
// Verify each candidate's complete interrupt synchronization interval.
// 检查每个候选地址的完整中断同步区间。
void check_int_sync()
{
    bool released = false;
    bool low = false;
    uint64_t low_since = 0;
    unsigned completed = 0;
    int int_mode = GPIO_MODE_INPUT;
    for (const auto &event : events)
    {
        if (event.kind == 'l' && event.pin == kReset && event.value == 1)
        {
            check(!released, "previous reset must complete INT synchronization");
            released = true;
            low = false;
        }
        if (event.pin != kInt)
        {
            continue;
        }
        if (event.kind == 'm')
        {
            if (released && event.value == GPIO_MODE_INPUT)
            {
                check(int_mode == GPIO_MODE_OUTPUT,
                      "INT must remain output throughout synchronization");
                check(low && event.ms - low_since >= 50,
                      "INT must remain actively low for >=50ms after RST release");
                released = false;
                ++completed;
            }
            int_mode = event.value;
        }
        if (released && event.kind == 'l')
        {
            check(int_mode == GPIO_MODE_OUTPUT,
                  "INT synchronization level must be driven as output");
            if (event.value == 0)
            {
                if (!low)
                {
                    low_since = event.ms;
                }
                low = true;
            }
            else
            {
                low = false;
            }
        }
    }
    check(!released, "last reset must return INT to input");
    check(completed == (accepted_address == 0x14 ? 1U : 2U),
          "all reset candidates must complete INT synchronization");
}
} // namespace
int gpio_set_direction(gpio_num_t pin, int mode)
{
    modes[pin] = mode;
    events.push_back({'m', pin, mode, now_ms});
    return ESP_OK;
}
int gpio_set_level(gpio_num_t pin, int level)
{
    levels[pin] = level;
    events.push_back({'l', pin, level, now_ms});
    return ESP_OK;
}
int gpio_hold_dis(gpio_num_t pin)
{
    events.push_back({'h', pin, 0, now_ms});
    return ESP_OK;
}
void vTaskDelay(TickType_t ticks)
{
    now_ms += ticks;
}
int64_t esp_timer_get_time()
{
    return static_cast<int64_t>(now_ms * 1000);
}
esp_err_t i2c_master_probe(i2c_master_bus_handle_t, uint16_t address, int)
{
    return address == accepted_address ? ESP_OK : ESP_FAIL;
}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t,
                                    const i2c_device_config_t *c,
                                    i2c_master_dev_handle_t *out)
{
    check(c->scl_speed_hz == 100000, "I2C frequency");
    check(!c->flags.disable_ack_check, "ACK checking enabled");
    *out = new FakeDevice{c->device_address};
    return ESP_OK;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t d)
{
    delete d;
    return ESP_OK;
}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t, const uint8_t *data, size_t n,
                              int)
{
    check(n == 3, "one-byte register write framing");
    const uint16_t reg = static_cast<uint16_t>((data[0] << 8) | data[1]);
    registers[reg] = {data[2]};
    if (reg == GT911_REG_STATUS && data[2] == 0)
        ++status_clears;
    return ESP_OK;
}
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t d, const uint8_t *request,
                                      size_t n, uint8_t *out, size_t len, int)
{
    check(n == 2, "16-bit big-endian register addressing");
    check(d->address == accepted_address, "selected I2C address");
    const uint16_t reg = static_cast<uint16_t>((request[0] << 8) | request[1]);
    if (reg == GT911_REG_ID)
    {
        ++id_reads;
        if (fail_id)
            return ESP_FAIL;
    }
    if (reg == GT911_REG_COORD_RES)
    {
        ++resolution_reads;
        if (fail_resolution)
            return ESP_FAIL;
    }
    const auto it = registers.find(reg);
    if (it == registers.end() || it->second.size() < len)
        return ESP_FAIL;
    std::memcpy(out, it->second.data(), len);
    return ESP_OK;
}
int main(int argc, char **argv)
{
    const bool require_sync = argc == 2 && std::string(argv[1]) == "--require-int-sync";
    check(argc == 1 || require_sync, "usage: test [--require-int-sync]");
    for (const uint16_t address : {uint16_t(0x14), uint16_t(0x5d)})
    {
        setup(address);
        GT911 driver;
        check(driver.begin(kInt, kReset, 800, 480, bus()), "normal startup");
        check(driver.address() == address && driver.isReady(), "ready and address");
        check_address_selection(address);
        if (require_sync)
            check_int_sync();
        uint16_t x = 0, y = 0;
        check(driver.readResolution(x, y) && x == 480 && y == 800,
              "resolution success");
        registers[GT911_REG_STATUS] = {0x81};
        registers[GT911_REG_POINTS] = {0xf0, 0, 0x90, 1, 5, 0, 0, 2};
        GTPoint point{};
        check(driver.read_points(&point, 1) == 1, "point count");
        check(point.x == 400 && point.y == 240 && point.size == 5 && point.id == 2,
              "point decoding and mapping");
        check(registers[GT911_REG_STATUS][0] == 0 && status_clears >= 2,
              "status cleared after read");
        check(driver.read_points(&point, 1) == 0, "no fresh data");
    }
    setup();
    {
        GT911 driver;
        fail_id = true;
        check(!driver.begin(kInt, kReset, 800, 480, bus()),
              "ID transport failure rejects startup");
        check(id_reads == 9, "three reset rounds times three ID transport retries");
        check(!driver.isReady(), "failure not ready");
    }
    setup();
    {
        GT911 driver;
        fail_resolution = true;
        check(!driver.begin(kInt, kReset, 800, 480, bus()),
              "resolution transport failure rejects startup");
        check(!driver.isReady(), "resolution failure leaves controller unready");
        uint16_t x = 0, y = 0;
        check(!driver.readResolution(x, y) && x == 2048 && y == 2048,
              "2048 defaults are not raw register evidence");
        check(resolution_reads == 6,
              "two resolution calls each exhaust three transport attempts");
    }
    setup();
    {
        GT911 driver;
        registers[GT911_REG_COORD_RES] = {0, 0, 0, 0};
        check(!driver.begin(kInt, kReset, 800, 480, bus()),
              "zero geometry rejects startup");
        check(!driver.isReady(), "zero geometry leaves controller unready");
        uint16_t x = 0, y = 0;
        check(!driver.readResolution(x, y) && x == 2048 && y == 2048,
              "invalid geometry returns defaults");
    }
    setup();
    {
        GT911 driver;
        check(driver.begin(kInt, kReset, 800, 480, bus()),
              "baseline before read failure");
        fail_resolution = true;
        uint16_t x = 0, y = 0;
        check(!driver.readResolution(x, y) && x == 480 && y == 800,
              "failed later read retains last geometry");
    }
    std::cout << "PASS: real GT911 startup, address selection, ID/resolution failures, "
                 "point decoding"
              << (require_sync ? ", INT sync" : " (INT sync assertion opt-in)") << '\n';
}

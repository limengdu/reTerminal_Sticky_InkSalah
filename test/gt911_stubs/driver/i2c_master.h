#pragma once
#include <cstddef>
#include <cstdint>
using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0, ESP_FAIL = -1;
inline const char *esp_err_to_name(esp_err_t e)
{
    return e == 0 ? "ESP_OK" : "ESP_FAIL";
}
using i2c_master_bus_handle_t = void *;
struct FakeDevice
{
    uint16_t address;
};
using i2c_master_dev_handle_t = FakeDevice *;
constexpr int I2C_ADDR_BIT_LEN_7 = 0;
struct i2c_device_config_t
{
    int dev_addr_length;
    uint16_t device_address;
    uint32_t scl_speed_hz;
    uint32_t scl_wait_us;
    struct
    {
        unsigned disable_ack_check;
    } flags;
};
esp_err_t i2c_master_probe(i2c_master_bus_handle_t, uint16_t, int);
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t,
                                    const i2c_device_config_t *,
                                    i2c_master_dev_handle_t *);
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t);
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t, const uint8_t *, size_t, int);
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t, const uint8_t *, size_t,
                                      uint8_t *, size_t, int);

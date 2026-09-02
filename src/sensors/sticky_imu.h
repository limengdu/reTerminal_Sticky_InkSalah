#pragma once

#include <cstdint>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "sticky_orientation.h"

// Latest accelerometer sample plus both observed and settled orientations.
// 最近一次加速度采样，以及快速观察姿态和最终稳定姿态。
struct StickyImuState {
    float acceleration_x_g = 0.0F;
    float acceleration_y_g = 0.0F;
    float acceleration_z_g = 0.0F;
    StickyImuOrientation observed_orientation =
        StickyImuOrientation::Unknown;
    StickyImuOrientation orientation = StickyImuOrientation::Unknown;
    uint8_t orientation_stable_samples = 0U;
    bool moving = false;
    bool valid = false;
};

// Initializes the LSM6DS3TR-C accelerometer on the shared sensor I2C bus.
// 在共享传感器I2C总线上初始化LSM6DS3TR-C加速度计。
esp_err_t sticky_imu_init(i2c_master_bus_handle_t bus);

// Starts the stable-orientation monitoring task.
// 启动稳定姿态监测任务。
esp_err_t sticky_imu_start_monitoring();

// Stops the monitoring task and clears measurements from the ended session.
// 停止监测任务，并清除本次会话留下的测量状态。
esp_err_t sticky_imu_stop_monitoring();

// Copies acceleration, fast stability count, motion, and settled orientation.
// 复制加速度、快速稳定计数、移动标记和最终放稳姿态。
esp_err_t sticky_imu_get_state(StickyImuState &state);

// Arms the low-power 4D orientation interrupt before ESP32 deep sleep.
// 在ESP32进入深睡前，启用低功耗4D姿态中断。
esp_err_t sticky_imu_prepare_orientation_wakeup();

// Returns the stable log and UI name for one orientation value.
// 返回姿态对应的稳定日志与界面名称。
const char *sticky_imu_orientation_name(StickyImuOrientation orientation);

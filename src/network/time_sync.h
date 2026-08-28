#pragma once

#include <cstdint>

#include "esp_err.h"
#include "sticky_rtc.h"

esp_err_t time_sync_from_network(int16_t utc_offset_minutes,
                                 StickyRtcDateTime &local_time,
                                 uint32_t timeout_ms);

// Converts one synchronized UTC timestamp into the configured local wall time.
// 将网络同步后的UTC时间转换成用户配置的本地时间。
bool time_sync_local_now(int16_t utc_offset_minutes,
                         StickyRtcDateTime &local_time);

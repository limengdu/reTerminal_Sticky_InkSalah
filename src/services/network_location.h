#pragma once

#include "esp_err.h"

struct NetworkLocationEstimate {
    bool valid = false;
    char city[48] = {};
    char country[48] = {};
    double latitude = 0.0;
    double longitude = 0.0;
    int16_t utc_offset_minutes = 0;
};

// Requests an approximate public-IP location for first-time setup.
// 在首次设置时请求一个基于公网IP的近似位置。
esp_err_t network_location_estimate(NetworkLocationEstimate &estimate);

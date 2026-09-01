#pragma once

#include <cstddef>

#include "settings_model.h"

// Decodes the Wi-Fi-only first step of the captive portal.
// 解析配网页面中只包含Wi-Fi的第一步表单。
bool provisioning_wifi_form_parse(const char *body,
                                  SalahSettings &settings,
                                  char *error,
                                  size_t error_capacity);

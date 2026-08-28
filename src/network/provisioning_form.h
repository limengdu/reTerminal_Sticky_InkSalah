#pragma once

#include <cstddef>

#include "settings_model.h"

// Decodes one application/x-www-form-urlencoded setup submission.
// 解析一次application/x-www-form-urlencoded格式的配网表单。
bool provisioning_form_parse(const char *body,
                             SalahSettings &settings,
                             char *error,
                             size_t error_capacity);

bool provisioning_parse_utc_offset(const char *text,
                                   int16_t &offset_minutes);

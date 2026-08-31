#pragma once

#include <cstdint>

struct IslamicBackgroundAsset {
    uint16_t width = 0U;
    uint16_t height = 0U;
    uint16_t stride = 0U;
    const uint8_t *data = nullptr;
};

// Returns the generated 1-bit portrait prayer-page ornament.
// 返回由设计源图生成的竖屏礼拜页面 1 位装饰背景。
IslamicBackgroundAsset islamic_prayer_background_asset();

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

// Returns the generated 1-bit portrait first-time setup ornament.
// 返回由第一版设计稿生成的竖屏首次配网装饰背景。
IslamicBackgroundAsset islamic_setup_background_asset();

// Returns the generated 1-bit landscape prayer-page ornament.
// 返回由横屏设计稿生成的横屏礼拜页面装饰背景。
IslamicBackgroundAsset islamic_landscape_background_asset();

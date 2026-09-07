#pragma once

#include <cstdint>

#include "prayer_time.h"

struct ArabicPrayerAsset {
    uint16_t width = 0U;
    uint16_t height = 0U;
    uint16_t stride = 0U;
    const uint8_t *data = nullptr;
};

// Returns a pre-shaped monochrome Arabic label for one prayer.
// 返回一次礼拜对应的预排版黑白阿拉伯语标签。
ArabicPrayerAsset arabic_prayer_asset(PrayerId prayer);
ArabicPrayerAsset arabic_time_asset(PrayerTimeSlot slot);

// Returns the native-size connected Arabic label for the current-prayer title.
// 返回当前礼拜标题对应的原生字号连写阿拉伯语标签。
ArabicPrayerAsset arabic_prayer_hero_asset(PrayerId prayer, bool portrait);

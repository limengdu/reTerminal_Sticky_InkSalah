#pragma once

#include "canvas.h"
#include "islamic_background_asset.h"

constexpr int kPrayerStatusCenterY = 38;

struct StatusBarArea {
    int x;
    int y;
    int width;
    int height;

    bool contains(int px, int py) const
    {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

inline StatusBarArea prayer_location_area(int width, int height)
{
    const int group_width = width < height ? 132 : 300;
    return {width / 2 - group_width / 2, 24, group_width, 28};
}

inline StatusBarArea prayer_method_area(int width, int height)
{
    const int group_width = width < height ? 146 : 210;
    return {width - 24 - group_width, 24, group_width, 28};
}

// Splits the space between header groups into adjacent, distinct touch targets.
// 将顶部两组内容之间的空隙分配给相邻且独立的触摸区域。
inline StatusBarArea prayer_location_touch_area(int width, int height)
{
    const auto location = prayer_location_area(width, height);
    const auto method = prayer_method_area(width, height);
    const int boundary = (location.x + location.width + method.x) / 2;
    return {location.x - 16, 8, boundary - location.x + 16, 60};
}

inline StatusBarArea prayer_settings_touch_area(int width, int height)
{
    const auto location = prayer_location_touch_area(width, height);
    const int left = location.x + location.width;
    return {left, 8, width - 4 - left, 60};
}

// Centers the occupied rows of a status bitmap on the shared horizontal axis.
// 将状态位图实际占用行的中心对齐到统一水平轴线。
inline int prayer_status_bitmap_y(const IslamicBackgroundAsset &asset)
{
    int top = asset.height;
    int bottom = -1;
    for (int row = 0; row < asset.height; ++row) {
        for (int column = 0; column < asset.width; ++column) {
            if ((asset.data[row * asset.stride + column / 8] & (1U << (7 - column % 8))) == 0) continue;
            if (top == asset.height) top = row;
            bottom = row;
            break;
        }
    }
    return bottom < top ? kPrayerStatusCenterY
                         : kPrayerStatusCenterY - (bottom - top + 1) / 2 - top;
}

inline StatusBarArea battery_status_area(const Canvas &canvas, bool sleep_layout)
{
    if (canvas.prayer_header()) return {24, 24, 72, 28};
    return {canvas.width() - 98 - (sleep_layout ? 55 : 6), 4, 98, 28};
}

inline StatusBarArea sleep_status_area(const Canvas &canvas)
{
    return canvas.prayer_header() ? StatusBarArea{102, 24, 28, 28}
                                  : StatusBarArea{8, 8, 28, 28};
}

#pragma once

#include "canvas.h"

struct StatusBarArea {
    int x;
    int y;
    int width;
    int height;
};

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

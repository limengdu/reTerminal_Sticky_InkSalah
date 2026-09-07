#include "battery_status_render.h"

#include <algorithm>
#include <cstdio>

#include "battery_status_overlay_theme.h"
#include "status_bar_layout.h"

namespace {

void charging_bolt(Canvas &canvas, int x, int y, GrayLevel color)
{
    canvas.draw_line(x + 5, y, x, y + 7, color);
    canvas.draw_line(x + 6, y, x + 1, y + 7, color);
    canvas.draw_line(x, y + 7, x + 6, y + 7, color);
    canvas.draw_line(x + 1, y + 8, x + 7, y + 8, color);
    canvas.draw_line(x + 6, y + 7, x + 1, y + 15, color);
    canvas.draw_line(x + 7, y + 8, x + 2, y + 15, color);
}

}  // namespace

void battery_status_render_clear(Canvas &canvas, bool sleep_layout)
{
    const auto area = battery_status_area(canvas, sleep_layout);
    const auto theme = battery_status_overlay_theme(canvas, area.x, area.y, area.width, area.height);
    canvas.fill_rect(area.x, area.y, area.width, area.height, theme.background);
}

void battery_status_render(Canvas &canvas, bool valid, int percent,
                           bool external_power, bool sleep_layout)
{
    const auto area = battery_status_area(canvas, sleep_layout);
    const auto theme = battery_status_overlay_theme(canvas, area.x, area.y, area.width, area.height);
    canvas.fill_rect(area.x, area.y, area.width, area.height, theme.background);
    const bool prayer = canvas.prayer_header();
    const int width = prayer ? 24 : 34;
    const int x = area.x + 1;
    const int y = area.y + 6;
    percent = std::clamp(percent, 0, 100);
    canvas.draw_rect(x, y, width, 16, theme.foreground);
    canvas.fill_rect(x + width, y + 5, 4, 6, theme.foreground);
    if (valid && percent > 0)
        canvas.fill_rect(x + 2, y + 2, ((width - 4) * percent + 99) / 100, 12, theme.foreground);
    if (external_power)
        charging_bolt(canvas, x + width / 2 - 4, y,
                       valid && percent >= 50 ? theme.background : theme.foreground);

    char label[6] = {};
    if (valid) std::snprintf(label, sizeof(label), prayer ? "%d" : "%d%%", percent);
    else std::snprintf(label, sizeof(label), "%s", prayer ? "--" : "--%");
    const int label_x = prayer ? x + width + 10 : area.x + 43;
    const auto weight = prayer ? TextWeight::Bold : TextWeight::Regular;
    const int label_width = Canvas::text_width(label, 20, weight);
    const int offset = prayer ? 0 : (area.width - 43 - label_width) / 2;
    canvas.draw_text_sized(label_x + offset, prayer ? area.y + 2 : area.y + 7,
                           label, 20, theme.foreground, weight);
}

#pragma once

class Canvas;

// Paints battery status from a supplied reading in the page's status-bar area.
// 根据传入读数在页面指定的状态栏区域绘制电量。
void battery_status_render(Canvas &canvas, bool valid, int percent,
                           bool external_power, bool sleep_layout);
void battery_status_render_clear(Canvas &canvas, bool sleep_layout);

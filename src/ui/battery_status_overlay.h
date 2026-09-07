#pragma once

class Canvas;

// Clears the overlay footprint using its detected page background color.
// 使用检测到的页面底色清除普通布局或睡眠布局的电量显示区域。
void battery_status_overlay_clear(Canvas &canvas, bool sleep_layout);

// Draws the cached battery reading in the current page's status-bar area.
// 在当前页面的状态栏区域绘制缓存的电量读数。
void battery_status_overlay_draw(Canvas &canvas, bool sleep_layout);

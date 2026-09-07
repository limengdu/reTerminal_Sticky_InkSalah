#include "sleep_indicator.h"

#include "canvas.h"
#include "status_bar_layout.h"

namespace {

constexpr int kMoonRadius = 9;
constexpr int kCutoutRadius = 8;

}  // namespace

void sleep_indicator_render(Canvas &canvas)
{
    const auto area = sleep_status_area(canvas);
    const int center_x = area.x + 13;
    const int center_y = area.y + 13;
    const GrayLevel background = canvas.pixel_at(area.x, area.y);
    const GrayLevel foreground = background <= GrayLevel::DarkGray
                                     ? GrayLevel::White : GrayLevel::Black;
    // Clears a compact badge area, then carves one circle from another to
    // produce a crisp crescent without scaling a bitmap.
    // 先清理小型标记区域，再用两个圆形相减得到清晰月牙，无需缩放位图。
    canvas.fill_rect(area.x,
                     area.y,
                     area.width,
                     area.height,
                     background);
    canvas.fill_circle(center_x,
                       center_y,
                       kMoonRadius,
                       foreground);
    canvas.fill_circle(center_x + 4,
                       center_y - 4,
                       kCutoutRadius,
                       background);
}

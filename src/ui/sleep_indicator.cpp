#include "sleep_indicator.h"

#include "canvas.h"

namespace {

constexpr int kIndicatorLeft = 8;
constexpr int kIndicatorTop = 8;
constexpr int kIndicatorSize = 28;
constexpr int kMoonCenterX = kIndicatorLeft + 13;
constexpr int kMoonCenterY = kIndicatorTop + 13;
constexpr int kMoonRadius = 9;
constexpr int kCutoutCenterX = kMoonCenterX + 4;
constexpr int kCutoutCenterY = kMoonCenterY - 4;
constexpr int kCutoutRadius = 8;

}  // namespace

void sleep_indicator_render(Canvas &canvas)
{
    // Clears a compact badge area, then carves one circle from another to
    // produce a crisp crescent without scaling a bitmap.
    // 先清理小型标记区域，再用两个圆形相减得到清晰月牙，无需缩放位图。
    canvas.fill_rect(kIndicatorLeft,
                     kIndicatorTop,
                     kIndicatorSize,
                     kIndicatorSize,
                     GrayLevel::White);
    canvas.fill_circle(kMoonCenterX,
                       kMoonCenterY,
                       kMoonRadius,
                       GrayLevel::Black);
    canvas.fill_circle(kCutoutCenterX,
                       kCutoutCenterY,
                       kCutoutRadius,
                       GrayLevel::White);
}

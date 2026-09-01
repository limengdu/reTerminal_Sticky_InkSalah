#pragma once

#include <cstdint>

enum class StickyVerticalSwipe : uint8_t {
    None,
    UpFromBottom,
    DownFromTop,
};

enum class StickyHorizontalSwipe : uint8_t {
    None,
    RightFromLeft,
};

struct StickyLogicalTouchPath {
    int start_x;
    int start_y;
    int end_x;
    int end_y;
    uint32_t duration_ms;
};

// Recognizes a deliberate vertical edge swipe in the current page axes.
// 在当前页面坐标系中识别明确的纵向边缘滑动。
StickyVerticalSwipe sticky_touch_classify_vertical_swipe(
    const StickyLogicalTouchPath &path,
    int page_width,
    int page_height);

// Recognizes a deliberate swipe from the left edge toward the page center.
// 识别从页面左侧边缘明确滑向页面中部的横向手势。
StickyHorizontalSwipe sticky_touch_classify_horizontal_swipe(
    const StickyLogicalTouchPath &path,
    int page_width,
    int page_height);

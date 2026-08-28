#pragma once

#include "canvas.h"
#include "sticky_orientation.h"

CanvasRotation salah_default_rotation();

// Maps the calibrated sensor axes to the enclosure's physical orientation.
// 将传感器标定坐标映射为外壳的实际摆放方向。
CanvasRotation salah_rotation_from_orientation(
    StickyImuOrientation orientation,
    CanvasRotation fallback);

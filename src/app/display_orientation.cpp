#include "display_orientation.h"

CanvasRotation salah_default_rotation()
{
    return CanvasRotation::Deg90CounterClockwise;
}

CanvasRotation salah_rotation_from_orientation(
    StickyImuOrientation orientation,
    CanvasRotation fallback)
{
    switch (orientation) {
    case StickyImuOrientation::Landscape0:
        return CanvasRotation::Deg90CounterClockwise;
    case StickyImuOrientation::Landscape180:
        return CanvasRotation::Deg90Clockwise;
    case StickyImuOrientation::Portrait0:
        return CanvasRotation::Deg180;
    case StickyImuOrientation::Portrait180:
        return CanvasRotation::Deg0;
    case StickyImuOrientation::Unknown:
    case StickyImuOrientation::FaceUp:
    case StickyImuOrientation::FaceDown:
    default:
        return fallback;
    }
}

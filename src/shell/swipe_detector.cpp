#include "shell/swipe_detector.h"

namespace {
constexpr int32_t SWIPE_THRESHOLD_PX = 60;
}

SwipeDirection SwipeDetector::feed(bool touched, uint16_t x, uint16_t y) {
    if (touched) {
        if (!active_) {
            active_ = true;
            start_x_ = x;
            start_y_ = y;
        }
        last_x_ = x;
        last_y_ = y;
        return SwipeDirection::NONE;
    }

    if (!active_) {
        return SwipeDirection::NONE;
    }
    active_ = false;

    int32_t dx = static_cast<int32_t>(last_x_) - static_cast<int32_t>(start_x_);
    int32_t dy = static_cast<int32_t>(start_y_) - static_cast<int32_t>(last_y_);  // positive = moved up
    int32_t adx = dx < 0 ? -dx : dx;
    int32_t ady = dy < 0 ? -dy : dy;

    if (adx < SWIPE_THRESHOLD_PX && ady < SWIPE_THRESHOLD_PX) {
        return SwipeDirection::NONE;
    }

    if (ady >= adx) {
        return dy > 0 ? SwipeDirection::UP : SwipeDirection::DOWN;
    }
    return dx > 0 ? SwipeDirection::RIGHT : SwipeDirection::LEFT;
}

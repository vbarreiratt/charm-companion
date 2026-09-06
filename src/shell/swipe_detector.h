#ifndef SHELL_SWIPE_DETECTOR_H
#define SHELL_SWIPE_DETECTOR_H

#include <cstdint>

enum class SwipeDirection { NONE, UP, DOWN, LEFT, RIGHT };

// Pure gesture-detection logic, no hardware dependency. Fed the current
// tick's touch state (as TouchHAL reports it); reports a swipe direction
// the instant a touch-release completes a motion past threshold.
//
// Kept separate from TouchHAL deliberately: TouchHAL's native (non-ARDUINO)
// fallback always reports "not touched", so gesture logic living inside
// Shell::poll_sensors() directly would be untestable without real hardware.
class SwipeDetector {
public:
    SwipeDirection feed(bool touched, uint16_t x, uint16_t y);

private:
    bool active_ = false;
    uint16_t start_x_ = 0;
    uint16_t start_y_ = 0;
    uint16_t last_x_ = 0;
    uint16_t last_y_ = 0;
};

#endif  // SHELL_SWIPE_DETECTOR_H

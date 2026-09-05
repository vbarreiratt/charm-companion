#ifndef SHELL_EVENT_TYPES_H
#define SHELL_EVENT_TYPES_H

#include <cstdint>

enum class EventType : uint8_t {
    MOOD_CHANGED,
    TOUCH_EVENT,
    MOTION_EVENT,
    APP_TRANSITION,
};

// Event payloads (union or tagged struct)
struct TouchEventData {
    uint16_t x;
    uint16_t y;
    uint16_t duration_ms;
    uint8_t intensity;  // 0-255
};

struct MotionEventData {
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    float intensity;    // magnitude
    uint8_t direction;  // encoded direction
};

struct MoodChangedData {
    // Points to the global Spicy instance's internal context.
    // Safe for Phase 1: dispatch is synchronous and g_spicy is a global singleton.
    // REVISIT if event dispatch becomes async.
    void* personality_context;
};

struct AppTransitionData {
    const char* from_app;
    const char* to_app;
};

// Ruling 4: Add TouchEvent / MotionEvent type aliases
using TouchEvent  = TouchEventData;
using MotionEvent = MotionEventData;

// Generic event struct
struct Event {
    EventType type;
    union {
        TouchEventData touch;
        MotionEventData motion;
        MoodChangedData mood;
        AppTransitionData app_transition;
    } data;
};

#endif // SHELL_EVENT_TYPES_H

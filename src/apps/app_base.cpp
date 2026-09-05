// src/apps/app_base.cpp
#include "apps/app_base.h"

void App::on_event(const Event& e) {
    switch (e.type) {
        case EventType::TOUCH_EVENT:
            on_touch(e.data.touch);
            break;
        case EventType::MOTION_EVENT:
            on_motion(e.data.motion);
            break;
        case EventType::MOOD_CHANGED:
            // Apps can override to react to mood changes
            break;
        default:
            break;
    }
}

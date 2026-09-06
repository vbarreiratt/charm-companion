#include "shell/shell.h"
#include "config/pin_config.h"
#include "shell/hal/display_hal.h"
#include "shell/hal/touch_hal.h"
#include "shell/hal/imu_hal.h"
#include "shell/hal/power_hal.h"
#include "shell/event_types.h"
#include "spicy/spicy.h"
#include "spicy/personality_nvs.h"
#include "apps/app_base.h"
#include "apps/home/home_app.h"
#include "apps/scenes/scenes_app.h"

#include <cmath>
#include <cstring>

#if defined(ARDUINO)
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#include <cstdio>
#endif

Shell& Shell::instance() {
    static Shell sh;
    return sh;
}

Shell::Shell() : screen_canvas(DISPLAY_WIDTH, DISPLAY_HEIGHT) {}

Shell::~Shell() {
    if (current_app) {
        current_app->on_exit();
        current_app = nullptr;
    }
    next_app = nullptr;
}

bool Shell::init() {
#if defined(ARDUINO)
    Serial.println("Shell::init()");
#else
    printf("Shell::init()\n");
#endif

    if (!DisplayHAL::instance().init()) {
#if defined(ARDUINO)
        Serial.println("DisplayHAL init failed");
#else
        printf("DisplayHAL init failed\n");
#endif
        return false;
    }

    if (!TouchHAL::instance().init()) {
#if defined(ARDUINO)
        Serial.println("TouchHAL init failed");
#else
        printf("TouchHAL init failed\n");
#endif
    }

    if (!IMUHAL::instance().init()) {
#if defined(ARDUINO)
        Serial.println("IMUHAL init failed");
#else
        printf("IMUHAL init failed\n");
#endif
    }

    if (!PowerHAL::instance().init()) {
#if defined(ARDUINO)
        Serial.println("PowerHAL init failed");
#else
        printf("PowerHAL init failed\n");
#endif
    }

    g_personality_nvs.init();
    g_spicy.set_mood(g_personality_nvs.load_mood(Mood::CURIOUS));

    if (!current_app) {
        switch_app("home");
        handle_app_transition();
    }

#if defined(ARDUINO)
    Serial.println("Shell initialized.");
#else
    printf("Shell initialized.\n");
#endif

    return true;
}

void Shell::run() {
#if defined(ARDUINO)
    Serial.println("Shell::run() — starting main loop");
    uint32_t last_time = millis();
    while (1) {
        uint32_t now = millis();
        uint32_t dt = (last_time > 0 && now >= last_time) ? (now - last_time) : 16;
        last_time = now;

        tick(dt);

        vTaskDelay(pdMS_TO_TICKS(16));
    }
#else
    printf("Shell::run() — starting main loop\n");
    while (1) {
        tick(16);
    }
#endif
}

void Shell::tick(uint32_t dt) {
    poll_sensors();
    handle_app_transition();
    update_active_app(dt);
    render_and_flush();
}

void Shell::switch_app(App* app, const char* name) {
    next_app = app;
    next_app_name = name ? name : (app ? "custom" : "");
    app_transition_pending = true;
}

void Shell::switch_app(const char* app_name) {
    if (!app_name) {
        switch_app(static_cast<App*>(nullptr), nullptr);
        return;
    }

    if (strcmp(app_name, "home") == 0) {
        static HomeApp home_app;
        switch_app(&home_app, "home");
    } else if (strcmp(app_name, "scenes") == 0) {
        static ScenesApp scenes_app;
        switch_app(&scenes_app, "scenes");
    } else {
#if defined(ARDUINO)
        Serial.printf("Shell::switch_app: unknown app '%s'\n", app_name);
#else
        printf("Shell::switch_app: unknown app '%s'\n", app_name);
#endif
    }
}

void Shell::handle_app_transition() {
    if (app_transition_pending) {
        if (current_app) {
            current_app->on_exit();
        }

        current_app = next_app;
        next_app = nullptr;
        app_transition_pending = false;

        if (current_app) {
            current_app->on_enter();
        }

        Event e;
        e.type = EventType::APP_TRANSITION;
        e.data.app_transition.from_app = current_app_name ? current_app_name : "";
        e.data.app_transition.to_app = next_app_name ? next_app_name : "";
        current_app_name = next_app_name;
        next_app_name = nullptr;
        g_event_bus.publish(e);
    }
}

void Shell::poll_sensors() {
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    float gx = 0.0f, gy = 0.0f, gz = 0.0f;
    if (IMUHAL::instance().read_motion(&ax, &ay, &az, &gx, &gy, &gz)) {
        Event e;
        e.type = EventType::MOTION_EVENT;
        e.data.motion.accel_x = ax;
        e.data.motion.accel_y = ay;
        e.data.motion.accel_z = az;
        e.data.motion.gyro_x = gx;
        e.data.motion.gyro_y = gy;
        e.data.motion.gyro_z = gz;
        e.data.motion.intensity = sqrtf(ax * ax + ay * ay + az * az);
        e.data.motion.direction = 0;
        g_event_bus.publish(e);
    }

    uint16_t tx = 0, ty = 0;
    if (TouchHAL::instance().get_touch_point(&tx, &ty)) {
        Event e;
        e.type = EventType::TOUCH_EVENT;
        e.data.touch.x = tx;
        e.data.touch.y = ty;
        e.data.touch.duration_ms = 0;
        e.data.touch.intensity = 100;
        g_event_bus.publish(e);
    }
}

void Shell::update_active_app(uint32_t dt) {
    if (current_app) {
        current_app->update(dt);
    }
}

void Shell::render_and_flush() {
    if (current_app) {
        current_app->render(&screen_canvas);
        DisplayHAL::instance().flush(reinterpret_cast<uint8_t*>(screen_canvas.get_buffer()));
    }
}

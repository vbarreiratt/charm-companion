#include <Arduino.h>
#include "config/pin_config.h"

#include "shell/shell.h"
#include "shell/event_bus.h"
#include "shell/event_types.h"
#include "spicy/spicy.h"
#include "apps/scenes/scene_registry.h"
#include "apps/scenes/scenes_app.h"
#include "apps/scenes/planet_scene.h"
#include "apps/scenes/eye_scene.h"
#include "apps/home/home_app.h"
#include <cstring>

// Debug-only serial command console, for exercising touch/scene handling
// without physical finger input. Not a substitute for physical touch
// verification of TouchHAL/get_touch_point() itself — it publishes directly
// to g_event_bus, bypassing the CST9217 driver entirely.
//
// Commands (newline-terminated over the USB CDC serial port):
//   touch <x> <y>   publish a synthetic TOUCH_EVENT
//   next            ScenesApp::next_scene() (no-op if current app isn't scenes)
//   prev            ScenesApp::prev_scene()
//   app <name>      Shell::instance().switch_app(name), e.g. "app scenes"
//   swipe up        simulate an upward swipe (Home -> Scenes) via
//                    Shell::debug_inject_touch(), same path real touch uses
//   boot            simulate a BOOT-button press via Shell::handle_boot_press()
void handle_serial_command(const String& line) {
    if (line.startsWith("touch ")) {
        int sx = line.indexOf(' ');
        int sy = line.indexOf(' ', sx + 1);
        if (sx < 0 || sy < 0) {
            Serial.println("[cmd] usage: touch <x> <y>");
            return;
        }
        uint16_t x = static_cast<uint16_t>(line.substring(sx + 1, sy).toInt());
        uint16_t y = static_cast<uint16_t>(line.substring(sy + 1).toInt());
        Event e;
        e.type = EventType::TOUCH_EVENT;
        e.data.touch = {x, y, 0, 255};
        g_event_bus.publish(e);
        Serial.printf("[cmd] injected touch x=%u y=%u\n", x, y);
    } else if (line == "next" || line == "prev") {
        // No RTTI on this build (-fno-rtti), so we can't dynamic_cast the
        // current App* — use Shell's tracked app name to confirm it's safe
        // to static_cast, matching how Shell::switch_app("scenes") tags it.
        const char* name = Shell::instance().get_current_app_name();
        if (!name || strcmp(name, "scenes") != 0) {
            Serial.println("[cmd] current app is not ScenesApp — try 'app scenes' first");
            return;
        }
        auto* scenes = static_cast<ScenesApp*>(Shell::instance().get_current_app());
        if (line == "next") {
            scenes->next_scene();
        } else {
            scenes->prev_scene();
        }
        auto* scene = scenes->get_current_scene();
        Serial.printf("[cmd] scene now: %s\n", scene ? scene->name() : "(none)");
    } else if (line.startsWith("app ")) {
        String name = line.substring(4);
        Shell::instance().switch_app(name.c_str());
        Serial.printf("[cmd] switch_app(%s)\n", name.c_str());
    } else if (line == "swipe up") {
        Shell::instance().debug_inject_touch(233, 400, true);
        Shell::instance().debug_inject_touch(233, 300, true);
        Shell::instance().debug_inject_touch(0, 0, false);
        Serial.printf("[cmd] injected swipe up; current app: %s\n",
                       Shell::instance().get_current_app_name());
    } else if (line == "boot") {
        Shell::instance().handle_boot_press();
        Serial.printf("[cmd] simulated BOOT press; current app: %s\n",
                       Shell::instance().get_current_app_name());
    } else if (line.length() > 0) {
        Serial.printf("[cmd] unknown command: %s\n", line.c_str());
    }
}

void setup() {
    Serial.begin(115200);
    // Root cause of "touch only works while a serial monitor is attached":
    // HWCDC's default 100ms TX timeout (with up to 20 retries, ~2s total)
    // makes every Serial.print() block for up to ~2s when nobody is
    // reading the USB CDC port and its ring buffer fills up -- and
    // poll_sensors() prints on every detected touch, stalling the whole
    // main loop. Non-blocking: prints are dropped instead of stalling
    // firmware behavior on whether a host happens to be listening.
    Serial.setTxTimeoutMs(0);
    delay(1000);
    Serial.println("Charm Companion starting...");
    Serial.printf("Display: %d x %d\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);

    if (!psramInit()) {
        Serial.println("PSRAM init FAILED! Halting.");
        while (true) {
            delay(1000);
        }
    }
    Serial.printf("PSRAM: %u / %u bytes free\n", ESP.getFreePsram(), ESP.getPsramSize());

    // Register scenes
    SceneRegistry::instance().register_scene("planet", []() { return new PlanetScene(); });
    SceneRegistry::instance().register_scene("eye", []() { return new EyeScene(); });

    // Initialize Shell (which initializes HALs, sets Spicy mood, and activates default app)
    if (!Shell::instance().init()) {
        Serial.println("Shell init failed! Halting.");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("All systems initialized. Shell running.");
}

void loop() {
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            handle_serial_command(line);
        }
    }

    static uint32_t last_time = 0;
    uint32_t now = millis();
    uint32_t dt = (last_time > 0) ? (now - last_time) : 16;
    last_time = now;

    Shell::instance().tick(dt);
    delay(10); // yield
}

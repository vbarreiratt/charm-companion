#include <Arduino.h>
#include "config/pin_config.h"

#include "shell/shell.h"
#include "spicy/spicy.h"
#include "apps/scenes/scene_registry.h"
#include "apps/scenes/planet_scene.h"
#include "apps/scenes/eye_scene.h"
#include "apps/home/home_app.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Charm Companion starting...");
    Serial.printf("Display: %d x %d\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);

    if (!psramInit()) {
        Serial.println("PSRAM init FAILED");
    } else {
        Serial.printf("PSRAM: %u / %u bytes free\n", ESP.getFreePsram(), ESP.getPsramSize());
    }

    // Register scenes
    SceneRegistry::instance().register_scene("planet", []() { return new PlanetScene(); });
    SceneRegistry::instance().register_scene("eye", []() { return new EyeScene(); });

    // Initialize Shell (which initializes HALs, sets Spicy mood, and activates default app)
    if (!Shell::instance().init()) {
        Serial.println("Shell init failed!");
        return;
    }

    Serial.println("All systems initialized. Shell running.");
}

void loop() {
    Shell::instance().tick(16);
    delay(16);
}

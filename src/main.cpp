#include <Arduino.h>
#include "config/pin_config.h"

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

    // TODO: Initialize shell, start main loop
}

void loop() {
    delay(1000);
}

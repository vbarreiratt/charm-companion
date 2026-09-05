#include <Arduino.h>
#include "config/pin_config.h"

#include "shell/hal/display_hal.h"
#include "shell/hal/touch_hal.h"
#include "shell/hal/imu_hal.h"
#include "shell/hal/power_hal.h"

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

    DisplayHAL::instance().init();
    TouchHAL::instance().init();
    IMUHAL::instance().init();
    PowerHAL::instance().init();
    Serial.println("All HALs initialized.");

    // TODO: Initialize shell, start main loop
}

void loop() {
    delay(1000);
}

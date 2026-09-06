// src/shell/hal/display_hal.cpp
#include "shell/hal/display_hal.h"
#include "config/pin_config.h"
#include "utils/color_utils.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#else
#include <cstdio>
#endif

DisplayHAL& DisplayHAL::instance() {
    static DisplayHAL inst;
    return inst;
}

bool DisplayHAL::init() {
#if defined(ARDUINO)
    Serial.println("DisplayHAL::init() — CO5300 initialization");
    // CO5300 has a 1px address-window bug where any draw with width=1 or height=1
    // silently discards pixels; flush() always writes the full 466x466 frame in
    // one shot via draw16bitRGBBitmap(), which never hits that path.
    // Display rotation must stay 0 (CO5300 does not support hardware rotation).
    bus_ = new Arduino_ESP32QSPI(
        DISPLAY_CS_PIN, DISPLAY_CLK_PIN, DISPLAY_DATA0_PIN, DISPLAY_DATA1_PIN,
        DISPLAY_DATA2_PIN, DISPLAY_DATA3_PIN);
    panel_ = new Arduino_CO5300(
        bus_, DISPLAY_RESET_PIN, 0 /* rotation */, DISPLAY_WIDTH, DISPLAY_HEIGHT,
        6 /* col_offset1 */, 0 /* row_offset1 */, 0 /* col_offset2 */, 0 /* row_offset2 */);
    if (!panel_->begin()) {
        Serial.println("DisplayHAL: panel_->begin() failed");
        return false;
    }
    panel_->fillScreen(COLOR_BG_BLACK);
    panel_->setBrightness(128);
    return true;
#else
    printf("DisplayHAL::init() — CO5300 initialization\n");
    return true;
#endif
}

void DisplayHAL::flush(uint8_t* frame_buffer) {
#if defined(ARDUINO)
    // Full-frame PSRAM flush cycle takes ~56.7ms (~17 fps ceiling).
    if (panel_ && frame_buffer) {
        panel_->draw16bitRGBBitmap(0, 0, reinterpret_cast<uint16_t*>(frame_buffer),
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }
#else
    (void)frame_buffer;
#endif
}

void DisplayHAL::set_brightness(uint8_t percent) {
#if defined(ARDUINO)
    if (percent > 100) percent = 100;
    if (panel_) {
        panel_->setBrightness(static_cast<uint8_t>((static_cast<uint16_t>(percent) * 255) / 100));
    }
#else
    printf("DisplayHAL::set_brightness(%d%%)\n", percent);
#endif
}

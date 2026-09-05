// src/shell/hal/display_hal.cpp
#include "shell/hal/display_hal.h"
#include "config/pin_config.h"

#if defined(ARDUINO)
#include <Arduino.h>
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
#else
    printf("DisplayHAL::init() — CO5300 initialization\n");
#endif
    // TODO: Initialize Arduino_GFX with CO5300 driver (#41), Arduino_ESP32QSPI bus (#1),
    // and PSRAM framebuffer (TFT_USE_CANVAS=1).
    // Note: CO5300 has a 1px address window bug where any draw with width=1 or height=1
    // silently discards pixels. Full-frame PSRAM canvas is mandatory workaround.
    // Display rotation must be TFT_ROTATION=0 (CO5300 does not support hardware rotation).
    return true;
}

void DisplayHAL::flush(uint8_t* frame_buffer) {
    // TODO: Flush full PSRAM framebuffer (466x466x2 = 434KB buffer) to display via QSPI.
    // Full-frame PSRAM flush cycle takes ~56.7ms (~17 fps ceiling).
    // Must be guarded with recursive mutex if shared with set_brightness SPI commands.
#if defined(ARDUINO)
    Serial.println("DisplayHAL::flush() — not yet implemented");
#else
    printf("DisplayHAL::flush() — not yet implemented\n");
#endif
}

void DisplayHAL::set_brightness(uint8_t percent) {
    // TODO: Write CO5300 brightness register (0x51) via QSPI command.
    // Must be guarded by lockPanel()/unlockPanel() mutex to avoid SPI transaction race with flush task.
#if defined(ARDUINO)
    Serial.printf("DisplayHAL::set_brightness(%d%%)\n", percent);
#else
    printf("DisplayHAL::set_brightness(%d%%)\n", percent);
#endif
}

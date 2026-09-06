// src/shell/hal/display_hal.h
#ifndef SHELL_HAL_DISPLAY_HAL_H
#define SHELL_HAL_DISPLAY_HAL_H

#include <cstdint>

#if defined(ARDUINO)
class Arduino_DataBus;
class Arduino_CO5300;
#endif

class DisplayHAL {
public:
    static DisplayHAL& instance();

    bool init();
    void flush(uint8_t* frame_buffer);  // 434KB PSRAM buffer, RGB565
    void set_brightness(uint8_t percent);  // 0-100

private:
    DisplayHAL() = default;
    ~DisplayHAL() = default;  // process-lifetime singleton; no cleanup on embedded target

    DisplayHAL(const DisplayHAL&) = delete;
    DisplayHAL& operator=(const DisplayHAL&) = delete;

#if defined(ARDUINO)
    Arduino_DataBus* bus_ = nullptr;
    Arduino_CO5300* panel_ = nullptr;
#endif
};

#endif

#ifndef UTILS_COLOR_UTILS_H
#define UTILS_COLOR_UTILS_H

#include <cstdint>

// RGB565 color constants (from HARDWARE.md + design phase)
inline constexpr uint16_t COLOR_BG_BLACK       = 0x0000;  // #000000
inline constexpr uint16_t COLOR_ACCENT_RED     = 0xF9A6;  // #ff3333 (R=31, G=13, B=6)
inline constexpr uint16_t COLOR_MONO_NEUTRAL   = 0xE73C;  // #e8e8e8 (R=28, G=28, B=28)
inline constexpr uint16_t COLOR_MONO_DIM       = 0x39C7;  // #3a3a3a (R=7, G=7, B=7)
inline constexpr uint16_t COLOR_TEXT_SECONDARY = 0x9CF4;  // #9e9ea8 (R=19, G=29, B=20)

#endif // UTILS_COLOR_UTILS_H

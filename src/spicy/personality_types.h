#ifndef SPICY_PERSONALITY_TYPES_H
#define SPICY_PERSONALITY_TYPES_H

#include <cstdint>

enum class Mood : uint8_t {
    PLAYFUL,
    CURIOUS,
    CALM,
    ANGRY,
    SCARED,
    SAD,
    LOST,
};

struct Traits {
    uint8_t curiosity;      // 0-100
    uint8_t mischief;       // 0-100
    uint8_t patience;       // 0-100
    uint8_t creativity;     // 0-100
    uint8_t fashion_sense;  // 0-100
};

struct EmotionalState {
    uint8_t happiness;      // 0-100
    uint8_t irritation;     // 0-100
    uint8_t confidence;     // 0-100
    uint8_t playfulness;    // 0-100
};

struct Theme {
    uint16_t primary_color;        // RGB565
    uint16_t accent_color;         // RGB565
    uint16_t bg_color;             // RGB565
    float animation_speed_factor;  // 0.5 to 2.0
};

struct PersonalityContext {
    Mood mood;
    Traits traits;
    EmotionalState emotional_state;
    Theme theme;
};

#endif // SPICY_PERSONALITY_TYPES_H

#ifndef SPICY_PERSONALITY_API_H
#define SPICY_PERSONALITY_API_H

#include <cstdint>
#include "personality_types.h"

// New enums (define in personality_api.h)
enum class TextType : uint8_t {
    GREETING,
    REACTION,
    PREDICTION,
    STATUS,
};

enum class AnimationType : uint8_t {
    IDLE,
    EXCITED,
    CALM,
    CONFUSED,
};

struct AnimationStyle {
    float speed;       // 0.5 to 2.0
    uint8_t intensity; // 0-255
};

class PersonalityAPI {
public:
    PersonalityContext get_context() const;
    const char* get_tone(TextType type) const;
    AnimationStyle get_animation_style(AnimationType type) const;
    Theme get_theme_colors() const;
    void record_interaction(const char* interaction_type);  // Phase 2 stub
};

extern PersonalityAPI g_personality_api;

#endif // SPICY_PERSONALITY_API_H

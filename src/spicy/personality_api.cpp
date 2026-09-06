#include "personality_api.h"
#include "spicy.h"

PersonalityAPI g_personality_api;

PersonalityContext PersonalityAPI::get_context() const {
    return g_spicy.get_context();
}

const char* PersonalityAPI::get_tone(TextType type) const {
    auto ctx = get_context();

    // Tone varies by mood + text type
    if (ctx.mood == Mood::PLAYFUL) {
        switch (type) {
            case TextType::GREETING:
                return "Hiya! 💫";
            case TextType::REACTION:
                return "Woo! Fun!";
            case TextType::STATUS:
                return "Living best life!";
            default:
                return "Let's go!";
        }
    } else if (ctx.mood == Mood::CALM) {
        switch (type) {
            case TextType::GREETING:
                return "Hello.";
            case TextType::REACTION:
                return "Mmm, nice.";
            case TextType::STATUS:
                return "All good.";
            default:
                return "Take it slow.";
        }
    } else if (ctx.mood == Mood::CURIOUS) {
        switch (type) {
            case TextType::GREETING:
                return "Oh, hi! What's new?";
            case TextType::REACTION:
                return "Ooh, interesting...";
            case TextType::STATUS:
                return "Exploring!";
            default:
                return "Tell me more!";
        }
    }

    // Default
    return "...";
}

AnimationStyle PersonalityAPI::get_animation_style(AnimationType type) const {
    auto ctx = get_context();
    AnimationStyle style;

    // Base speed from mood
    style.speed = ctx.theme.animation_speed_factor;

    // Intensity from emotional state
    style.intensity = ctx.emotional_state.playfulness;

    // Adjust by animation type
    if (type == AnimationType::IDLE) {
        style.speed *= 1.0f;
    } else if (type == AnimationType::EXCITED) {
        style.speed *= 1.5f;
        style.intensity = 255;
    } else if (type == AnimationType::CALM) {
        style.speed *= 0.7f;
        style.intensity = 100;
    } else if (type == AnimationType::CONFUSED) {
        style.speed *= 0.8f;
        style.intensity = 150;
    }

    return style;
}

Theme PersonalityAPI::get_theme_colors() const {
    return get_context().theme;
}

void PersonalityAPI::record_interaction(const char* interaction_type) {
    // Phase 2: affects emotional state + traits over time
    (void)interaction_type;  // Silence unused warning
}

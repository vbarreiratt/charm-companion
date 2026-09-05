#include "spicy.h"
#include "shell/event_bus.h"
#include "utils/color_utils.h"

Spicy g_spicy;

static inline uint8_t clamp_trait(int value) {
    if (value < 0) return 0;
    if (value > 100) return 100;
    return static_cast<uint8_t>(value);
}

Spicy::Spicy() {
    context.mood = Mood::CURIOUS;
    apply_mood_modifiers(Mood::CURIOUS);
    select_theme_for_mood(Mood::CURIOUS);
}

Spicy::~Spicy() = default;

// NOTE: set_mood() must NOT be called from within an on_event() callback
// in Phase 1. EventBus::publish() holds a std::mutex during listener callbacks,
// so calling set_mood() (which publishes MOOD_CHANGED) from inside an on_event()
// callback would result in a deadlock.
void Spicy::set_mood(Mood m) {
    context.mood = m;
    apply_mood_modifiers(m);
    select_theme_for_mood(m);

    // Publish event
    Event e;
    e.type = EventType::MOOD_CHANGED;
    e.data.mood.personality_context = &context;  // safe: synchronous dispatch, g_spicy is global
    g_event_bus.publish(e);
}

PersonalityContext Spicy::get_context() const {
    return context;
}

void Spicy::apply_mood_modifiers(Mood m) {
    // Baseline traits before applying mood modifiers
    int curiosity = 60;
    int mischief = 40;
    int patience = 70;
    int creativity = 80;
    int fashion_sense = 90;

    // Baseline emotional state (all 0 initially, set per-mood)
    context.emotional_state.happiness = 0;
    context.emotional_state.irritation = 0;
    context.emotional_state.confidence = 0;
    context.emotional_state.playfulness = 0;

    // Mood-specific adjustments
    switch (m) {
        case Mood::PLAYFUL:
            curiosity += 20;
            mischief += 30;
            patience -= 20;
            context.emotional_state.playfulness = 100;
            context.emotional_state.happiness = 90;
            break;
        case Mood::CURIOUS:
            curiosity += 30;
            creativity += 10;
            context.emotional_state.playfulness = 70;
            break;
        case Mood::CALM:
            patience += 30;
            mischief -= 20;
            context.emotional_state.playfulness = 30;
            context.emotional_state.happiness = 70;
            break;
        case Mood::ANGRY:
            patience -= 50;
            mischief += 20;
            context.emotional_state.irritation = 100;
            context.emotional_state.happiness = 20;
            break;
        case Mood::SCARED:
            patience -= 20;
            context.emotional_state.confidence = 20;
            context.emotional_state.playfulness = 10;
            break;
        case Mood::SAD:
            creativity -= 10;
            context.emotional_state.happiness = 30;
            context.emotional_state.playfulness = 20;
            break;
        case Mood::LOST:
            curiosity -= 20;
            context.emotional_state.confidence = 40;
            context.emotional_state.playfulness = 50;
            break;
    }

    // Clamp traits to [0, 100] to prevent arithmetic underflow/overflow wrapping
    context.traits.curiosity = clamp_trait(curiosity);
    context.traits.mischief = clamp_trait(mischief);
    context.traits.patience = clamp_trait(patience);
    context.traits.creativity = clamp_trait(creativity);
    context.traits.fashion_sense = clamp_trait(fashion_sense);
}

void Spicy::select_theme_for_mood(Mood m) {
    switch (m) {
        case Mood::PLAYFUL:
            context.theme.primary_color = COLOR_ACCENT_RED;
            context.theme.accent_color = COLOR_ACCENT_RED;
            context.theme.bg_color = COLOR_BG_BLACK;
            context.theme.animation_speed_factor = 1.5f;
            break;
        case Mood::CURIOUS:
            context.theme.primary_color = 0xFBE0;  // Cyan-ish
            context.theme.accent_color = 0xFFE0;   // Yellow-ish
            context.theme.bg_color = COLOR_BG_BLACK;
            context.theme.animation_speed_factor = 1.0f;
            break;
        case Mood::CALM:
            context.theme.primary_color = 0x8E71;  // Blue-ish
            context.theme.accent_color = COLOR_TEXT_SECONDARY;
            context.theme.bg_color = COLOR_BG_BLACK;
            context.theme.animation_speed_factor = 0.7f;
            break;
        case Mood::ANGRY:
            context.theme.primary_color = COLOR_ACCENT_RED;
            context.theme.accent_color = 0xFBE0;
            context.theme.bg_color = COLOR_BG_BLACK;
            context.theme.animation_speed_factor = 1.8f;
            break;
        case Mood::SCARED:
            context.theme.primary_color = 0xFD20;
            context.theme.accent_color = COLOR_MONO_DIM;
            context.theme.bg_color = COLOR_BG_BLACK;
            context.theme.animation_speed_factor = 1.6f;
            break;
        case Mood::SAD:
            context.theme.primary_color = 0x4A69;
            context.theme.accent_color = COLOR_MONO_DIM;
            context.theme.bg_color = COLOR_BG_BLACK;
            context.theme.animation_speed_factor = 0.5f;
            break;
        case Mood::LOST:
            context.theme.primary_color = COLOR_TEXT_SECONDARY;
            context.theme.accent_color = COLOR_MONO_DIM;
            context.theme.bg_color = COLOR_BG_BLACK;
            context.theme.animation_speed_factor = 0.8f;
            break;
        default:
            context.theme.primary_color = COLOR_MONO_NEUTRAL;
            context.theme.accent_color = COLOR_MONO_DIM;
            context.theme.bg_color = COLOR_BG_BLACK;
            context.theme.animation_speed_factor = 1.0f;
            break;
    }
}

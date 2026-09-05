#ifndef SPICY_SPICY_H
#define SPICY_SPICY_H

#include "personality_types.h"

class Spicy {
public:
    Spicy();
    ~Spicy();

    // NOTE: set_mood() must NOT be called from within an on_event() callback
    // in Phase 1. EventBus::publish() holds a std::mutex during listener callbacks,
    // so calling set_mood() (which publishes MOOD_CHANGED) from inside an on_event()
    // callback would result in a deadlock.
    void set_mood(Mood m);
    PersonalityContext get_context() const;

private:
    PersonalityContext context;

    void apply_mood_modifiers(Mood m);
    void select_theme_for_mood(Mood m);
};

extern Spicy g_spicy;

#endif // SPICY_SPICY_H

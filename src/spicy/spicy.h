#ifndef SPICY_SPICY_H
#define SPICY_SPICY_H

#include "personality_types.h"

class Spicy {
public:
    Spicy();
    ~Spicy();

    // set_mood() is safe to call from within an on_event() callback: EventBus::publish()
    // snapshots its listener list under lock and releases the lock before invoking any
    // listener, so a cascaded publish() from inside a callback does not deadlock.
    void set_mood(Mood m);
    PersonalityContext get_context() const;

private:
    PersonalityContext context;

    void apply_mood_modifiers(Mood m);
    void select_theme_for_mood(Mood m);
};

extern Spicy g_spicy;

#endif // SPICY_SPICY_H

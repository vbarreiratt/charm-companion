// src/spicy/personality_nvs.h
#ifndef SPICY_PERSONALITY_NVS_H
#define SPICY_PERSONALITY_NVS_H

#include "personality_types.h"

class PersonalityNVS {
public:
    void init();
    void save_mood(Mood m);
    Mood load_mood(Mood default_mood);

#if !defined(ARDUINO)
    // Test-only: clears the in-memory native store between tests.
    void reset_for_testing();
#endif
};

extern PersonalityNVS g_personality_nvs;

#endif // SPICY_PERSONALITY_NVS_H

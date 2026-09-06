// src/spicy/personality_nvs.cpp
#include "personality_nvs.h"

#if defined(ARDUINO)
#include <Preferences.h>
static Preferences s_prefs;
#else
#include <cstdio>
static bool s_has_saved_mood = false;
static Mood s_saved_mood = Mood::CURIOUS;
#endif

PersonalityNVS g_personality_nvs;

void PersonalityNVS::init() {
#if defined(ARDUINO)
    // Namespace is opened per-operation (begin/end) in save_mood/load_mood
    // rather than held open for the app's lifetime.
#else
    printf("PersonalityNVS::init() (native stub)\n");
#endif
}

void PersonalityNVS::save_mood(Mood m) {
#if defined(ARDUINO)
    s_prefs.begin("spicy", false);
    s_prefs.putUChar("mood", static_cast<uint8_t>(m));
    s_prefs.end();
#else
    s_has_saved_mood = true;
    s_saved_mood = m;
#endif
}

Mood PersonalityNVS::load_mood(Mood default_mood) {
#if defined(ARDUINO)
    s_prefs.begin("spicy", true);
    uint8_t stored = s_prefs.getUChar("mood", static_cast<uint8_t>(default_mood));
    s_prefs.end();
    return static_cast<Mood>(stored);
#else
    return s_has_saved_mood ? s_saved_mood : default_mood;
#endif
}

#if !defined(ARDUINO)
void PersonalityNVS::reset_for_testing() {
    s_has_saved_mood = false;
    s_saved_mood = Mood::CURIOUS;
}
#endif

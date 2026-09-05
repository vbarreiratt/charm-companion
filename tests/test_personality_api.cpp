#include <gtest/gtest.h>
#include <cstring>
#include "spicy/personality_api.h"
#include "spicy/spicy.h"

TEST(PersonalityAPITest, GetToneForGreeting) {
    PersonalityAPI api;
    g_spicy.set_mood(Mood::PLAYFUL);

    const char* tone = api.get_tone(TextType::GREETING);
    ASSERT_NE(tone, nullptr);
}

TEST(PersonalityAPITest, GetThemeColors) {
    PersonalityAPI api;
    g_spicy.set_mood(Mood::CURIOUS);

    Theme theme = api.get_theme_colors();
    ASSERT_NE(theme.primary_color, 0x0000);
}

TEST(PersonalityAPITest, GetAnimationStyle) {
    PersonalityAPI api;
    g_spicy.set_mood(Mood::PLAYFUL);

    auto style = api.get_animation_style(AnimationType::IDLE);
    ASSERT_GT(style.speed, 1.0f);

    g_spicy.set_mood(Mood::CALM);
    style = api.get_animation_style(AnimationType::IDLE);
    ASSERT_LT(style.speed, 1.0f);
}

TEST(PersonalityAPITest, GetToneVariations) {
    PersonalityAPI api;

    // Calm tones
    g_spicy.set_mood(Mood::CALM);
    EXPECT_STREQ(api.get_tone(TextType::GREETING), "Hello.");
    EXPECT_STREQ(api.get_tone(TextType::REACTION), "Mmm, nice.");
    EXPECT_STREQ(api.get_tone(TextType::STATUS), "All good.");
    EXPECT_STREQ(api.get_tone(TextType::PREDICTION), "Take it slow.");

    // Curious tones
    g_spicy.set_mood(Mood::CURIOUS);
    EXPECT_STREQ(api.get_tone(TextType::GREETING), "Oh, hi! What's new?");
    EXPECT_STREQ(api.get_tone(TextType::REACTION), "Ooh, interesting...");
    EXPECT_STREQ(api.get_tone(TextType::STATUS), "Exploring!");
    EXPECT_STREQ(api.get_tone(TextType::PREDICTION), "Tell me more!");

    // Default tone for unhandled mood
    g_spicy.set_mood(Mood::SAD);
    EXPECT_STREQ(api.get_tone(TextType::GREETING), "...");
}

TEST(PersonalityAPITest, AnimationStyleExcitedAndCalm) {
    PersonalityAPI api;
    g_spicy.set_mood(Mood::PLAYFUL);

    auto base_speed = api.get_context().theme.animation_speed_factor;
    auto excited = api.get_animation_style(AnimationType::EXCITED);
    EXPECT_FLOAT_EQ(excited.speed, base_speed * 1.5f);
    EXPECT_EQ(excited.intensity, 255);

    auto calm = api.get_animation_style(AnimationType::CALM);
    EXPECT_FLOAT_EQ(calm.speed, base_speed * 0.7f);
    EXPECT_EQ(calm.intensity, 100);

    auto confused = api.get_animation_style(AnimationType::CONFUSED);
    EXPECT_FLOAT_EQ(confused.speed, base_speed);
    EXPECT_EQ(confused.intensity, api.get_context().emotional_state.playfulness);
}

TEST(PersonalityAPITest, GlobalInstanceAndInteraction) {
    g_spicy.set_mood(Mood::CURIOUS);
    auto ctx = g_personality_api.get_context();
    EXPECT_EQ(ctx.mood, Mood::CURIOUS);

    // Phase 2 stub should be callable without error
    g_personality_api.record_interaction("tap");
}

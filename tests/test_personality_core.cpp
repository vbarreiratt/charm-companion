#include <gtest/gtest.h>
#include "spicy/spicy.h"
#include "shell/event_bus.h"

class MoodChangeListener : public Listener {
public:
    int call_count = 0;
    Mood last_mood = Mood::CURIOUS;
    const PersonalityContext* last_context = nullptr;

    void on_event(const Event& e) override {
        if (e.type == EventType::MOOD_CHANGED) {
            call_count++;
            last_context = static_cast<const PersonalityContext*>(e.data.mood.personality_context);
            if (last_context) {
                last_mood = last_context->mood;
            }
        }
    }
};

TEST(SpicyTest, SetMoodPublishesEvent) {
    MoodChangeListener listener;
    g_event_bus.subscribe(EventType::MOOD_CHANGED, &listener);

    g_spicy.set_mood(Mood::PLAYFUL);

    ASSERT_EQ(listener.call_count, 1);
    ASSERT_EQ(listener.last_mood, Mood::PLAYFUL);
    ASSERT_NE(listener.last_context, nullptr);
    ASSERT_EQ(listener.last_context->mood, Mood::PLAYFUL);

    g_event_bus.unsubscribe(EventType::MOOD_CHANGED, &listener);
}

TEST(SpicyTest, MoodCascadesToTraits) {
    Spicy spicy;
    spicy.set_mood(Mood::PLAYFUL);

    auto ctx = spicy.get_context();
    ASSERT_GT(ctx.traits.curiosity, 50);   // Playful = curious (60 + 20 = 80 > 50)
    ASSERT_GT(ctx.traits.mischief, 50);    // Playful = mischievous (40 + 30 = 70 > 50)
    ASSERT_LE(ctx.traits.patience, 50);    // Playful = impatient (baseline 70 - 20 = 50 <= 50)
    ASSERT_LT(ctx.traits.patience, 70);    // Decreased from baseline
}

TEST(SpicyTest, MoodSelectsThemeColors) {
    Spicy spicy;
    spicy.set_mood(Mood::PLAYFUL);

    auto ctx = spicy.get_context();
    auto playful_primary = ctx.theme.primary_color;

    spicy.set_mood(Mood::CALM);
    ctx = spicy.get_context();
    auto calm_primary = ctx.theme.primary_color;

    ASSERT_NE(playful_primary, calm_primary);  // Different moods, different colors
}

TEST(SpicyTest, TraitClampingPreventsUnderflow) {
    Spicy spicy;
    // ANGRY reduces patience by 50 (baseline 70 -> 20)
    spicy.set_mood(Mood::ANGRY);
    auto ctx = spicy.get_context();
    ASSERT_LE(ctx.traits.patience, 100);
    ASSERT_GE(ctx.traits.patience, 0);
    ASSERT_EQ(ctx.traits.patience, 20);

    // CALM reduces mischief by 20 (baseline 40 -> 20)
    spicy.set_mood(Mood::CALM);
    ctx = spicy.get_context();
    ASSERT_LE(ctx.traits.mischief, 100);
    ASSERT_GE(ctx.traits.mischief, 0);
    ASSERT_EQ(ctx.traits.mischief, 20);
}

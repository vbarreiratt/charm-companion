#include <gtest/gtest.h>
#include "spicy/personality_nvs.h"

class PersonalityNVSTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_personality_nvs.reset_for_testing();
    }
};

TEST_F(PersonalityNVSTest, LoadReturnsDefaultWhenNeverSaved) {
    Mood m = g_personality_nvs.load_mood(Mood::CALM);
    EXPECT_EQ(m, Mood::CALM);
}

TEST_F(PersonalityNVSTest, SaveThenLoadRoundTrips) {
    g_personality_nvs.save_mood(Mood::PLAYFUL);
    Mood m = g_personality_nvs.load_mood(Mood::CURIOUS);
    EXPECT_EQ(m, Mood::PLAYFUL);
}

TEST_F(PersonalityNVSTest, SavingDifferentMoodOverwritesPrevious) {
    g_personality_nvs.save_mood(Mood::ANGRY);
    g_personality_nvs.save_mood(Mood::SAD);
    Mood m = g_personality_nvs.load_mood(Mood::CURIOUS);
    EXPECT_EQ(m, Mood::SAD);
}

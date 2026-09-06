#include <gtest/gtest.h>
#include "shell/swipe_detector.h"

TEST(SwipeDetectorTest, NoTouchProducesNone) {
    SwipeDetector d;
    EXPECT_EQ(d.feed(false, 0, 0), SwipeDirection::NONE);
}

TEST(SwipeDetectorTest, ShortMovementBelowThresholdProducesNone) {
    SwipeDetector d;
    d.feed(true, 233, 400);
    d.feed(true, 233, 380);  // only 20px, below threshold
    EXPECT_EQ(d.feed(false, 0, 0), SwipeDirection::NONE);
}

TEST(SwipeDetectorTest, UpwardSwipePastThresholdProducesUp) {
    SwipeDetector d;
    d.feed(true, 233, 400);
    d.feed(true, 233, 300);  // 100px up
    EXPECT_EQ(d.feed(false, 0, 0), SwipeDirection::UP);
}

TEST(SwipeDetectorTest, DownwardSwipeProducesDown) {
    SwipeDetector d;
    d.feed(true, 233, 200);
    d.feed(true, 233, 320);  // 120px down
    EXPECT_EQ(d.feed(false, 0, 0), SwipeDirection::DOWN);
}

TEST(SwipeDetectorTest, HorizontalSwipeDominatesOverSmallVertical) {
    SwipeDetector d;
    d.feed(true, 100, 233);
    d.feed(true, 250, 250);  // dx=150, dy=-17 -> RIGHT
    EXPECT_EQ(d.feed(false, 0, 0), SwipeDirection::RIGHT);
}

TEST(SwipeDetectorTest, StillActiveTouchReturnsNoneEachTick) {
    SwipeDetector d;
    EXPECT_EQ(d.feed(true, 233, 400), SwipeDirection::NONE);
    EXPECT_EQ(d.feed(true, 233, 300), SwipeDirection::NONE);  // mid-swipe, not released yet
}

TEST(SwipeDetectorTest, NewTouchAfterSwipeResetsState) {
    SwipeDetector d;
    d.feed(true, 233, 400);
    d.feed(true, 233, 300);
    EXPECT_EQ(d.feed(false, 0, 0), SwipeDirection::UP);

    // second gesture, starting fresh
    d.feed(true, 233, 400);
    d.feed(true, 233, 395);  // tiny movement
    EXPECT_EQ(d.feed(false, 0, 0), SwipeDirection::NONE);
}

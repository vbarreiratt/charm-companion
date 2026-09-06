#include <gtest/gtest.h>
#include "utils/canvas_wrapper.h"

TEST(CanvasTest, ConstructsWithZeroedBuffer) {
    Canvas c(10, 10);
    EXPECT_EQ(c.width(), 10);
    EXPECT_EQ(c.height(), 10);
    EXPECT_EQ(c.get_pixel(0, 0), 0);
    EXPECT_EQ(c.get_pixel(9, 9), 0);
}

TEST(CanvasTest, FillScreenSetsAllPixels) {
    Canvas c(4, 4);
    c.fill_screen(0xFFFF);
    for (int16_t y = 0; y < 4; ++y) {
        for (int16_t x = 0; x < 4; ++x) {
            EXPECT_EQ(c.get_pixel(x, y), 0xFFFF);
        }
    }
}

TEST(CanvasTest, DrawPixelSetsSinglePixel) {
    Canvas c(5, 5);
    c.draw_pixel(2, 3, 0x1234);
    EXPECT_EQ(c.get_pixel(2, 3), 0x1234);
    EXPECT_EQ(c.get_pixel(2, 2), 0);
}

TEST(CanvasTest, DrawPixelOutOfBoundsDoesNotCrash) {
    Canvas c(5, 5);
    c.draw_pixel(-1, 0, 0x1234);
    c.draw_pixel(0, -1, 0x1234);
    c.draw_pixel(5, 0, 0x1234);
    c.draw_pixel(0, 5, 0x1234);
    for (int16_t y = 0; y < 5; ++y) {
        for (int16_t x = 0; x < 5; ++x) {
            EXPECT_EQ(c.get_pixel(x, y), 0);
        }
    }
}

TEST(CanvasTest, FillCircleSetsPixelsWithinRadius) {
    Canvas c(21, 21);
    c.fill_circle(10, 10, 5, 0xFFFF);
    EXPECT_EQ(c.get_pixel(10, 10), 0xFFFF);   // center
    EXPECT_EQ(c.get_pixel(10, 5), 0xFFFF);    // top edge, distance == radius
    EXPECT_EQ(c.get_pixel(10, 0), 0);         // far outside radius
}

TEST(CanvasTest, DrawCircleSetsEdgePixelsOnly) {
    Canvas c(21, 21);
    c.draw_circle(10, 10, 5, 0xFFFF);
    EXPECT_EQ(c.get_pixel(15, 10), 0xFFFF);  // rightmost edge point (cx+r, cy)
    EXPECT_EQ(c.get_pixel(10, 10), 0);       // center not filled
}

TEST(CanvasTest, DrawCircleOctantPointsCorrect) {
    Canvas c(21, 21);
    c.draw_circle(10, 10, 5, 0xFFFF);
    // Verify octant points from hand-traced Midpoint Circle algorithm
    // For r=5: sequence is (5,0),(5,1),(5,2),(4,3)
    EXPECT_EQ(c.get_pixel(14, 13), 0xFFFF);  // (cx+4, cy+3) - catches algorithm bug
    EXPECT_EQ(c.get_pixel(15, 11), 0xFFFF);  // (cx+5, cy+1) - another verified point
}

TEST(CanvasTest, FillRectSetsBoundedRegion) {
    Canvas c(10, 10);
    c.fill_rect(2, 2, 3, 3, 0xABCD);
    EXPECT_EQ(c.get_pixel(2, 2), 0xABCD);
    EXPECT_EQ(c.get_pixel(4, 4), 0xABCD);
    EXPECT_EQ(c.get_pixel(5, 5), 0);
    EXPECT_EQ(c.get_pixel(1, 1), 0);
}

TEST(CanvasTest, DrawLineConnectsEndpoints) {
    Canvas c(10, 1);
    c.draw_line(0, 0, 9, 0, 0x5555);
    for (int16_t x = 0; x < 10; ++x) {
        EXPECT_EQ(c.get_pixel(x, 0), 0x5555);
    }
}

TEST(CanvasTest, BufferSizeMatchesDimensions) {
    Canvas c(466, 466);
    EXPECT_EQ(c.buffer_size_bytes(), static_cast<size_t>(466) * 466 * 2);
    EXPECT_NE(c.get_buffer(), nullptr);
}

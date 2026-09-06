#include <gtest/gtest.h>
#include "shell/hal/touch_hal.h"

// Reference protocol: waveshareteam/esp32-badge's cached
// waveshare__esp_lcd_touch_cst9217/esp_lcd_touch_cst9217.c
// esp_lcd_touch_cst9217_read_data(): a 10-byte read from the CST9217's
// data register. Byte layout for a single touch point:
//   data[0] low nibble = status (0x06 = finger down)
//   data[1] = x high byte, data[3] high nibble = x low nibble
//   data[2] = y high byte, data[3] low nibble  = y low nibble
//   data[5] low 7 bits    = number of touch points
//   data[6]               = ACK byte, must equal 0xAB

TEST(TouchHALParseTest, ValidSinglePointIsParsed) {
    // x=233 -> data[1]=0x0E, x-low-nibble=0x9
    // y=100 -> data[2]=0x06, y-low-nibble=0x4
    uint8_t data[10] = {0x06, 0x0E, 0x06, 0x94, 0x00, 0x01, 0xAB, 0x00, 0x00, 0x00};

    uint16_t x = 0, y = 0;
    EXPECT_TRUE(TouchHAL::parse_touch_data(data, &x, &y));
    EXPECT_EQ(x, 233);
    EXPECT_EQ(y, 100);
}

TEST(TouchHALParseTest, WrongAckByteIsRejected) {
    uint8_t data[10] = {0x06, 0x0E, 0x06, 0x94, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};

    uint16_t x = 0, y = 0;
    EXPECT_FALSE(TouchHAL::parse_touch_data(data, &x, &y));
}

TEST(TouchHALParseTest, ZeroPointsIsRejected) {
    uint8_t data[10] = {0x06, 0x0E, 0x06, 0x94, 0x00, 0x00, 0xAB, 0x00, 0x00, 0x00};

    uint16_t x = 0, y = 0;
    EXPECT_FALSE(TouchHAL::parse_touch_data(data, &x, &y));
}

TEST(TouchHALParseTest, FingerUpStatusIsRejected) {
    uint8_t data[10] = {0x00, 0x0E, 0x06, 0x94, 0x00, 0x01, 0xAB, 0x00, 0x00, 0x00};

    uint16_t x = 0, y = 0;
    EXPECT_FALSE(TouchHAL::parse_touch_data(data, &x, &y));
}

// config/pin_config.h
#ifndef CONFIG_PIN_CONFIG_H
#define CONFIG_PIN_CONFIG_H

// Display (QSPI) — CO5300
#define DISPLAY_DATA0_PIN  4
#define DISPLAY_DATA1_PIN  5
#define DISPLAY_DATA2_PIN  6
#define DISPLAY_DATA3_PIN  7
#define DISPLAY_CLK_PIN    38
#define DISPLAY_CS_PIN     12
#define DISPLAY_RESET_PIN  1

// Touch — CST9217
#define TOUCH_SDA_PIN      15
#define TOUCH_SCL_PIN      14
#define TOUCH_INT_PIN      11
#define TOUCH_RESET_PIN    2

// IMU — QMI8658
#define IMU_SDA_PIN        15  // Shared I2C
#define IMU_SCL_PIN        14

// Power — AXP2101
#define PMU_SDA_PIN        15  // Shared I2C
#define PMU_SCL_PIN        14

// Audio (phase 2+)
#define AUDIO_BCLK_PIN     9
#define AUDIO_LRCK_PIN     45
#define AUDIO_DIN_PIN      10
#define AUDIO_MCLK_PIN     16
#define AUDIO_DOUT_PIN     8
#define AUDIO_PA_PIN       46

// User Input
#define BOOT_BUTTON_PIN    0

// Display resolution
#define DISPLAY_WIDTH      466
#define DISPLAY_HEIGHT     466

#endif

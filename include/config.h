#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =======================
// Chế độ chạy
// =======================
// 1 = dùng simulator
// 0 = dùng GPS thật
#define USE_SIMULATOR 1

// =======================
// Debug
// =======================
// 1 = in NMEA GPS ra Serial Monitor
// 0 = tắt debug GPS
#define GPS_DEBUG_NMEA 1

// =======================
// OLED SSD1306
// =======================
#define OLED_SCREEN_WIDTH 128
#define OLED_SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

// =======================
// GPS NEO-6M
// =======================
#define GPS_BAUDRATE 9600
#define GPS_SIGNAL_TIMEOUT_MS 2500UL

// =======================
// Buzzer
// =======================
#define BUZZER_PIN PB0

// Chạy thật: 1000m = 1km
// Test nhanh có thể đổi thành 100.0f
#define BUZZER_DISTANCE_STEP_M 1000.0f

#define BUZZER_BEEP_TIMES 3
#define BUZZER_BEEP_ON_MS 200UL
#define BUZZER_BEEP_OFF_MS 200UL

// =======================
// Button
// =======================
// B1 = Confirm / Start
// B2 = Next / Back when long pressed
#define BUTTON_B1_PIN PB10
#define BUTTON_B2_PIN PB11

// Nút đấu 1 chân vào GPIO, 1 chân vào GND
// dùng INPUT_PULLUP nên nhấn = LOW
#define BUTTON_DEBOUNCE_MS 30UL
#define BUTTON_LONG_PRESS_MS 1000UL

// =======================
// Chu kỳ cập nhật OLED
// =======================
#define DISPLAY_UPDATE_INTERVAL_MS 1000UL

// =======================
// MicroSD history logger
// =======================

// SPI1 STM32F103:
// SCK  = PA5
// MISO = PA6
// MOSI = PA7
// CS   = PA4
#define SD_CS_PIN PA4

// Khi test nên để 1MHz cho ổn định.
// Sau này ổn rồi có thể tăng lên 4 hoặc 8.
#define SD_SPI_SPEED_MHZ 1

// File lưu lịch sử các buổi chạy
// Nên dùng tên 8.3 cho an toàn với FAT
#define SD_HISTORY_FILE "RUNS.CSV"

// File test đọc/ghi SD
#define SD_TEST_FILE "TEST.TXT"

// 1 = in log SD ra Serial Monitor
// 0 = tắt log
#define SD_HISTORY_DEBUG 1

#endif
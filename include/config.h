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
#define GPS_DEBUG_NMEA 0

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
#define SD_TEST_FILE "SDTEST.TXT"

// 1 = in log SD ra Serial Monitor
// 0 = tắt log
#define SD_HISTORY_DEBUG 0

// =======================
// History test mode
// =======================
// 1 = dùng dữ liệu mẫu, test được khi chưa cắm MicroSD
// 0 = dùng MicroSD thật, nếu không có SD sẽ báo lỗi trên OLED
#define SD_HISTORY_USE_SAMPLE_DATA 0

// GPS time là UTC, Việt Nam +7
#define GPS_TIMEZONE_OFFSET_HOURS 7

// =======================
// Run tracker algorithm
// =======================
#define RUN_TRACKER_MIN_STEP_M 1.5f
#define RUN_TRACKER_MAX_SPEED_MPS 12.0f

#define RUN_TRACKER_AUTO_PAUSE_SPEED_KMPH 1.0f
#define RUN_TRACKER_AUTO_RESUME_SPEED_KMPH 2.0f
#define RUN_TRACKER_AUTO_RESUME_DISTANCE_M 5.0f
#define RUN_TRACKER_AUTO_PAUSE_DELAY_MS 5000UL

#endif
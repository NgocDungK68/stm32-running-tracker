#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

void buzzer_init(int pin);

// Hàm tổng quát: kêu beepCount lần
void buzzer_start(
    uint8_t beepCount = 3,
    unsigned long onMs = 200,
    unsigned long offMs = 200
);

// Kêu 1 cái: dùng cho start / pause / continue / stop
void buzzer_beep_once();

// Kêu nhiều cái: dùng cho mốc 1 km
void buzzer_beep_kilometer();

void buzzer_update();
void buzzer_stop();

bool buzzer_isBusy();

#endif
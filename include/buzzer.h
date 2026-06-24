#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

void buzzer_init(int pin);

// Mặc định kêu 3 lần, mỗi lần bật 200ms, nghỉ 200ms
void buzzer_start(int times = 3, unsigned long onMs = 200, unsigned long offMs = 200);

void buzzer_update();
void buzzer_stop();

bool buzzer_isBusy();

#endif
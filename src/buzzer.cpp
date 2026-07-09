#include "buzzer.h"
#include "config.h"

#include <Arduino.h>

static int buzzerOutputPin = -1;

static bool buzzerActive = false;
static bool buzzerOutputHigh = false;

static uint8_t targetBeepCount = 0;
static uint8_t completedBeepCount = 0;

static unsigned long beepOnDurationMs = BUZZER_BEEP_ON_MS;
static unsigned long beepOffDurationMs = BUZZER_BEEP_OFF_MS;
static unsigned long lastBuzzerChangeTime = 0;

void buzzer_init(int pin) {
    buzzerOutputPin = pin;

    pinMode(buzzerOutputPin, OUTPUT);
    digitalWrite(buzzerOutputPin, LOW);

    buzzerActive = false;
    buzzerOutputHigh = false;

    targetBeepCount = 0;
    completedBeepCount = 0;

    lastBuzzerChangeTime = 0;
}

void buzzer_start(
    uint8_t beepCount,
    unsigned long onMs,
    unsigned long offMs
) {
    if (buzzerOutputPin < 0) {
        return;
    }

    if (beepCount == 0) {
        return;
    }

    targetBeepCount = beepCount;
    completedBeepCount = 0;

    beepOnDurationMs = onMs;
    beepOffDurationMs = offMs;

    buzzerActive = true;
    buzzerOutputHigh = true;

    digitalWrite(buzzerOutputPin, HIGH);
    lastBuzzerChangeTime = millis();
}

void buzzer_beep_once() {
    buzzer_start(
        1,
        BUZZER_BEEP_ON_MS,
        BUZZER_BEEP_OFF_MS
    );
}

void buzzer_beep_kilometer() {
    buzzer_start(
        BUZZER_BEEP_TIMES,
        BUZZER_BEEP_ON_MS,
        BUZZER_BEEP_OFF_MS
    );
}

void buzzer_update() {
    if (!buzzerActive) {
        return;
    }

    unsigned long now = millis();

    if (buzzerOutputHigh) {
        if (now - lastBuzzerChangeTime >= beepOnDurationMs) {
            buzzerOutputHigh = false;
            digitalWrite(buzzerOutputPin, LOW);

            completedBeepCount++;
            lastBuzzerChangeTime = now;

            if (completedBeepCount >= targetBeepCount) {
                buzzerActive = false;
                digitalWrite(buzzerOutputPin, LOW);
            }
        }
    } else {
        if (now - lastBuzzerChangeTime >= beepOffDurationMs) {
            buzzerOutputHigh = true;
            digitalWrite(buzzerOutputPin, HIGH);

            lastBuzzerChangeTime = now;
        }
    }
}

void buzzer_stop() {
    buzzerActive = false;
    buzzerOutputHigh = false;

    if (buzzerOutputPin >= 0) {
        digitalWrite(buzzerOutputPin, LOW);
    }
}

bool buzzer_isBusy() {
    return buzzerActive;
}
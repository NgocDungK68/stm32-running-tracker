#include "buzzer.h"
#include <Arduino.h>

static int buzzerPin = -1;

static bool active = false;
static bool buzzerOn = false;

static int targetBeeps = 0;
static int doneBeeps = 0;

static unsigned long beepOnTime = 200;
static unsigned long beepOffTime = 200;
static unsigned long lastChangeTime = 0;

void buzzer_init(int pin) {
    buzzerPin = pin;

    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);

    active = false;
    buzzerOn = false;
    targetBeeps = 0;
    doneBeeps = 0;
}

void buzzer_start(int times, unsigned long onMs, unsigned long offMs) {
    if (buzzerPin < 0) return;
    if (times <= 0) return;

    targetBeeps = times;
    doneBeeps = 0;

    beepOnTime = onMs;
    beepOffTime = offMs;

    active = true;
    buzzerOn = true;

    digitalWrite(buzzerPin, HIGH);
    lastChangeTime = millis();
}

void buzzer_update() {
    if (!active) return;

    unsigned long now = millis();

    if (buzzerOn) {
        if (now - lastChangeTime >= beepOnTime) {
            buzzerOn = false;
            digitalWrite(buzzerPin, LOW);

            doneBeeps++;
            lastChangeTime = now;

            if (doneBeeps >= targetBeeps) {
                active = false;
                digitalWrite(buzzerPin, LOW);
            }
        }
    } else {
        if (now - lastChangeTime >= beepOffTime) {
            buzzerOn = true;
            digitalWrite(buzzerPin, HIGH);

            lastChangeTime = now;
        }
    }
}

void buzzer_stop() {
    active = false;
    buzzerOn = false;

    if (buzzerPin >= 0) {
        digitalWrite(buzzerPin, LOW);
    }
}

bool buzzer_isBusy() {
    return active;
}
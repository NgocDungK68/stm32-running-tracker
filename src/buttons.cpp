#include "buttons.h"
#include "config.h"

#include <Arduino.h>

struct ButtonState {
    int pin;

    bool stablePressed;
    bool lastReadingPressed;

    bool shortPressEvent;
    bool longPressEvent;
    bool longPressFired;

    unsigned long pressedTime;
    unsigned long lastDebounceTime;
};

static ButtonState b1;
static ButtonState b2;

static void button_init_one(ButtonState &button, int pin) {
    button.pin = pin;

    button.stablePressed = false;
    button.lastReadingPressed = false;

    button.shortPressEvent = false;
    button.longPressEvent = false;
    button.longPressFired = false;

    button.pressedTime = 0;
    button.lastDebounceTime = 0;

    pinMode(button.pin, INPUT_PULLUP);
}

static void button_update_one(ButtonState &button) {
    unsigned long now = millis();

    // Vì dùng INPUT_PULLUP nên nhấn nút = LOW
    bool readingPressed = (digitalRead(button.pin) == LOW);

    // Nếu trạng thái đọc được thay đổi, bắt đầu đếm debounce
    if (readingPressed != button.lastReadingPressed) {
        button.lastDebounceTime = now;
        button.lastReadingPressed = readingPressed;
    }

    // Sau khoảng debounce, nếu trạng thái ổn định thay đổi thì cập nhật
    if ((now - button.lastDebounceTime) >= BUTTON_DEBOUNCE_MS) {
        if (readingPressed != button.stablePressed) {
            button.stablePressed = readingPressed;

            if (button.stablePressed) {
                // Vừa nhấn xuống
                button.pressedTime = now;
                button.longPressFired = false;
            } else {
                // Vừa thả ra
                if (!button.longPressFired) {
                    button.shortPressEvent = true;
                }
            }
        }
    }

    // Nếu giữ đủ lâu thì tạo sự kiện long press
    if (button.stablePressed && !button.longPressFired) {
        if ((now - button.pressedTime) >= BUTTON_LONG_PRESS_MS) {
            button.longPressEvent = true;
            button.longPressFired = true;
        }
    }
}

static bool consume_short_press(ButtonState &button) {
    if (button.shortPressEvent) {
        button.shortPressEvent = false;
        return true;
    }

    return false;
}

static bool consume_long_press(ButtonState &button) {
    if (button.longPressEvent) {
        button.longPressEvent = false;
        return true;
    }

    return false;
}

void buttons_init() {
    button_init_one(b1, BUTTON_B1_PIN);
    button_init_one(b2, BUTTON_B2_PIN);
}

void buttons_update() {
    button_update_one(b1);
    button_update_one(b2);
}

bool button_b1_pressed() {
    return consume_short_press(b1);
}

bool button_b2_pressed() {
    return consume_short_press(b2);
}

bool button_b1_long_pressed() {
    return consume_long_press(b1);
}

bool button_b2_long_pressed() {
    return consume_long_press(b2);
}
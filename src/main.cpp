#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "buttons.h"
#include "simulator.h"

enum AppScreen {
    SCREEN_HOME,
    SCREEN_RUN_READY,
    SCREEN_RUN_ACTIVE,
    SCREEN_HISTORY
};

static AppScreen currentScreen = SCREEN_HOME;

static unsigned char selectedHomeOption = 0;

static unsigned long lastRunUpdateTime = 0;

static RunData currentRunData = {
    0.0f,   // distance
    0,      // seconds
    0.0f    // pace
};

static DisplayData make_display_data_from_run(const RunData &runData) {
    DisplayData data;

    data.distance = runData.distance;
    data.seconds = runData.seconds;
    data.pace = runData.pace;
    data.gps_status = GPS_SIM;

    return data;
}

static void go_home() {
    currentScreen = SCREEN_HOME;
    selectedHomeOption = 0;
    display_show_home(selectedHomeOption);
}

static void enter_run_ready() {
    currentScreen = SCREEN_RUN_READY;

    currentRunData.distance = 0.0f;
    currentRunData.seconds = 0;
    currentRunData.pace = 0.0f;

    display_show_run_ready();
}

static void start_run() {
    simulator_init();

    currentRunData.distance = 0.0f;
    currentRunData.seconds = 0;
    currentRunData.pace = 0.0f;

    lastRunUpdateTime = millis();

    currentScreen = SCREEN_RUN_ACTIVE;

    DisplayData screenData = make_display_data_from_run(currentRunData);
    display_show_run_data(screenData);
}

static void enter_history() {
    currentScreen = SCREEN_HISTORY;
    display_show_history_placeholder();
}

static void handle_home_screen() {
    if (button_b2_pressed()) {
        selectedHomeOption++;

        if (selectedHomeOption > 1) {
            selectedHomeOption = 0;
        }

        display_show_home(selectedHomeOption);
    }

    if (button_b1_pressed()) {
        if (selectedHomeOption == 0) {
            enter_run_ready();
        } else {
            enter_history();
        }
    }
}

static void handle_run_ready_screen() {
    if (button_b1_pressed()) {
        start_run();
    }
}

static void handle_run_active_screen() {
    unsigned long now = millis();

    if (now - lastRunUpdateTime >= DISPLAY_UPDATE_INTERVAL_MS) {
        lastRunUpdateTime = now;

        currentRunData = simulator_update();

        DisplayData screenData = make_display_data_from_run(currentRunData);
        display_show_run_data(screenData);
    }
}

static void handle_history_screen() {
    // Hiện tại chưa xử lý history.
    // Sau này có thể dùng B1/B2 để chọn từng buổi chạy.
}

void setup() {
    display_init();
    buttons_init();

    go_home();
}

void loop() {
    buttons_update();

    // Giữ B2 khoảng 1 giây để quay về HOME từ các màn con
    if (button_b2_long_pressed()) {
        if (currentScreen != SCREEN_HOME) {
            go_home();
        }
    }

    switch (currentScreen) {
        case SCREEN_HOME:
            handle_home_screen();
            break;

        case SCREEN_RUN_READY:
            handle_run_ready_screen();
            break;

        case SCREEN_RUN_ACTIVE:
            handle_run_active_screen();
            break;

        case SCREEN_HISTORY:
            handle_history_screen();
            break;
    }
}
#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "buttons.h"
#include "simulator.h"
#include "gps.h"
#include "run_tracker.h"
#include "sd_history.h"

enum AppScreen {
    SCREEN_HOME,
    SCREEN_RUN_READY,
    SCREEN_RUN_ACTIVE,
    SCREEN_HISTORY_LIST,
    SCREEN_HISTORY_DETAIL
};

static AppScreen currentScreen = SCREEN_HOME;

static unsigned char selectedHomeOption = 0;

static unsigned long lastRunUpdateTime = 0;

static RunData currentRunData = {
    0.0f,   // distance
    0,      // seconds
    0.0f    // pace
};

static const uint8_t HISTORY_VISIBLE_COUNT = 4;
static const uint8_t HISTORY_DETAIL_VISIBLE_LINES = 4;
static const uint8_t HISTORY_DETAIL_LINE_COUNT = 13;

static bool historyReady = false;
static uint8_t historyCount = 0;
static uint8_t selectedHistoryIndex = 0;
static uint8_t historyTopIndex = 0;
static uint8_t historyDetailTopLine = 0;

static DisplayData make_display_data_from_run(
    const RunData &runData,
    GpsStatus gpsStatus
) {
    DisplayData data;

    data.distance = runData.distance;
    data.seconds = runData.seconds;
    data.pace = runData.pace;
    data.gps_status = gpsStatus;

    return data;
}

static void refresh_history_list_screen();

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
#if USE_SIMULATOR
    simulator_init();
#else
    run_tracker_init();
#endif

    currentRunData.distance = 0.0f;
    currentRunData.seconds = 0;
    currentRunData.pace = 0.0f;

    lastRunUpdateTime = millis();

    currentScreen = SCREEN_RUN_ACTIVE;

#if USE_SIMULATOR
    DisplayData screenData = make_display_data_from_run(
        currentRunData,
        GPS_SIM
    );
#else
    DisplayData screenData = make_display_data_from_run(
        currentRunData,
        gps_getData().status
    );
#endif

    display_show_run_data(screenData);
}

static void enter_history_list() {
    currentScreen = SCREEN_HISTORY_LIST;

    historyReady = sd_history_init();

    if (!historyReady) {
        historyCount = 0;
        selectedHistoryIndex = 0;
        historyTopIndex = 0;

        display_show_history_unavailable();
        return;
    }

    historyCount = sd_history_getCount();

    // Mỗi lần vào history từ HOME thì luôn chọn bản ghi mới nhất.
    // Quy ước trong sd_history: index 0 = buổi chạy mới nhất.
    selectedHistoryIndex = 0;
    historyTopIndex = 0;

    refresh_history_list_screen();
}

static void enter_history_detail() {
    if (!historyReady || historyCount == 0) {
        return;
    }

    currentScreen = SCREEN_HISTORY_DETAIL;
    historyDetailTopLine = 0;

    RunHistoryRecord record;

    if (sd_history_getRecord(selectedHistoryIndex, record)) {
        display_show_history_detail(record, historyDetailTopLine);
    }
}

static void refresh_history_list_screen() {
    if (!historyReady) {
        display_show_history_unavailable();
        return;
    }

    if (historyCount == 0) {
        display_show_history_empty();
        return;
    }

    RunHistoryRecord visibleRecords[HISTORY_VISIBLE_COUNT];

    uint8_t visibleCount = 0;

    for (uint8_t i = 0; i < HISTORY_VISIBLE_COUNT; i++) {
        uint8_t recordIndex = historyTopIndex + i;

        if (recordIndex >= historyCount) {
            break;
        }

        if (sd_history_getRecord(recordIndex, visibleRecords[i])) {
            visibleCount++;
        }
    }

    uint8_t selectedVisibleIndex = 0;

    if (selectedHistoryIndex >= historyTopIndex) {
        selectedVisibleIndex = selectedHistoryIndex - historyTopIndex;
    }

    display_show_history_list(
        visibleRecords,
        visibleCount,
        selectedVisibleIndex,
        historyCount,
        historyTopIndex
    );
}

static void refresh_history_detail_screen() {
    RunHistoryRecord record;

    if (sd_history_getRecord(selectedHistoryIndex, record)) {
        display_show_history_detail(record, historyDetailTopLine);
    }
}

static void adjust_history_top_index() {
    if (historyCount <= HISTORY_VISIBLE_COUNT) {
        historyTopIndex = 0;
        return;
    }

    if (selectedHistoryIndex < historyTopIndex) {
        historyTopIndex = selectedHistoryIndex;
    }

    if (selectedHistoryIndex >= historyTopIndex + HISTORY_VISIBLE_COUNT) {
        historyTopIndex = selectedHistoryIndex - HISTORY_VISIBLE_COUNT + 1;
    }

    uint8_t maxTopIndex = historyCount - HISTORY_VISIBLE_COUNT;

    if (historyTopIndex > maxTopIndex) {
        historyTopIndex = maxTopIndex;
    }
}

static void history_move_down() {
    if (!historyReady || historyCount == 0) {
        return;
    }

    if (selectedHistoryIndex + 1 < historyCount) {
        selectedHistoryIndex++;
    } else {
        selectedHistoryIndex = 0;
    }

    adjust_history_top_index();
    refresh_history_list_screen();
}

static void history_move_up() {
    if (!historyReady || historyCount == 0) {
        return;
    }

    if (selectedHistoryIndex > 0) {
        selectedHistoryIndex--;
    } else {
        selectedHistoryIndex = historyCount - 1;
    }

    adjust_history_top_index();
    refresh_history_list_screen();
}

static void detail_scroll_down() {
    uint8_t maxTopLine = 0;

    if (HISTORY_DETAIL_LINE_COUNT > HISTORY_DETAIL_VISIBLE_LINES) {
        maxTopLine = HISTORY_DETAIL_LINE_COUNT - HISTORY_DETAIL_VISIBLE_LINES;
    }

    if (historyDetailTopLine < maxTopLine) {
        historyDetailTopLine++;
        refresh_history_detail_screen();
    }
}

static void detail_scroll_up() {
    if (historyDetailTopLine > 0) {
        historyDetailTopLine--;
        refresh_history_detail_screen();
    }
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
            enter_history_list();
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

#if USE_SIMULATOR
        currentRunData = simulator_update();

        DisplayData screenData = make_display_data_from_run(
            currentRunData,
            GPS_SIM
        );
#else
        GPSData gpsData = gps_getData();

        currentRunData = run_tracker_update(gpsData);

        DisplayData screenData = make_display_data_from_run(
            currentRunData,
            gpsData.status
        );
#endif

        display_show_run_data(screenData);
    }
}

static void handle_history_list_screen() {
    if (!historyReady || historyCount == 0) {
        return;
    }

    if (button_b2_pressed()) {
        history_move_down();
    }

    if (button_b1_long_pressed()) {
        history_move_up();
    }

    if (button_b1_pressed()) {
        enter_history_detail();
    }
}

static void handle_history_detail_screen() {
    if (button_b2_pressed()) {
        detail_scroll_down();
    }

    if (button_b1_long_pressed()) {
        detail_scroll_up();
    }
}

void setup() {
#if GPS_DEBUG_NMEA
    Serial.begin(115200);
    delay(1000);
#endif

#if !USE_SIMULATOR
    gps_init();
#endif

    display_init();
    buttons_init();

    go_home();
}

void loop() {
#if !USE_SIMULATOR
    gps_update();
#endif

    buttons_update();

    if (button_b2_long_pressed()) {
        if (currentScreen == SCREEN_HISTORY_DETAIL) {
            currentScreen = SCREEN_HISTORY_LIST;
            refresh_history_list_screen();
            return;
        }

        if (currentScreen != SCREEN_HOME) {
            go_home();
            return;
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

        case SCREEN_HISTORY_LIST:
            handle_history_list_screen();
            break;

        case SCREEN_HISTORY_DETAIL:
            handle_history_detail_screen();
            break;
    }
}
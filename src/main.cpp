#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "buttons.h"
#include "simulator.h"
#include "gps.h"
#include "run_tracker.h"
#include "sd_history.h"
#include "buzzer.h"

enum AppScreen {
    SCREEN_HOME,
    SCREEN_RUN_READY,
    SCREEN_RUN_ACTIVE,
    SCREEN_RUN_STOP_CONFIRM,
    SCREEN_HISTORY_LIST,
    SCREEN_HISTORY_DETAIL,
    SCREEN_HISTORY_DELETE_CONFIRM
};

static AppScreen currentScreen = SCREEN_HOME;

static unsigned char selectedHomeOption = 0;

static unsigned long lastRunUpdateTime = 0;
static unsigned long runStartMillis = 0;

static bool runPaused = false;
static bool runWasPausedBeforeStopConfirm = false;

static uint16_t simulatorPauseCount = 0;

static char currentRunDate[11] = "0000-00-00";
static char currentRunStartTime[9] = "00:00:00";

static RunData currentRunData = {
    0.0f,
    0,
    0.0f
};

static float nextDistanceAlertM = BUZZER_DISTANCE_STEP_M;

static const uint8_t HISTORY_VISIBLE_COUNT = 4;
static const uint8_t HISTORY_DETAIL_VISIBLE_LINES = 4;
static const uint8_t HISTORY_DETAIL_LINE_COUNT = 13;

static bool historyReady = false;
static uint8_t historyCount = 0;
static uint8_t selectedHistoryIndex = 0;
static uint8_t historyTopIndex = 0;
static uint8_t historyDetailTopLine = 0;

static int compile_month_to_number(const char *monthText) {
    if (strncmp(monthText, "Jan", 3) == 0) return 1;
    if (strncmp(monthText, "Feb", 3) == 0) return 2;
    if (strncmp(monthText, "Mar", 3) == 0) return 3;
    if (strncmp(monthText, "Apr", 3) == 0) return 4;
    if (strncmp(monthText, "May", 3) == 0) return 5;
    if (strncmp(monthText, "Jun", 3) == 0) return 6;
    if (strncmp(monthText, "Jul", 3) == 0) return 7;
    if (strncmp(monthText, "Aug", 3) == 0) return 8;
    if (strncmp(monthText, "Sep", 3) == 0) return 9;
    if (strncmp(monthText, "Oct", 3) == 0) return 10;
    if (strncmp(monthText, "Nov", 3) == 0) return 11;
    if (strncmp(monthText, "Dec", 3) == 0) return 12;

    return 1;
}

static void set_compile_datetime() {
    char monthText[4];
    int day;
    int year;

    sscanf(__DATE__, "%3s %d %d", monthText, &day, &year);

    int month = compile_month_to_number(monthText);

    snprintf(
        currentRunDate,
        sizeof(currentRunDate),
        "%04d-%02d-%02d",
        year,
        month,
        day
    );

    snprintf(
        currentRunStartTime,
        sizeof(currentRunStartTime),
        "%s",
        __TIME__
    );
}

static void capture_run_start_datetime() {
#if USE_SIMULATOR
    set_compile_datetime();
#else
    GPSData gpsData = gps_getData();

    if (gpsData.hasDateTime) {
        snprintf(
            currentRunDate,
            sizeof(currentRunDate),
            "%04d-%02d-%02d",
            gpsData.year,
            gpsData.month,
            gpsData.day
        );

        snprintf(
            currentRunStartTime,
            sizeof(currentRunStartTime),
            "%02d:%02d:%02d",
            gpsData.hour,
            gpsData.minute,
            gpsData.second
        );
    } else {
        set_compile_datetime();
    }
#endif
}

static DisplayData make_display_data_from_run(
    const RunData &runData,
    GpsStatus gpsStatus
) {
    DisplayData data;

    data.distance = runData.distance;
    data.seconds = runData.seconds;
    data.pace = runData.pace;
    data.gps_status = gpsStatus;

    data.user_paused = runPaused;

#if USE_SIMULATOR
    data.auto_paused = false;
#else
    data.auto_paused = run_tracker_isAutoPaused();
#endif

    return data;
}

static GpsStatus get_current_gps_status_for_screen() {
#if USE_SIMULATOR
    return GPS_SIM;
#else
    return gps_getData().status;
#endif
}

static void refresh_history_list_screen();
static void refresh_history_detail_screen();

static void wait_with_buzzer(unsigned long waitMs) {
    unsigned long startTime = millis();

    while (millis() - startTime < waitMs) {
        buzzer_update();
        delay(1);
    }
}

static void reset_distance_alert() {
    nextDistanceAlertM = BUZZER_DISTANCE_STEP_M;
}

static void check_distance_alert() {
    while (currentRunData.distance >= nextDistanceAlertM) {
        buzzer_beep_kilometer();
        nextDistanceAlertM += BUZZER_DISTANCE_STEP_M;
    }
}

static void show_current_run_screen() {
    DisplayData screenData = make_display_data_from_run(
        currentRunData,
        get_current_gps_status_for_screen()
    );

    display_show_run_data(screenData);
}

static void go_home() {
    currentScreen = SCREEN_HOME;
    selectedHomeOption = 0;
    display_show_home(selectedHomeOption);
}

static void enter_run_ready() {
    currentScreen = SCREEN_RUN_READY;

    runPaused = false;

    currentRunData.distance = 0.0f;
    currentRunData.seconds = 0;
    currentRunData.pace = 0.0f;

    reset_distance_alert();

    display_show_run_ready();
}

static void start_run() {
    capture_run_start_datetime();

#if USE_SIMULATOR
    simulator_init();
    simulatorPauseCount = 0;
#else
    run_tracker_init();
#endif

    runStartMillis = millis();
    runPaused = false;

    currentRunData.distance = 0.0f;
    currentRunData.seconds = 0;
    currentRunData.pace = 0.0f;

    reset_distance_alert();

    lastRunUpdateTime = millis();

    currentScreen = SCREEN_RUN_ACTIVE;

    buzzer_beep_once();

    show_current_run_screen();
}

static void toggle_run_pause() {
    if (!runPaused) {
        runPaused = true;

#if USE_SIMULATOR
        simulatorPauseCount++;
#else
        run_tracker_pause(true);
#endif

        buzzer_beep_once();
    } else {
        runPaused = false;

#if !USE_SIMULATOR
        run_tracker_resume();
#endif

        lastRunUpdateTime = millis();

        buzzer_beep_once();
    }

    show_current_run_screen();
}

static void enter_run_stop_confirm() {
    currentScreen = SCREEN_RUN_STOP_CONFIRM;

    runWasPausedBeforeStopConfirm = runPaused;

    if (!runPaused) {
        runPaused = true;

#if !USE_SIMULATOR
        run_tracker_pause(false);
#endif
    }

    DisplayData screenData = make_display_data_from_run(
        currentRunData,
        get_current_gps_status_for_screen()
    );

    display_show_run_stop_confirm(screenData);
}

static void cancel_run_stop_confirm() {
    currentScreen = SCREEN_RUN_ACTIVE;

    if (!runWasPausedBeforeStopConfirm) {
        runPaused = false;

#if !USE_SIMULATOR
        run_tracker_resume();
#endif

        lastRunUpdateTime = millis();
    }

    show_current_run_screen();
}

static void discard_current_run() {
#if !USE_SIMULATOR
    run_tracker_discard();
#endif

    runPaused = false;

    enter_run_ready();
}

static bool make_simulator_history_record(RunHistoryRecord &record) {
    record.date = currentRunDate;
    record.start_time = currentRunStartTime;

    record.distance_m = currentRunData.distance;

    record.moving_time_s = currentRunData.seconds;
    record.elapsed_time_s = (millis() - runStartMillis) / 1000UL;

    record.pause_count = simulatorPauseCount;

    record.max_speed_kmph = 0.0f;

    record.start_lat = 0.0;
    record.start_lon = 0.0;

    record.end_lat = 0.0;
    record.end_lon = 0.0;

    record.note = "SIM";

    return true;
}

static void reset_current_run_data() {
    runPaused = false;

    currentRunData.distance = 0.0f;
    currentRunData.seconds = 0;
    currentRunData.pace = 0.0f;

    simulatorPauseCount = 0;
    runStartMillis = 0;
    lastRunUpdateTime = 0;

    reset_distance_alert();
}

static void stop_and_save_run() {
    RunHistoryRecord record;
    bool recordOk = false;

#if USE_SIMULATOR
    recordOk = make_simulator_history_record(record);
#else
    recordOk = run_tracker_finish(
        record,
        currentRunDate,
        currentRunStartTime,
        "GPS"
    );
#endif

    bool saveOk = false;

    if (recordOk) {
        saveOk = sd_history_saveRun(record);
    }

    buzzer_beep_once();

    display_show_save_result(saveOk);
    wait_with_buzzer(1200);

#if !USE_SIMULATOR
    run_tracker_discard();
#endif

    reset_current_run_data();

    go_home();
}

static void enter_history_list() {
    currentScreen = SCREEN_HISTORY_LIST;

#if SD_HISTORY_DEBUG
    Serial.println();
    Serial.println("[APP] enter history");
#endif

    historyReady = sd_history_init();

#if SD_HISTORY_DEBUG
    Serial.print("[APP] historyReady=");
    Serial.println(historyReady ? "true" : "false");
#endif

    if (!historyReady) {
        historyCount = 0;
        selectedHistoryIndex = 0;
        historyTopIndex = 0;

        display_show_history_unavailable();
        return;
    }

    historyCount = sd_history_getCount();

#if SD_HISTORY_DEBUG
    Serial.print("[APP] historyCount=");
    Serial.println(historyCount);
#endif

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

    refresh_history_detail_screen();
}

static void enter_history_delete_confirm() {
    if (!historyReady || historyCount == 0) {
        return;
    }

    RunHistoryRecord record;

    if (sd_history_getRecord(selectedHistoryIndex, record)) {
        currentScreen = SCREEN_HISTORY_DELETE_CONFIRM;
        display_show_history_delete_confirm(record);
    }
}

static void cancel_history_delete_confirm() {
    currentScreen = SCREEN_HISTORY_DETAIL;
    refresh_history_detail_screen();
}

static void delete_selected_history() {
    bool ok = sd_history_deleteRecord(selectedHistoryIndex);

    display_show_delete_result(ok);
    wait_with_buzzer(1000);

    if (!ok) {
        currentScreen = SCREEN_HISTORY_DETAIL;
        refresh_history_detail_screen();
        return;
    }

    historyCount = sd_history_getCount();

    if (historyCount == 0) {
        selectedHistoryIndex = 0;
        historyTopIndex = 0;
        currentScreen = SCREEN_HISTORY_LIST;
        refresh_history_list_screen();
        return;
    }

    if (selectedHistoryIndex >= historyCount) {
        selectedHistoryIndex = historyCount - 1;
    }

    currentScreen = SCREEN_HISTORY_LIST;
    refresh_history_list_screen();
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
    } else {
        historyDetailTopLine = 0;
    }

    refresh_history_detail_screen();
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
    if (button_b1_long_pressed()) {
        enter_run_stop_confirm();
        return;
    }

    if (button_b1_pressed()) {
        toggle_run_pause();
        return;
    }

    if (runPaused) {
        return;
    }

    unsigned long now = millis();

    if (now - lastRunUpdateTime >= DISPLAY_UPDATE_INTERVAL_MS) {
        lastRunUpdateTime = now;

#if USE_SIMULATOR
        currentRunData = simulator_update();

        check_distance_alert();
#else
        bool wasAutoPaused = run_tracker_isAutoPaused();

        GPSData gpsData = gps_getData();
        currentRunData = run_tracker_update(gpsData);

        bool isAutoPaused = run_tracker_isAutoPaused();

        if (!wasAutoPaused && isAutoPaused) {
            buzzer_beep_once();
        }

        if (wasAutoPaused && !isAutoPaused) {
            buzzer_beep_once();
        }

        if (!run_tracker_isPaused()) {
            check_distance_alert();
        }
#endif

        show_current_run_screen();
    }
}

static void handle_run_stop_confirm_screen() {
    if (button_b1_long_pressed()) {
        stop_and_save_run();
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
    if (button_b1_long_pressed()) {
        enter_history_delete_confirm();
        return;
    }

    if (button_b2_pressed()) {
        detail_scroll_down();
    }
}

static void handle_history_delete_confirm_screen() {
    if (button_b1_long_pressed()) {
        delete_selected_history();
    }
}

void setup() {
#if GPS_DEBUG_NMEA || SD_HISTORY_DEBUG
    Serial.begin(115200);
    delay(1000);

#if SD_HISTORY_DEBUG
    Serial.println();
    Serial.println("BOOT");
    Serial.println("Serial OK");
#endif
#endif

#if !USE_SIMULATOR
    gps_init();
#endif

    display_init();
    buttons_init();
    buzzer_init(BUZZER_PIN);

    go_home();
}

void loop() {
#if !USE_SIMULATOR
    gps_update();
#endif

    buttons_update();
    buzzer_update();

    if (button_b2_long_pressed()) {
        if (currentScreen == SCREEN_RUN_ACTIVE) {
            discard_current_run();
            return;
        }

        if (currentScreen == SCREEN_RUN_STOP_CONFIRM) {
            cancel_run_stop_confirm();
            return;
        }

        if (currentScreen == SCREEN_HISTORY_DELETE_CONFIRM) {
            cancel_history_delete_confirm();
            return;
        }

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

        case SCREEN_RUN_STOP_CONFIRM:
            handle_run_stop_confirm_screen();
            break;

        case SCREEN_HISTORY_LIST:
            handle_history_list_screen();
            break;

        case SCREEN_HISTORY_DETAIL:
            handle_history_detail_screen();
            break;

        case SCREEN_HISTORY_DELETE_CONFIRM:
            handle_history_delete_confirm_screen();
            break;
    }
}
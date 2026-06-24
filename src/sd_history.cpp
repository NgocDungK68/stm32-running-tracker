#include "sd_history.h"
#include "config.h"

#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>

static SdFat sd;
static bool sdReady = false;

static void debugPrintln(const char* message) {
#if SD_HISTORY_DEBUG
    Serial.println(message);
#endif
}

static void debugPrint(const char* message) {
#if SD_HISTORY_DEBUG
    Serial.print(message);
#endif
}

static const char* safeText(const char* text) {
    if (text == nullptr) {
        return "";
    }

    return text;
}

static uint32_t getPausedTime(uint32_t movingTimeS, uint32_t elapsedTimeS) {
    if (elapsedTimeS > movingTimeS) {
        return elapsedTimeS - movingTimeS;
    }

    return 0;
}

static float getDistanceKm(float distanceM) {
    if (distanceM <= 0.0f) {
        return 0.0f;
    }

    return distanceM / 1000.0f;
}

static float getAvgPaceSecPerKm(float distanceM, uint32_t movingTimeS) {
    float distanceKm = getDistanceKm(distanceM);

    if (distanceKm <= 0.0f || movingTimeS == 0) {
        return 0.0f;
    }

    return movingTimeS / distanceKm;
}

static float getAvgSpeedKmph(float distanceM, uint32_t movingTimeS) {
    float distanceKm = getDistanceKm(distanceM);

    if (distanceKm <= 0.0f || movingTimeS == 0) {
        return 0.0f;
    }

    float movingTimeHour = movingTimeS / 3600.0f;

    return distanceKm / movingTimeHour;
}

static bool createHistoryFileIfNeeded() {
    if (!sdReady) {
        return false;
    }

    if (sd.exists(SD_HISTORY_FILE)) {
        return true;
    }

    File file = sd.open(SD_HISTORY_FILE, FILE_WRITE);

    if (!file) {
        debugPrintln("SD: create RUNS.CSV FAILED");
        return false;
    }

    file.println(
        "run_id,"
        "date,"
        "start_time,"
        "distance_m,"
        "distance_km,"
        "moving_time_s,"
        "elapsed_time_s,"
        "paused_time_s,"
        "pause_count,"
        "avg_pace_s_per_km,"
        "avg_speed_kmph,"
        "max_speed_kmph,"
        "start_lat,"
        "start_lon,"
        "end_lat,"
        "end_lon,"
        "note"
    );

    file.close();

    debugPrintln("SD: create RUNS.CSV OK");
    return true;
}

static uint32_t countSavedRuns() {
    if (!sdReady) {
        return 0;
    }

    File file = sd.open(SD_HISTORY_FILE);

    if (!file) {
        return 0;
    }

    uint32_t lineCount = 0;

    while (file.available()) {
        char c = file.read();

        if (c == '\n') {
            lineCount++;
        }
    }

    file.close();

    // line 1 là header, nên số buổi chạy = lineCount - 1
    if (lineCount == 0) {
        return 0;
    }

    return lineCount - 1;
}

bool sd_history_init() {
    sdReady = false;

    SPI.begin();

    if (!sd.begin(SD_CS_PIN, SD_SCK_MHZ(SD_SPI_SPEED_MHZ))) {
        debugPrintln("SD: init FAILED");
        return false;
    }

    sdReady = true;

    debugPrintln("SD: init OK");

    if (!createHistoryFileIfNeeded()) {
        return false;
    }

    return true;
}

bool sd_history_isReady() {
    return sdReady;
}

bool sd_history_saveRun(const RunHistoryRecord &record) {
    if (!sdReady) {
        debugPrintln("SD: not ready");
        return false;
    }

    if (!createHistoryFileIfNeeded()) {
        return false;
    }

    File file = sd.open(SD_HISTORY_FILE, FILE_WRITE);

    if (!file) {
        debugPrintln("SD: open RUNS.CSV FAILED");
        return false;
    }

    uint32_t runId = countSavedRuns() + 1;

    float distanceKm = getDistanceKm(record.distance_m);
    uint32_t pausedTimeS = getPausedTime(
        record.moving_time_s,
        record.elapsed_time_s
    );

    float avgPaceSecPerKm = getAvgPaceSecPerKm(
        record.distance_m,
        record.moving_time_s
    );

    float avgSpeedKmph = getAvgSpeedKmph(
        record.distance_m,
        record.moving_time_s
    );

    file.print(runId);
    file.print(",");

    file.print(safeText(record.date));
    file.print(",");

    file.print(safeText(record.start_time));
    file.print(",");

    file.print(record.distance_m, 2);
    file.print(",");

    file.print(distanceKm, 3);
    file.print(",");

    file.print(record.moving_time_s);
    file.print(",");

    file.print(record.elapsed_time_s);
    file.print(",");

    file.print(pausedTimeS);
    file.print(",");

    file.print(record.pause_count);
    file.print(",");

    file.print(avgPaceSecPerKm, 2);
    file.print(",");

    file.print(avgSpeedKmph, 2);
    file.print(",");

    file.print(record.max_speed_kmph, 2);
    file.print(",");

    file.print(record.start_lat, 6);
    file.print(",");

    file.print(record.start_lon, 6);
    file.print(",");

    file.print(record.end_lat, 6);
    file.print(",");

    file.print(record.end_lon, 6);
    file.print(",");

    file.println(safeText(record.note));

    file.close();

    debugPrintln("SD: save run OK");
    return true;
}

bool sd_history_writeTest() {
    if (!sdReady) {
        debugPrintln("SD: not ready");
        return false;
    }

    File file = sd.open(SD_TEST_FILE, FILE_WRITE);

    if (!file) {
        debugPrintln("SD: open TEST.TXT for write FAILED");
        return false;
    }

    file.println("HELLO STM32");
    file.close();

    debugPrintln("SD: write TEST.TXT OK");
    return true;
}

bool sd_history_readTest() {
    if (!sdReady) {
        debugPrintln("SD: not ready");
        return false;
    }

    File file = sd.open(SD_TEST_FILE);

    if (!file) {
        debugPrintln("SD: open TEST.TXT for read FAILED");
        return false;
    }

#if SD_HISTORY_DEBUG
    Serial.println("SD: Reading TEST.TXT:");

    while (file.available()) {
        Serial.write(file.read());
    }

    Serial.println();
#endif

    file.close();

    debugPrintln("SD: read TEST.TXT OK");
    return true;
}

bool sd_history_printAll(Stream &out) {
    if (!sdReady) {
        return false;
    }

    File file = sd.open(SD_HISTORY_FILE);

    if (!file) {
        return false;
    }

    while (file.available()) {
        out.write(file.read());
    }

    file.close();

    return true;
}

bool sd_history_clear() {
    if (!sdReady) {
        return false;
    }

    if (sd.exists(SD_HISTORY_FILE)) {
        sd.remove(SD_HISTORY_FILE);
    }

    return createHistoryFileIfNeeded();
}
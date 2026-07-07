#include "sd_history.h"
#include "config.h"

#include <Arduino.h>
#include <SD.h>
#include <string.h>
#include <stdlib.h>

#ifndef SD_HISTORY_USE_SAMPLE_DATA
#define SD_HISTORY_USE_SAMPLE_DATA 1
#endif

struct StoredRunHistoryRecord {
    char date[11];
    char start_time[9];

    float distance_m;

    uint32_t moving_time_s;
    uint32_t elapsed_time_s;

    uint16_t pause_count;

    float max_speed_kmph;

    double start_lat;
    double start_lon;

    double end_lat;
    double end_lon;

    char note[16];
};

static StoredRunHistoryRecord records[SD_HISTORY_MAX_RECORDS];
static uint8_t recordCount = 0;
static bool historyReady = false;

static void copy_text(char *dst, size_t dstSize, const char *src) {
    if (dstSize == 0) {
        return;
    }

    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

static void clear_records() {
    recordCount = 0;
}

static void stored_to_public(
    const StoredRunHistoryRecord &stored,
    RunHistoryRecord &record
) {
    record.date = stored.date;
    record.start_time = stored.start_time;

    record.distance_m = stored.distance_m;

    record.moving_time_s = stored.moving_time_s;
    record.elapsed_time_s = stored.elapsed_time_s;

    record.pause_count = stored.pause_count;

    record.max_speed_kmph = stored.max_speed_kmph;

    record.start_lat = stored.start_lat;
    record.start_lon = stored.start_lon;

    record.end_lat = stored.end_lat;
    record.end_lon = stored.end_lon;

    record.note = stored.note;
}

static void insert_record_at_front(const StoredRunHistoryRecord &newRecord) {
    if (recordCount < SD_HISTORY_MAX_RECORDS) {
        recordCount++;
    }

    for (int i = recordCount - 1; i > 0; i--) {
        records[i] = records[i - 1];
    }

    records[0] = newRecord;
}

static void add_sample_record(
    const char *date,
    const char *start_time,
    float distance_m,
    uint32_t moving_time_s,
    uint32_t elapsed_time_s,
    uint16_t pause_count,
    float max_speed_kmph,
    double start_lat,
    double start_lon,
    double end_lat,
    double end_lon,
    const char *note
) {
    StoredRunHistoryRecord record;

    copy_text(record.date, sizeof(record.date), date);
    copy_text(record.start_time, sizeof(record.start_time), start_time);

    record.distance_m = distance_m;

    record.moving_time_s = moving_time_s;
    record.elapsed_time_s = elapsed_time_s;

    record.pause_count = pause_count;

    record.max_speed_kmph = max_speed_kmph;

    record.start_lat = start_lat;
    record.start_lon = start_lon;

    record.end_lat = end_lat;
    record.end_lon = end_lon;

    copy_text(record.note, sizeof(record.note), note);

    if (recordCount < SD_HISTORY_MAX_RECORDS) {
        records[recordCount] = record;
        recordCount++;
    }
}

static void load_sample_data() {
    clear_records();

    // Mẫu mới nhất nằm trên cùng
    add_sample_record(
        "2026-07-07",
        "06:20:12",
        3210.0f,
        1263,
        1450,
        2,
        12.8f,
        21.005812,
        105.843145,
        21.010901,
        105.850212,
        "SIM"
    );

    add_sample_record(
        "2026-07-05",
        "17:42:30",
        5050.0f,
        1890,
        2015,
        1,
        14.2f,
        21.002100,
        105.840300,
        21.015400,
        105.852800,
        "GPS"
    );

    add_sample_record(
        "2026-07-02",
        "05:58:44",
        2210.0f,
        870,
        920,
        0,
        10.5f,
        21.004211,
        105.842019,
        21.008700,
        105.846231,
        "OUTDOOR"
    );

    add_sample_record(
        "2026-06-29",
        "18:10:05",
        10120.0f,
        3720,
        4050,
        3,
        15.1f,
        21.001112,
        105.839999,
        21.020213,
        105.861111,
        "GPS"
    );

    add_sample_record(
        "2026-06-25",
        "06:05:19",
        1520.0f,
        625,
        660,
        0,
        9.4f,
        21.003333,
        105.844444,
        21.006666,
        105.847777,
        "SIM"
    );
}

static bool parse_csv_line(char *line, StoredRunHistoryRecord &record) {
    const uint8_t FIELD_COUNT = 12;
    char *fields[FIELD_COUNT];

    uint8_t index = 0;
    char *token = strtok(line, ",");

    while (token != NULL && index < FIELD_COUNT) {
        fields[index] = token;
        index++;
        token = strtok(NULL, ",");
    }

    if (index < FIELD_COUNT) {
        return false;
    }

    copy_text(record.date, sizeof(record.date), fields[0]);
    copy_text(record.start_time, sizeof(record.start_time), fields[1]);

    record.distance_m = atof(fields[2]);

    record.moving_time_s = strtoul(fields[3], NULL, 10);
    record.elapsed_time_s = strtoul(fields[4], NULL, 10);

    record.pause_count = (uint16_t)strtoul(fields[5], NULL, 10);

    record.max_speed_kmph = atof(fields[6]);

    record.start_lat = atof(fields[7]);
    record.start_lon = atof(fields[8]);

    record.end_lat = atof(fields[9]);
    record.end_lon = atof(fields[10]);

    copy_text(record.note, sizeof(record.note), fields[11]);

    return true;
}

static void load_records_from_sd() {
    clear_records();

    File file = SD.open(SD_HISTORY_FILE, FILE_READ);

    if (!file) {
        return;
    }

    char line[160];
    uint8_t pos = 0;

    while (file.available()) {
        char c = file.read();

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            line[pos] = '\0';

            if (pos > 0) {
                StoredRunHistoryRecord record;

                if (parse_csv_line(line, record)) {
                    // Dòng mới hơn thường được ghi sau.
                    // Đưa bản ghi mới lên đầu list.
                    insert_record_at_front(record);
                }
            }

            pos = 0;
        } else {
            if (pos < sizeof(line) - 1) {
                line[pos] = c;
                pos++;
            }
        }
    }

    if (pos > 0) {
        line[pos] = '\0';

        StoredRunHistoryRecord record;

        if (parse_csv_line(line, record)) {
            insert_record_at_front(record);
        }
    }

    file.close();
}

bool sd_history_init() {
#if SD_HISTORY_USE_SAMPLE_DATA
    load_sample_data();
    historyReady = true;
    return true;
#else
    clear_records();

    historyReady = SD.begin(SD_CS_PIN);

    if (!historyReady) {
        return false;
    }

    load_records_from_sd();

    return true;
#endif
}

bool sd_history_isReady() {
    return historyReady;
}

uint8_t sd_history_getCount() {
    return recordCount;
}

bool sd_history_getRecord(uint8_t index, RunHistoryRecord &record) {
    if (!historyReady) {
        return false;
    }

    if (index >= recordCount) {
        return false;
    }

    stored_to_public(records[index], record);

    return true;
}

bool sd_history_saveRun(const RunHistoryRecord &record) {
#if SD_HISTORY_USE_SAMPLE_DATA
    StoredRunHistoryRecord stored;

    copy_text(stored.date, sizeof(stored.date), record.date);
    copy_text(stored.start_time, sizeof(stored.start_time), record.start_time);

    stored.distance_m = record.distance_m;

    stored.moving_time_s = record.moving_time_s;
    stored.elapsed_time_s = record.elapsed_time_s;

    stored.pause_count = record.pause_count;

    stored.max_speed_kmph = record.max_speed_kmph;

    stored.start_lat = record.start_lat;
    stored.start_lon = record.start_lon;

    stored.end_lat = record.end_lat;
    stored.end_lon = record.end_lon;

    copy_text(stored.note, sizeof(stored.note), record.note);

    insert_record_at_front(stored);

    return true;
#else
    if (!historyReady) {
        return false;
    }

    File file = SD.open(SD_HISTORY_FILE, FILE_WRITE);

    if (!file) {
        return false;
    }

    file.print(record.date);
    file.print(",");
    file.print(record.start_time);
    file.print(",");
    file.print(record.distance_m, 2);
    file.print(",");
    file.print(record.moving_time_s);
    file.print(",");
    file.print(record.elapsed_time_s);
    file.print(",");
    file.print(record.pause_count);
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
    file.println(record.note);

    file.close();

    sd_history_init();

    return true;
#endif
}

bool sd_history_writeTest() {
#if SD_HISTORY_USE_SAMPLE_DATA
    return true;
#else
    if (!historyReady) {
        return false;
    }

    File file = SD.open(SD_TEST_FILE, FILE_WRITE);

    if (!file) {
        return false;
    }

    file.println("STM32 SD TEST OK");
    file.close();

    return true;
#endif
}

bool sd_history_readTest() {
#if SD_HISTORY_USE_SAMPLE_DATA
    return true;
#else
    if (!historyReady) {
        return false;
    }

    File file = SD.open(SD_TEST_FILE, FILE_READ);

    if (!file) {
        return false;
    }

    file.close();

    return true;
#endif
}

bool sd_history_printAll(Stream &out) {
    if (!historyReady) {
        out.println("SD history not ready");
        return false;
    }

    for (uint8_t i = 0; i < recordCount; i++) {
        out.print(i);
        out.print(": ");

        out.print(records[i].date);
        out.print(" ");
        out.print(records[i].start_time);
        out.print(" ");

        out.print(records[i].distance_m / 1000.0f, 2);
        out.print(" km ");

        out.print(records[i].moving_time_s);
        out.print(" s ");

        out.print(records[i].note);
        out.println();
    }

    return true;
}

bool sd_history_clear() {
#if SD_HISTORY_USE_SAMPLE_DATA
    clear_records();
    return true;
#else
    if (!historyReady) {
        return false;
    }

    SD.remove(SD_HISTORY_FILE);
    clear_records();

    return true;
#endif
}
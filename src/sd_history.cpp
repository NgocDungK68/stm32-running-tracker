#include "sd_history.h"
#include "config.h"

#include <Arduino.h>
#include <SD.h>
#include <string.h>
#include <stdlib.h>

#if SD_HISTORY_DEBUG
#define SD_LOG(x) Serial.println(x)
#define SD_PRINT(x) Serial.print(x)
#else
#define SD_LOG(x)
#define SD_PRINT(x)
#endif

struct StoredRunHistoryRecord {
    char date[11];
    char start_time[9];

    float distance_m;

    uint32_t moving_time_s;
    uint32_t elapsed_time_s;

    uint16_t pause_count;

    float max_speed_kmph;

    char note[16];
};

static StoredRunHistoryRecord records[SD_HISTORY_MAX_RECORDS];
static uint8_t recordCount = 0;
static bool historyReady = false;

static void copy_text(char *dst, size_t dstSize, const char *src) {
    if (dstSize == 0) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
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

    record.note = stored.note;
}

static void public_to_stored(
    const RunHistoryRecord &record,
    StoredRunHistoryRecord &stored
) {
    copy_text(stored.date, sizeof(stored.date), record.date);
    copy_text(stored.start_time, sizeof(stored.start_time), record.start_time);

    stored.distance_m = record.distance_m;

    stored.moving_time_s = record.moving_time_s;
    stored.elapsed_time_s = record.elapsed_time_s;

    stored.pause_count = record.pause_count;

    stored.max_speed_kmph = record.max_speed_kmph;

    copy_text(stored.note, sizeof(stored.note), record.note);
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

static bool remove_record_from_ram(uint8_t index) {
    if (index >= recordCount) {
        return false;
    }

    for (uint8_t i = index; i + 1 < recordCount; i++) {
        records[i] = records[i + 1];
    }

    recordCount--;

    return true;
}

static bool parse_csv_line(char *line, StoredRunHistoryRecord &record) {
    const uint8_t FIELD_COUNT = 8;
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

    // Nếu dòng CSV cũ còn dư field tọa độ thì bỏ qua để tránh đọc sai format.
    if (token != NULL) {
        return false;
    }

    copy_text(record.date, sizeof(record.date), fields[0]);
    copy_text(record.start_time, sizeof(record.start_time), fields[1]);

    record.distance_m = atof(fields[2]);

    record.moving_time_s = strtoul(fields[3], NULL, 10);
    record.elapsed_time_s = strtoul(fields[4], NULL, 10);

    record.pause_count = (uint16_t)strtoul(fields[5], NULL, 10);

    record.max_speed_kmph = atof(fields[6]);

    copy_text(record.note, sizeof(record.note), fields[7]);

    return true;
}

static void write_record_line(File &file, const StoredRunHistoryRecord &record) {
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
    file.println(record.note);
}

static bool rewrite_all_records_to_sd() {
#if SD_HISTORY_USE_SAMPLE_DATA
    return true;
#else
    SD_LOG("[SD] rewrite");

    if (SD.exists(SD_HISTORY_FILE)) {
        SD.remove(SD_HISTORY_FILE);
    }

    if (recordCount == 0) {
        SD_LOG("empty");
        return true;
    }

    File file = SD.open(SD_HISTORY_FILE, FILE_WRITE);

    if (!file) {
        SD_LOG("rewrite FAIL");
        return false;
    }

    for (int i = recordCount - 1; i >= 0; i--) {
        write_record_line(file, records[i]);
    }

    file.close();

    SD_LOG("rewrite OK");
    return true;
#endif
}

static void load_records_from_sd() {
    clear_records();

    SD_LOG("[SD] load");

    if (!SD.exists(SD_HISTORY_FILE)) {
        SD_LOG("no RUNS");
        return;
    }

    File file = SD.open(SD_HISTORY_FILE, FILE_READ);

    if (!file) {
        SD_LOG("open RUNS FAIL");
        return;
    }

    char line[90];
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
                    insert_record_at_front(record);
                } else {
                    SD_LOG("csv BAD");
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
        } else {
            SD_LOG("csv BAD");
        }
    }

    file.close();

    SD_PRINT("rec=");
    SD_LOG(recordCount);
}

bool sd_history_init() {
#if SD_HISTORY_USE_SAMPLE_DATA
    load_sample_data();
    historyReady = true;
    return true;
#else
    clear_records();
    historyReady = false;

    SD_LOG("[SD] init");

    if (!SD.begin(SD_CS_PIN)) {
        SD_LOG("begin FAIL");
        return false;
    }

    SD_LOG("begin OK");

    historyReady = true;

    load_records_from_sd();

    SD_LOG("init OK");

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
    public_to_stored(record, stored);
    insert_record_at_front(stored);
    historyReady = true;

    SD_LOG("save RAM OK");

    return true;
#else
    SD_LOG("[SD] save");

    if (!historyReady) {
        if (!sd_history_init()) {
            SD_LOG("init FAIL");
            return false;
        }
    }

    File file = SD.open(SD_HISTORY_FILE, FILE_WRITE);

    if (!file) {
        SD_LOG("save FAIL");
        return false;
    }

    StoredRunHistoryRecord stored;
    public_to_stored(record, stored);

    write_record_line(file, stored);
    file.close();

    insert_record_at_front(stored);

    SD_LOG("save OK");

    return true;
#endif
}

bool sd_history_deleteRecord(uint8_t index) {
    if (!historyReady) {
        SD_LOG("del no init");
        return false;
    }

    if (index >= recordCount) {
        SD_LOG("del bad idx");
        return false;
    }

    SD_LOG("[SD] del");

    if (!remove_record_from_ram(index)) {
        SD_LOG("del RAM FAIL");
        return false;
    }

#if SD_HISTORY_USE_SAMPLE_DATA
    SD_LOG("del RAM OK");
    return true;
#else
    if (!rewrite_all_records_to_sd()) {
        SD_LOG("del SD FAIL");
        sd_history_init();
        return false;
    }

    SD_LOG("del OK");
    return true;
#endif
}

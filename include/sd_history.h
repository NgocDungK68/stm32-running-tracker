#ifndef SD_HISTORY_H
#define SD_HISTORY_H

#include <Arduino.h>

#ifndef SD_HISTORY_MAX_RECORDS
#define SD_HISTORY_MAX_RECORDS 10
#endif

struct RunHistoryRecord {
    const char *date;
    const char *start_time;

    float distance_m;

    uint32_t moving_time_s;
    uint32_t elapsed_time_s;

    uint16_t pause_count;

    float max_speed_kmph;

    const char *note;
};

bool sd_history_init();
bool sd_history_isReady();

uint8_t sd_history_getCount();
bool sd_history_getRecord(uint8_t index, RunHistoryRecord &record);

bool sd_history_saveRun(const RunHistoryRecord &record);
bool sd_history_deleteRecord(uint8_t index);

#endif
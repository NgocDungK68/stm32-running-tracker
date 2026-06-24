#ifndef SD_HISTORY_H
#define SD_HISTORY_H

#include <Arduino.h>

struct RunHistoryRecord {
    const char* date;          // "YYYY-MM-DD"
    const char* start_time;    // "HH:MM:SS"

    float distance_m;          // Tổng quãng đường, đơn vị mét

    uint32_t moving_time_s;    // Thời gian chạy thực, không tính pause
    uint32_t elapsed_time_s;   // Tổng thời gian từ Start đến Stop, có tính pause

    uint16_t pause_count;      // Số lần pause/auto-pause

    float max_speed_kmph;      // Tốc độ lớn nhất, km/h

    double start_lat;          // Vĩ độ điểm bắt đầu
    double start_lon;          // Kinh độ điểm bắt đầu

    double end_lat;            // Vĩ độ điểm kết thúc
    double end_lon;            // Kinh độ điểm kết thúc

    const char* note;          // Ghi chú ngắn: "GPS", "SIM", "OUTDOOR"...
};

bool sd_history_init();
bool sd_history_isReady();

bool sd_history_saveRun(const RunHistoryRecord &record);

bool sd_history_writeTest();
bool sd_history_readTest();

bool sd_history_printAll(Stream &out);
bool sd_history_clear();

#endif
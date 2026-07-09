#ifndef DISPLAY_H
#define DISPLAY_H

#include "app_types.h"
#include "sd_history.h"

struct DisplayData {
    float distance;       // mét
    int seconds;          // giây
    float pace;           // phút/km

    GpsStatus gps_status;

    bool user_paused;     // pause do người dùng bấm B1
    bool auto_paused;     // pause do thuật toán tự phát hiện đứng yên
};

bool display_init();

void display_show_home(unsigned char selectedOption);

void display_show_run_ready();
void display_show_run_data(const DisplayData &data);
void display_show_run_stop_confirm(const DisplayData &data);
void display_show_save_result(bool ok);

void display_show_history_placeholder();

void display_show_history_unavailable();
void display_show_history_empty();

void display_show_history_list(
    const RunHistoryRecord records[],
    uint8_t visibleCount,
    uint8_t selectedVisibleIndex,
    uint8_t totalCount,
    uint8_t topIndex
);

void display_show_history_detail(
    const RunHistoryRecord &record,
    uint8_t topLine
);

void display_show_history_delete_confirm(const RunHistoryRecord &record);
void display_show_delete_result(bool ok);

#endif
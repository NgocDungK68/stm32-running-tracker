#ifndef DISPLAY_H
#define DISPLAY_H

#include "app_types.h"
#include "sd_history.h"

struct DisplayData {
    float distance;       // mét
    int seconds;          // giây
    float pace;           // phút/km
    GpsStatus gps_status;
};

bool display_init();

void display_show_home(unsigned char selectedOption);

void display_show_run_ready();
void display_show_run_data(const DisplayData &data);

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

#endif
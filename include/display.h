#ifndef DISPLAY_H
#define DISPLAY_H

#include "app_types.h"

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

#endif
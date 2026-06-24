#ifndef GPS_H
#define GPS_H

#include "app_types.h"

struct GPSData {
    bool valid;          // true nếu GPS đã fix vị trí
    bool hasSignal;      // true nếu STM32 nhận được dữ liệu UART từ GPS

    double lat;
    double lon;
    float speed_kmph;

    GpsStatus status;
};

void gps_init();
GPSData gps_update();
GPSData gps_getData();

#endif
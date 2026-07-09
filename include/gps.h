#ifndef GPS_H
#define GPS_H

#include "app_types.h"

struct GPSData {
    bool valid;
    bool hasSignal;

    double lat;
    double lon;
    float speed_kmph;

    bool hasDateTime;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    GpsStatus status;
};

void gps_init();
GPSData gps_update();
GPSData gps_getData();

#endif
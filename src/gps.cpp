#include "gps.h"
#include "config.h"

#include <Arduino.h>
#include <TinyGPS++.h>

static TinyGPSPlus gpsParser;

HardwareSerial SerialGPS(USART2);

static GPSData currentData;
static unsigned long lastSignalTime = 0;

static bool is_leap_year(int year) {
    if (year % 400 == 0) {
        return true;
    }

    if (year % 100 == 0) {
        return false;
    }

    return (year % 4 == 0);
}

static int days_in_month(int year, int month) {
    switch (month) {
        case 1:
            return 31;

        case 2:
            return is_leap_year(year) ? 29 : 28;

        case 3:
            return 31;

        case 4:
            return 30;

        case 5:
            return 31;

        case 6:
            return 30;

        case 7:
            return 31;

        case 8:
            return 31;

        case 9:
            return 30;

        case 10:
            return 31;

        case 11:
            return 30;

        case 12:
            return 31;

        default:
            return 30;
    }
}

static void apply_timezone_offset(
    int &year,
    int &month,
    int &day,
    int &hour
) {
    hour += GPS_TIMEZONE_OFFSET_HOURS;

    while (hour >= 24) {
        hour -= 24;
        day++;

        if (day > days_in_month(year, month)) {
            day = 1;
            month++;

            if (month > 12) {
                month = 1;
                year++;
            }
        }
    }

    while (hour < 0) {
        hour += 24;
        day--;

        if (day < 1) {
            month--;

            if (month < 1) {
                month = 12;
                year--;
            }

            day = days_in_month(year, month);
        }
    }
}

static GPSData make_empty_gps_data() {
    GPSData data;

    data.valid = false;
    data.hasSignal = false;

    data.lat = 0.0;
    data.lon = 0.0;
    data.speed_kmph = 0.0f;

    data.hasDateTime = false;
    data.year = 0;
    data.month = 0;
    data.day = 0;
    data.hour = 0;
    data.minute = 0;
    data.second = 0;

    data.status = GPS_NO_DATA;

    return data;
}

void gps_init() {
    SerialGPS.begin(GPS_BAUDRATE);

    currentData = make_empty_gps_data();
    lastSignalTime = 0;
}

GPSData gps_update() {
    while (SerialGPS.available()) {
        char c = SerialGPS.read();

#if GPS_DEBUG_NMEA
        Serial.write(c);
#endif

        gpsParser.encode(c);
        lastSignalTime = millis();
    }

    currentData.hasSignal =
        (lastSignalTime != 0) &&
        (millis() - lastSignalTime < GPS_SIGNAL_TIMEOUT_MS);

    currentData.valid =
        currentData.hasSignal &&
        gpsParser.location.isValid();

    if (!currentData.hasSignal) {
        currentData.status = GPS_NO_DATA;

        currentData.valid = false;
        currentData.lat = 0.0;
        currentData.lon = 0.0;
        currentData.speed_kmph = 0.0f;

        currentData.hasDateTime = false;
    }
    else if (!currentData.valid) {
        currentData.status = GPS_NO_FIX;

        currentData.lat = 0.0;
        currentData.lon = 0.0;
        currentData.speed_kmph = 0.0f;
    }
    else {
        currentData.status = GPS_OK;

        currentData.lat = gpsParser.location.lat();
        currentData.lon = gpsParser.location.lng();

        if (gpsParser.speed.isValid()) {
            currentData.speed_kmph = gpsParser.speed.kmph();
        } else {
            currentData.speed_kmph = 0.0f;
        }
    }

    if (
        currentData.hasSignal &&
        gpsParser.date.isValid() &&
        gpsParser.time.isValid()
    ) {
        int year = gpsParser.date.year();
        int month = gpsParser.date.month();
        int day = gpsParser.date.day();

        int hour = gpsParser.time.hour();
        int minute = gpsParser.time.minute();
        int second = gpsParser.time.second();

        apply_timezone_offset(year, month, day, hour);

        currentData.hasDateTime = true;
        currentData.year = year;
        currentData.month = month;
        currentData.day = day;
        currentData.hour = hour;
        currentData.minute = minute;
        currentData.second = second;
    }

    return currentData;
}

GPSData gps_getData() {
    return currentData;
}
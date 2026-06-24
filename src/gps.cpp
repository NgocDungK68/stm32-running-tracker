#include "gps.h"
#include "config.h"

#include <Arduino.h>
#include <TinyGPS++.h>

static TinyGPSPlus gpsParser;

// Bạn đang dùng USART2:
// PA2 = TX2 của STM32
// PA3 = RX2 của STM32
HardwareSerial SerialGPS(USART2);

static GPSData currentData;
static unsigned long lastSignalTime = 0;

static GPSData make_empty_gps_data() {
    GPSData data;

    data.valid = false;
    data.hasSignal = false;

    data.lat = 0.0;
    data.lon = 0.0;
    data.speed_kmph = 0.0f;

    data.status = GPS_NO_DATA;

    return data;
}

void gps_init() {
    SerialGPS.begin(GPS_BAUDRATE);

#if GPS_DEBUG_NMEA
    Serial.begin(115200);
#endif

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

    return currentData;
}

GPSData gps_getData() {
    return currentData;
}
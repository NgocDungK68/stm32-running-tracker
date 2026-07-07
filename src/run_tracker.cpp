#include "run_tracker.h"
#include "config.h"

#include <Arduino.h>
#include <math.h>

// Nếu sau này muốn chỉnh ngưỡng, có thể đưa các define này sang config.h
#ifndef RUN_TRACKER_MIN_STEP_M
#define RUN_TRACKER_MIN_STEP_M 1.5f
#endif

#ifndef RUN_TRACKER_MAX_SPEED_MPS
#define RUN_TRACKER_MAX_SPEED_MPS 12.0f
#endif

static RunData currentRunData = {
    0.0f,   // distance, mét
    0,      // seconds
    0.0f    // pace, phút/km
};

static bool trackerStarted = false;

static bool hasLastPoint = false;
static double lastLat = 0.0;
static double lastLon = 0.0;

static unsigned long startTime = 0;
static unsigned long lastPointTime = 0;

static double deg_to_rad(double degree) {
    return degree * 0.017453292519943295;
}

static float calculate_distance_m(
    double lat1,
    double lon1,
    double lat2,
    double lon2
) {
    const double earthRadiusM = 6371000.0;

    double dLat = deg_to_rad(lat2 - lat1);
    double dLon = deg_to_rad(lon2 - lon1);

    double rLat1 = deg_to_rad(lat1);
    double rLat2 = deg_to_rad(lat2);

    double a =
        sin(dLat / 2.0) * sin(dLat / 2.0) +
        cos(rLat1) * cos(rLat2) *
        sin(dLon / 2.0) * sin(dLon / 2.0);

    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

    return (float)(earthRadiusM * c);
}

static void update_pace() {
    if (currentRunData.distance > 1.0f && currentRunData.seconds > 0) {
        float distanceKm = currentRunData.distance / 1000.0f;
        float timeMin = currentRunData.seconds / 60.0f;

        currentRunData.pace = timeMin / distanceKm;
    } else {
        currentRunData.pace = 0.0f;
    }
}

void run_tracker_init() {
    currentRunData.distance = 0.0f;
    currentRunData.seconds = 0;
    currentRunData.pace = 0.0f;

    hasLastPoint = false;
    lastLat = 0.0;
    lastLon = 0.0;

    startTime = millis();
    lastPointTime = startTime;

    trackerStarted = true;
}

RunData run_tracker_update(const GPSData &gpsData) {
    if (!trackerStarted) {
        run_tracker_init();
    }

    unsigned long now = millis();

    currentRunData.seconds = (int)((now - startTime) / 1000UL);

    // Nếu GPS chưa có fix thì chỉ cập nhật thời gian, chưa cộng quãng đường.
    if (!gpsData.valid) {
        update_pace();
        return currentRunData;
    }

    // Điểm GPS hợp lệ đầu tiên chỉ dùng làm mốc, chưa cộng quãng đường.
    if (!hasLastPoint) {
        lastLat = gpsData.lat;
        lastLon = gpsData.lon;
        lastPointTime = now;
        hasLastPoint = true;

        update_pace();
        return currentRunData;
    }

    float distanceStep = calculate_distance_m(
        lastLat,
        lastLon,
        gpsData.lat,
        gpsData.lon
    );

    float deltaTimeS = (now - lastPointTime) / 1000.0f;

    if (deltaTimeS <= 0.0f) {
        deltaTimeS = 1.0f;
    }

    // Giới hạn để tránh GPS nhảy tọa độ quá xa làm cộng sai quãng đường.
    float maxAllowedDistance = RUN_TRACKER_MAX_SPEED_MPS * deltaTimeS + 10.0f;

    if (maxAllowedDistance < 20.0f) {
        maxAllowedDistance = 20.0f;
    }

    if (
        distanceStep >= RUN_TRACKER_MIN_STEP_M &&
        distanceStep <= maxAllowedDistance
    ) {
        currentRunData.distance += distanceStep;
    }

    // Luôn cập nhật điểm cuối để tránh bị cộng lặp một bước nhảy GPS lỗi.
    lastLat = gpsData.lat;
    lastLon = gpsData.lon;
    lastPointTime = now;

    update_pace();

    return currentRunData;
}

RunData run_tracker_getData() {
    return currentRunData;
}
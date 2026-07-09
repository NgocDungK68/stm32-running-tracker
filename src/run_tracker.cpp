#include "run_tracker.h"
#include "config.h"

#include <Arduino.h>
#include <math.h>

#ifndef RUN_TRACKER_MIN_STEP_M
#define RUN_TRACKER_MIN_STEP_M 1.5f
#endif

#ifndef RUN_TRACKER_MAX_SPEED_MPS
#define RUN_TRACKER_MAX_SPEED_MPS 12.0f
#endif

#ifndef RUN_TRACKER_AUTO_PAUSE_SPEED_KMPH
#define RUN_TRACKER_AUTO_PAUSE_SPEED_KMPH 1.0f
#endif

#ifndef RUN_TRACKER_AUTO_RESUME_SPEED_KMPH
#define RUN_TRACKER_AUTO_RESUME_SPEED_KMPH 2.0f
#endif

#ifndef RUN_TRACKER_AUTO_RESUME_DISTANCE_M
#define RUN_TRACKER_AUTO_RESUME_DISTANCE_M 5.0f
#endif

#ifndef RUN_TRACKER_AUTO_PAUSE_DELAY_MS
#define RUN_TRACKER_AUTO_PAUSE_DELAY_MS 5000UL
#endif

static RunData currentRunData = {
    0.0f,
    0,
    0.0f
};

static bool trackerStarted = false;

static bool userPaused = false;
static bool autoPaused = false;

static bool hasLastPoint = false;
static bool hasStartPoint = false;
static bool hasEndPoint = false;

static double lastLat = 0.0;
static double lastLon = 0.0;

static double startLat = 0.0;
static double startLon = 0.0;

static double endLat = 0.0;
static double endLon = 0.0;

static unsigned long startWallTime = 0;
static unsigned long lastMoveUpdateTime = 0;
static unsigned long lastPointTime = 0;
static unsigned long stillStartTime = 0;

static unsigned long movingTimeMs = 0;

static uint16_t pauseCount = 0;
static float maxSpeedKmph = 0.0f;

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

static void add_moving_time_until(unsigned long now) {
    if (!trackerStarted || userPaused || autoPaused) {
        lastMoveUpdateTime = now;
        return;
    }

    if (lastMoveUpdateTime == 0) {
        lastMoveUpdateTime = now;
        return;
    }

    movingTimeMs += now - lastMoveUpdateTime;
    lastMoveUpdateTime = now;

    currentRunData.seconds = (int)(movingTimeMs / 1000UL);
}

static bool check_auto_pause(
    bool isMoving,
    unsigned long now
) {
    if (isMoving) {
        stillStartTime = 0;
        return false;
    }

    if (stillStartTime == 0) {
        stillStartTime = now;
        return false;
    }

    if (now - stillStartTime >= RUN_TRACKER_AUTO_PAUSE_DELAY_MS) {
        autoPaused = true;
        pauseCount++;

        lastMoveUpdateTime = now;

        update_pace();

        return true;
    }

    return false;
}

void run_tracker_init() {
    currentRunData.distance = 0.0f;
    currentRunData.seconds = 0;
    currentRunData.pace = 0.0f;

    trackerStarted = true;

    userPaused = false;
    autoPaused = false;

    hasLastPoint = false;
    hasStartPoint = false;
    hasEndPoint = false;

    lastLat = 0.0;
    lastLon = 0.0;

    startLat = 0.0;
    startLon = 0.0;

    endLat = 0.0;
    endLon = 0.0;

    startWallTime = millis();
    lastMoveUpdateTime = startWallTime;
    lastPointTime = startWallTime;
    stillStartTime = 0;

    movingTimeMs = 0;

    pauseCount = 0;
    maxSpeedKmph = 0.0f;
}

RunData run_tracker_update(const GPSData &gpsData) {
    if (!trackerStarted) {
        run_tracker_init();
    }

    unsigned long now = millis();

    if (userPaused) {
        lastMoveUpdateTime = now;
        update_pace();
        return currentRunData;
    }

    if (!gpsData.valid) {
        lastMoveUpdateTime = now;
        stillStartTime = 0;
        hasLastPoint = false;

        update_pace();
        return currentRunData;
    }

    if (!hasStartPoint) {
        startLat = gpsData.lat;
        startLon = gpsData.lon;
        hasStartPoint = true;
    }

    endLat = gpsData.lat;
    endLon = gpsData.lon;
    hasEndPoint = true;

    if (autoPaused) {
        if (!hasLastPoint) {
            lastLat = gpsData.lat;
            lastLon = gpsData.lon;
            lastPointTime = now;
            hasLastPoint = true;

            lastMoveUpdateTime = now;

            update_pace();
            return currentRunData;
        }

        float resumeDistance = calculate_distance_m(
            lastLat,
            lastLon,
            gpsData.lat,
            gpsData.lon
        );

        bool shouldResume =
            gpsData.speed_kmph >= RUN_TRACKER_AUTO_RESUME_SPEED_KMPH ||
            resumeDistance >= RUN_TRACKER_AUTO_RESUME_DISTANCE_M;

        if (!shouldResume) {
            lastMoveUpdateTime = now;

            update_pace();
            return currentRunData;
        }

        autoPaused = false;
        stillStartTime = 0;

        lastLat = gpsData.lat;
        lastLon = gpsData.lon;
        lastPointTime = now;
        lastMoveUpdateTime = now;

        update_pace();
        return currentRunData;
    }

    if (gpsData.speed_kmph > maxSpeedKmph && gpsData.speed_kmph < 80.0f) {
        maxSpeedKmph = gpsData.speed_kmph;
    }

    if (!hasLastPoint) {
        bool isMovingBySpeed =
            gpsData.speed_kmph >= RUN_TRACKER_AUTO_PAUSE_SPEED_KMPH;

        if (check_auto_pause(isMovingBySpeed, now)) {
            lastLat = gpsData.lat;
            lastLon = gpsData.lon;
            lastPointTime = now;
            hasLastPoint = true;

            update_pace();
            return currentRunData;
        }

        add_moving_time_until(now);

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

    float maxAllowedDistance =
        RUN_TRACKER_MAX_SPEED_MPS * deltaTimeS + 10.0f;

    if (maxAllowedDistance < 20.0f) {
        maxAllowedDistance = 20.0f;
    }

    bool distanceLooksValid =
        distanceStep >= RUN_TRACKER_MIN_STEP_M &&
        distanceStep <= maxAllowedDistance;

    bool isMoving =
        gpsData.speed_kmph >= RUN_TRACKER_AUTO_PAUSE_SPEED_KMPH ||
        distanceLooksValid;

    if (check_auto_pause(isMoving, now)) {
        lastLat = gpsData.lat;
        lastLon = gpsData.lon;
        lastPointTime = now;

        update_pace();
        return currentRunData;
    }

    add_moving_time_until(now);

    if (distanceLooksValid) {
        currentRunData.distance += distanceStep;

        float stepSpeedKmph = (distanceStep / deltaTimeS) * 3.6f;

        if (stepSpeedKmph > maxSpeedKmph && stepSpeedKmph < 80.0f) {
            maxSpeedKmph = stepSpeedKmph;
        }
    }

    lastLat = gpsData.lat;
    lastLon = gpsData.lon;
    lastPointTime = now;

    update_pace();

    return currentRunData;
}

RunData run_tracker_getData() {
    return currentRunData;
}

void run_tracker_pause(bool countPause) {
    if (!trackerStarted) {
        return;
    }

    if (userPaused) {
        return;
    }

    if (autoPaused) {
        autoPaused = false;
        userPaused = true;

        lastMoveUpdateTime = millis();
        stillStartTime = 0;
        hasLastPoint = false;

        return;
    }

    userPaused = true;

    lastMoveUpdateTime = millis();
    stillStartTime = 0;
    hasLastPoint = false;

    if (countPause) {
        pauseCount++;
    }

    update_pace();
}

void run_tracker_resume() {
    if (!trackerStarted) {
        return;
    }

    userPaused = false;
    autoPaused = false;

    unsigned long now = millis();

    lastMoveUpdateTime = now;
    lastPointTime = now;
    stillStartTime = 0;

    hasLastPoint = false;

    update_pace();
}

bool run_tracker_isPaused() {
    return userPaused || autoPaused;
}

bool run_tracker_isUserPaused() {
    return userPaused;
}

bool run_tracker_isAutoPaused() {
    return autoPaused;
}

bool run_tracker_finish(
    RunHistoryRecord &record,
    const char *date,
    const char *startTime,
    const char *note
) {
    if (!trackerStarted) {
        return false;
    }

    unsigned long elapsedTimeS = (millis() - startWallTime) / 1000UL;

    record.date = date;
    record.start_time = startTime;

    record.distance_m = currentRunData.distance;

    record.moving_time_s = movingTimeMs / 1000UL;
    record.elapsed_time_s = elapsedTimeS;

    record.pause_count = pauseCount;

    record.max_speed_kmph = maxSpeedKmph;

    record.start_lat = hasStartPoint ? startLat : 0.0;
    record.start_lon = hasStartPoint ? startLon : 0.0;

    record.end_lat = hasEndPoint ? endLat : 0.0;
    record.end_lon = hasEndPoint ? endLon : 0.0;

    record.note = note;

    return true;
}

void run_tracker_discard() {
    trackerStarted = false;

    userPaused = false;
    autoPaused = false;

    currentRunData.distance = 0.0f;
    currentRunData.seconds = 0;
    currentRunData.pace = 0.0f;

    hasLastPoint = false;
    hasStartPoint = false;
    hasEndPoint = false;

    stillStartTime = 0;
    movingTimeMs = 0;

    pauseCount = 0;
    maxSpeedKmph = 0.0f;
}
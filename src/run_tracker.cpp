#include "run_tracker.h"
#include "config.h"

#include <Arduino.h>
#include <math.h>

#ifndef RUN_TRACKER_MIN_STEP_M
#define RUN_TRACKER_MIN_STEP_M 1.5f
#endif

#ifndef RUN_TRACKER_MAX_VALID_SPEED_MPS
#define RUN_TRACKER_MAX_VALID_SPEED_MPS 8.0f
#endif

#ifndef RUN_TRACKER_GPS_JUMP_MARGIN_M
#define RUN_TRACKER_GPS_JUMP_MARGIN_M 8.0f
#endif

#ifndef RUN_TRACKER_MAX_SPEED_RECORD_KMPH
#define RUN_TRACKER_MAX_SPEED_RECORD_KMPH 40.0f
#endif

#ifndef RUN_TRACKER_CURRENT_PACE_SAMPLE_COUNT
#define RUN_TRACKER_CURRENT_PACE_SAMPLE_COUNT 8
#endif

#ifndef RUN_TRACKER_CURRENT_PACE_MIN_SAMPLES
#define RUN_TRACKER_CURRENT_PACE_MIN_SAMPLES 5
#endif

#ifndef RUN_TRACKER_CURRENT_PACE_MIN_DISTANCE_M
#define RUN_TRACKER_CURRENT_PACE_MIN_DISTANCE_M 6.0f
#endif

#ifndef RUN_TRACKER_AUTO_PAUSE_WINDOW_MS
#define RUN_TRACKER_AUTO_PAUSE_WINDOW_MS 5000UL
#endif

#ifndef RUN_TRACKER_AUTO_RESUME_WINDOW_MS
#define RUN_TRACKER_AUTO_RESUME_WINDOW_MS 5000UL
#endif

#ifndef RUN_TRACKER_AUTO_PAUSE_DISTANCE_M
#define RUN_TRACKER_AUTO_PAUSE_DISTANCE_M 2.5f
#endif

#ifndef RUN_TRACKER_AUTO_RESUME_DISTANCE_M
#define RUN_TRACKER_AUTO_RESUME_DISTANCE_M 4.0f
#endif

#define RUN_TRACKER_MAX_SAMPLE_COUNT 12

struct RunStepSample {
    unsigned long timeMs;
    unsigned long durationMs;
    float distanceM;
};

static RunData currentRunData = {
    0.0f,
    0,
    0.0f
};

static bool trackerStarted = false;

static bool userPaused = false;
static bool autoPaused = false;

static bool hasLastPoint = false;

static float lastLat = 0.0;
static float lastLon = 0.0;

static unsigned long startWallTime = 0;
static unsigned long lastMoveUpdateTime = 0;
static unsigned long lastPointTime = 0;

static unsigned long movingTimeMs = 0;

static uint16_t pauseCount = 0;
static float maxSpeedKmph = 0.0f;

static bool lastGpsOk = false;

static RunStepSample paceSamples[RUN_TRACKER_MAX_SAMPLE_COUNT];
static uint8_t paceSampleCount = 0;

static RunStepSample autoPauseSamples[RUN_TRACKER_MAX_SAMPLE_COUNT];
static uint8_t autoPauseSampleCount = 0;

static RunStepSample autoResumeSamples[RUN_TRACKER_MAX_SAMPLE_COUNT];
static uint8_t autoResumeSampleCount = 0;

static bool gps_is_ok_for_tracking(const GPSData &gpsData) {
    return gpsData.valid && gpsData.status == GPS_OK;
}

static double deg_to_rad(double degree) {
    return degree * 0.017453292519943295;
}

// static float calculate_distance_m(
//     float lat1,
//     float lon1,
//     float lat2,
//     float lon2
// ) {
//     const double earthRadiusM = 6371000.0;

//     double dLat = deg_to_rad(lat2 - lat1);
//     double dLon = deg_to_rad(lon2 - lon1);

//     double rLat1 = deg_to_rad(lat1);
//     double rLat2 = deg_to_rad(lat2);

//     double a =
//         sin(dLat / 2.0) * sin(dLat / 2.0) +
//         cos(rLat1) * cos(rLat2) *
//         sin(dLon / 2.0) * sin(dLon / 2.0);

//     double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

//     return (float)(earthRadiusM * c);
// }

// static float calculate_distance_m(
//     float lat1,
//     float lon1,
//     float lat2,
//     float lon2
// ) {
//     const float DEG_TO_M_LAT = 111320.0f;
//     const float MY_DEG_TO_RAD_F = 0.01745329252f;

//     float avgLatRad = ((lat1 + lat2) * 0.5f) * MY_DEG_TO_RAD_F;

//     float dLatM = (lat2 - lat1) * DEG_TO_M_LAT;
//     float dLonM = (lon2 - lon1) * DEG_TO_M_LAT * cosf(avgLatRad);

//     return sqrtf(dLatM * dLatM + dLonM * dLonM);
// }

static float calculate_distance_m(
    float lat1,
    float lon1,
    float lat2,
    float lon2
) {
    const float EARTH_RADIUS_M = 6371000.0f;
    const float DEG_TO_RAD_F = 0.01745329252f;

    float dLat = (lat2 - lat1) * DEG_TO_RAD_F;
    float dLon = (lon2 - lon1) * DEG_TO_RAD_F;

    float avgLatRad = ((lat1 + lat2) * 0.5f) * DEG_TO_RAD_F;

    float x = dLon * cosf(avgLatRad);
    float y = dLat;

    return EARTH_RADIUS_M * sqrtf(x * x + y * y);
}

static void clear_samples(
    RunStepSample samples[],
    uint8_t &sampleCount
) {
    sampleCount = 0;
}

static void remove_oldest_sample(
    RunStepSample samples[],
    uint8_t &sampleCount
) {
    if (sampleCount == 0) {
        return;
    }

    for (uint8_t i = 0; i + 1 < sampleCount; i++) {
        samples[i] = samples[i + 1];
    }

    sampleCount--;
}

static void add_sample(
    RunStepSample samples[],
    uint8_t &sampleCount,
    unsigned long now,
    float distanceM,
    unsigned long durationMs
) {
    if (durationMs == 0) {
        return;
    }

    if (sampleCount >= RUN_TRACKER_MAX_SAMPLE_COUNT) {
        remove_oldest_sample(samples, sampleCount);
    }

    samples[sampleCount].timeMs = now;
    samples[sampleCount].distanceM = distanceM;
    samples[sampleCount].durationMs = durationMs;

    sampleCount++;
}

static void trim_samples_by_age(
    RunStepSample samples[],
    uint8_t &sampleCount,
    unsigned long now,
    unsigned long windowMs
) {
    while (sampleCount > 0) {
        unsigned long age = now - samples[0].timeMs;

        if (age <= windowMs) {
            break;
        }

        remove_oldest_sample(samples, sampleCount);
    }
}

static float sum_distance(
    RunStepSample samples[],
    uint8_t sampleCount
) {
    float totalDistanceM = 0.0f;

    for (uint8_t i = 0; i < sampleCount; i++) {
        totalDistanceM += samples[i].distanceM;
    }

    return totalDistanceM;
}

static unsigned long sum_duration(
    RunStepSample samples[],
    uint8_t sampleCount
) {
    unsigned long totalDurationMs = 0;

    for (uint8_t i = 0; i < sampleCount; i++) {
        totalDurationMs += samples[i].durationMs;
    }

    return totalDurationMs;
}

/*
    Pace hiện tại trên màn hình chạy.

    Không dùng gpsData.speed_kmph.
    Pace được tính từ các mẫu GPS gần nhất đã lọc nhiễu.

    Công thức:
        pace = timeMin / distanceKm

    Trong đó:
        timeMin = tổng thời gian của các mẫu gần nhất / 60000
        distanceKm = tổng quãng đường của các mẫu gần nhất / 1000

    Nếu chưa đủ mẫu hoặc quãng đường quá nhỏ:
        pace = 0
        màn hình sẽ hiện "--"
*/
static void update_current_pace() {
    if (paceSampleCount < RUN_TRACKER_CURRENT_PACE_MIN_SAMPLES) {
        currentRunData.pace = 0.0f;
        return;
    }

    float totalDistanceM = sum_distance(
        paceSamples,
        paceSampleCount
    );

    unsigned long totalDurationMs = sum_duration(
        paceSamples,
        paceSampleCount
    );

    if (
        totalDistanceM < RUN_TRACKER_CURRENT_PACE_MIN_DISTANCE_M ||
        totalDurationMs == 0
    ) {
        currentRunData.pace = 0.0f;
        return;
    }

    float distanceKm = totalDistanceM / 1000.0f;
    float timeMin = totalDurationMs / 60000.0f;

    currentRunData.pace = timeMin / distanceKm;
}

static void add_current_pace_sample(
    unsigned long now,
    float distanceM,
    unsigned long durationMs
) {
    add_sample(
        paceSamples,
        paceSampleCount,
        now,
        distanceM,
        durationMs
    );

    while (paceSampleCount > RUN_TRACKER_CURRENT_PACE_SAMPLE_COUNT) {
        remove_oldest_sample(paceSamples, paceSampleCount);
    }

    update_current_pace();
}

static void clear_current_pace() {
    clear_samples(paceSamples, paceSampleCount);
    currentRunData.pace = 0.0f;
}

static void add_moving_time_until(unsigned long now) {
    if (!trackerStarted || userPaused || autoPaused || !lastGpsOk) {
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

/*
    Lọc nhiễu GPS.

    rawDistanceM:
        khoảng cách Haversine giữa 2 điểm liên tiếp.

    Nếu rawDistanceM quá nhỏ:
        coi là GPS drift, không cộng.

    Nếu rawDistanceM quá lớn so với deltaTime:
        coi là GPS jump, không cộng.

    Output:
        khoảng cách được chấp nhận để cộng vào distance.
*/
static float filter_distance_step(
    float rawDistanceM,
    unsigned long deltaTimeMs
) {
    if (deltaTimeMs == 0) {
        return 0.0f;
    }

    if (rawDistanceM < RUN_TRACKER_MIN_STEP_M) {
        return 0.0f;
    }

    float deltaTimeS = deltaTimeMs / 1000.0f;

    float maxAllowedDistance =
        RUN_TRACKER_MAX_VALID_SPEED_MPS * deltaTimeS +
        RUN_TRACKER_GPS_JUMP_MARGIN_M;

    if (rawDistanceM > maxAllowedDistance) {
        return 0.0f;
    }

    return rawDistanceM;
}

static void update_max_speed(
    float acceptedDistanceM,
    unsigned long deltaTimeMs
) {
    if (acceptedDistanceM <= 0.0f || deltaTimeMs == 0) {
        return;
    }

    float deltaTimeS = deltaTimeMs / 1000.0f;
    float stepSpeedKmph = (acceptedDistanceM / deltaTimeS) * 3.6f;

    if (
        stepSpeedKmph > maxSpeedKmph &&
        stepSpeedKmph <= RUN_TRACKER_MAX_SPEED_RECORD_KMPH
    ) {
        maxSpeedKmph = stepSpeedKmph;
    }
}

static bool should_auto_pause(
    unsigned long now,
    float acceptedDistanceM,
    unsigned long durationMs
) {
    add_sample(
        autoPauseSamples,
        autoPauseSampleCount,
        now,
        acceptedDistanceM,
        durationMs
    );

    trim_samples_by_age(
        autoPauseSamples,
        autoPauseSampleCount,
        now,
        RUN_TRACKER_AUTO_PAUSE_WINDOW_MS
    );

    unsigned long windowDurationMs = sum_duration(
        autoPauseSamples,
        autoPauseSampleCount
    );

    if (windowDurationMs < RUN_TRACKER_AUTO_PAUSE_WINDOW_MS) {
        return false;
    }

    float windowDistanceM = sum_distance(
        autoPauseSamples,
        autoPauseSampleCount
    );

    return windowDistanceM < RUN_TRACKER_AUTO_PAUSE_DISTANCE_M;
}

static bool should_auto_resume(
    unsigned long now,
    float acceptedDistanceM,
    unsigned long durationMs
) {
    add_sample(
        autoResumeSamples,
        autoResumeSampleCount,
        now,
        acceptedDistanceM,
        durationMs
    );

    trim_samples_by_age(
        autoResumeSamples,
        autoResumeSampleCount,
        now,
        RUN_TRACKER_AUTO_RESUME_WINDOW_MS
    );

    unsigned long windowDurationMs = sum_duration(
        autoResumeSamples,
        autoResumeSampleCount
    );

    if (windowDurationMs < RUN_TRACKER_AUTO_RESUME_WINDOW_MS) {
        return false;
    }

    float windowDistanceM = sum_distance(
        autoResumeSamples,
        autoResumeSampleCount
    );

    return windowDistanceM >= RUN_TRACKER_AUTO_RESUME_DISTANCE_M;
}

static void clear_tracking_windows() {
    clear_samples(autoPauseSamples, autoPauseSampleCount);
    clear_samples(autoResumeSamples, autoResumeSampleCount);
    clear_current_pace();
}

static void enter_auto_pause(unsigned long now) {
    autoPaused = true;
    pauseCount++;

    lastMoveUpdateTime = now;

    clear_samples(autoResumeSamples, autoResumeSampleCount);
    clear_current_pace();
}

static void exit_auto_pause(unsigned long now) {
    autoPaused = false;

    lastMoveUpdateTime = now;
    lastPointTime = now;

    clear_samples(autoPauseSamples, autoPauseSampleCount);
    clear_samples(autoResumeSamples, autoResumeSampleCount);

    clear_current_pace();

    hasLastPoint = false;
}

/*
    GPS không OK thì dừng toàn bộ thông số.

    Không cộng:
    - thời gian
    - distance
    - pace
    - auto pause / auto continue

    Đồng thời reset mốc GPS để khi GPS OK lại,
    không nối nhầm điểm cũ sang điểm mới.
*/
static void handle_gps_not_ok(unsigned long now) {
    lastGpsOk = false;

    lastMoveUpdateTime = now;
    lastPointTime = now;

    hasLastPoint = false;

    clear_tracking_windows();
}

void run_tracker_init() {
    currentRunData.distance = 0.0f;
    currentRunData.seconds = 0;
    currentRunData.pace = 0.0f;

    trackerStarted = true;

    userPaused = false;
    autoPaused = false;

    hasLastPoint = false;

    lastLat = 0.0;
    lastLon = 0.0;

    startWallTime = millis();
    lastMoveUpdateTime = startWallTime;
    lastPointTime = startWallTime;

    movingTimeMs = 0;

    pauseCount = 0;
    maxSpeedKmph = 0.0f;

    lastGpsOk = false;

    clear_tracking_windows();
}

RunData run_tracker_update(const GPSData &gpsData) {
    if (!trackerStarted) {
        run_tracker_init();
    }

    unsigned long now = millis();

    if (userPaused) {
        lastMoveUpdateTime = now;

        if (!gps_is_ok_for_tracking(gpsData)) {
            handle_gps_not_ok(now);
        }

        return currentRunData;
    }

    if (!gps_is_ok_for_tracking(gpsData)) {
        handle_gps_not_ok(now);
        return currentRunData;
    }

    lastGpsOk = true;

    if (!hasLastPoint) {
        lastLat = gpsData.lat;
        lastLon = gpsData.lon;
        lastPointTime = now;
        lastMoveUpdateTime = now;
        hasLastPoint = true;

        return currentRunData;
    }

    unsigned long deltaTimeMs = now - lastPointTime;

    if (deltaTimeMs == 0) {
        deltaTimeMs = 1000UL;
    }

    float rawDistanceM = calculate_distance_m(
        lastLat,
        lastLon,
        (float)gpsData.lat,
        (float)gpsData.lon
    );

    float acceptedDistanceM = filter_distance_step(
        rawDistanceM,
        deltaTimeMs
    );

    if (autoPaused) {
        if (
            should_auto_resume(
                now,
                acceptedDistanceM,
                deltaTimeMs
            )
        ) {
            exit_auto_pause(now);

            lastLat = gpsData.lat;
            lastLon = gpsData.lon;
            lastPointTime = now;

            return currentRunData;
        }

        lastLat = gpsData.lat;
        lastLon = gpsData.lon;
        lastPointTime = now;
        lastMoveUpdateTime = now;

        return currentRunData;
    }

    if (
        should_auto_pause(
            now,
            acceptedDistanceM,
            deltaTimeMs
        )
    ) {
        enter_auto_pause(now);

        lastLat = gpsData.lat;
        lastLon = gpsData.lon;
        lastPointTime = now;

        return currentRunData;
    }

    add_moving_time_until(now);

    currentRunData.distance += acceptedDistanceM;

    add_current_pace_sample(
        now,
        acceptedDistanceM,
        deltaTimeMs
    );

    update_max_speed(
        acceptedDistanceM,
        deltaTimeMs
    );

    lastLat = gpsData.lat;
    lastLon = gpsData.lon;
    lastPointTime = now;

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

    userPaused = true;
    autoPaused = false;

    lastMoveUpdateTime = millis();
    hasLastPoint = false;

    clear_tracking_windows();

    if (countPause) {
        pauseCount++;
    }
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

    hasLastPoint = false;
    lastGpsOk = false;

    clear_tracking_windows();
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

    unsigned long now = millis();

    if (!userPaused && !autoPaused && lastGpsOk) {
        add_moving_time_until(now);
    }

    unsigned long elapsedTimeS = (now - startWallTime) / 1000UL;

    record.date = date;
    record.start_time = startTime;

    record.distance_m = currentRunData.distance;

    record.moving_time_s = movingTimeMs / 1000UL;
    record.elapsed_time_s = elapsedTimeS;

    record.pause_count = pauseCount;

    record.max_speed_kmph = maxSpeedKmph;

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

    lastLat = 0.0;
    lastLon = 0.0;

    movingTimeMs = 0;

    pauseCount = 0;
    maxSpeedKmph = 0.0f;

    lastGpsOk = false;

    clear_tracking_windows();
}
#ifndef RUN_TRACKER_H
#define RUN_TRACKER_H

#include "gps.h"
#include "simulator.h"
#include "sd_history.h"

void run_tracker_init();

RunData run_tracker_update(const GPSData &gpsData);
RunData run_tracker_getData();

void run_tracker_pause(bool countPause = true);
void run_tracker_resume();

bool run_tracker_isPaused();
bool run_tracker_isUserPaused();
bool run_tracker_isAutoPaused();

bool run_tracker_finish(
    RunHistoryRecord &record,
    const char *date,
    const char *startTime,
    const char *note
);

void run_tracker_discard();

#endif
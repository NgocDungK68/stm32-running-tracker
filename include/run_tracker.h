#ifndef RUN_TRACKER_H
#define RUN_TRACKER_H

#include "gps.h"
#include "simulator.h"

// Khởi tạo lại một buổi chạy thật bằng GPS
void run_tracker_init();

// Cập nhật dữ liệu chạy thật từ GPSData
RunData run_tracker_update(const GPSData &gpsData);

// Lấy dữ liệu chạy hiện tại
RunData run_tracker_getData();

#endif
#include "display.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>

static Adafruit_SSD1306 display(
    OLED_SCREEN_WIDTH,
    OLED_SCREEN_HEIGHT,
    &Wire,
    -1
);

static const uint8_t HISTORY_VISIBLE_ROWS = 4;

static const char* gps_status_to_text(GpsStatus status) {
    switch (status) {
        case GPS_NO_DATA:
            return "NO DATA";

        case GPS_NO_FIX:
            return "NO FIX";

        case GPS_OK:
            return "OK";

        case GPS_SIM:
            return "SIM";

        default:
            return "UNKNOWN";
    }
}

static void draw_menu_item(int y, const char* text, bool selected) {
    if (selected) {
        display.fillRect(0, y - 1, 128, 12, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(4, y);
        display.print("> ");
        display.println(text);
        display.setTextColor(SSD1306_WHITE);
    } else {
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(4, y);
        display.print("  ");
        display.println(text);
    }
}

static void print_time_mmss(int seconds) {
    int minute = seconds / 60;
    int second = seconds % 60;

    display.print(minute);
    display.print(":");

    if (second < 10) {
        display.print("0");
    }

    display.print(second);
}

static void print_duration_hhmmss(uint32_t seconds) {
    uint32_t hour = seconds / 3600UL;
    uint32_t minute = (seconds % 3600UL) / 60UL;
    uint32_t second = seconds % 60UL;

    if (hour > 0) {
        display.print(hour);
        display.print(":");

        if (minute < 10) {
            display.print("0");
        }

        display.print(minute);
        display.print(":");

        if (second < 10) {
            display.print("0");
        }

        display.print(second);
    } else {
        display.print(minute);
        display.print(":");

        if (second < 10) {
            display.print("0");
        }

        display.print(second);
    }
}

static float calculate_average_pace(const RunHistoryRecord &record) {
    if (record.distance_m <= 1.0f || record.moving_time_s == 0) {
        return 0.0f;
    }

    float distanceKm = record.distance_m / 1000.0f;
    float timeMin = record.moving_time_s / 60.0f;

    return timeMin / distanceKm;
}

static void print_pace(float pace) {
    if (pace > 0.0f) {
        int pace_min = (int)pace;
        int pace_sec = (int)((pace - pace_min) * 60.0f);

        display.print(pace_min);
        display.print(":");

        if (pace_sec < 10) {
            display.print("0");
        }

        display.print(pace_sec);
        display.print("/km");
    } else {
        display.print("--");
    }
}

static void make_short_date(const char *date, char *out, size_t outSize) {
    // Input:  "YYYY-MM-DD"
    // Output: "DD/MM/YY"

    if (outSize < 9) {
        return;
    }

    if (date == NULL || strlen(date) < 10) {
        strncpy(out, "--/--/--", outSize - 1);
        out[outSize - 1] = '\0';
        return;
    }

    out[0] = date[8];
    out[1] = date[9];
    out[2] = '/';
    out[3] = date[5];
    out[4] = date[6];
    out[5] = '/';
    out[6] = date[2];
    out[7] = date[3];
    out[8] = '\0';
}

bool display_init() {
    Wire.begin();

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        return false;
    }

    display.clearDisplay();
    display.display();

    return true;
}

void display_show_home(unsigned char selectedOption) {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(22, 0);
    display.println("RUN TRACKER");

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    draw_menu_item(18, "START RUN", selectedOption == 0);
    draw_menu_item(34, "RUN HISTORY", selectedOption == 1);

    display.setCursor(0, 56);
    display.setTextColor(SSD1306_WHITE);
    display.print("B1 OK  B2 NEXT");

    display.display();
}

void display_show_run_ready() {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(25, 0);
    display.println("START RUN");

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 18);
    display.println("Ready to run");

    display.setCursor(0, 32);
    display.println("B1: Start");

    display.setCursor(0, 46);
    display.println("Hold B2: Back");

    display.display();
}

void display_show_run_data(const DisplayData &data) {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print("RUNNING");

    display.setCursor(0, 12);
    display.print("Dist: ");
    display.print(data.distance / 1000.0f, 2);
    display.println(" km");

    display.setCursor(0, 24);
    display.print("Time: ");
    print_time_mmss(data.seconds);
    display.println();

    display.setCursor(0, 36);
    display.print("Pace: ");
    print_pace(data.pace);
    display.println();

    display.setCursor(0, 48);
    display.print("GPS: ");
    display.println(gps_status_to_text(data.gps_status));

    display.display();
}

void display_show_history_placeholder() {
    display_show_history_empty();
}

void display_show_history_unavailable() {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("RUN HISTORY");

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 18);
    display.println("MicroSD not ready");

    display.setCursor(0, 32);
    display.println("Check CS/SPI/power");

    display.setCursor(0, 46);
    display.println("or enable sample");

    display.display();
}

void display_show_history_empty() {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("RUN HISTORY");

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 22);
    display.println("No run history");

    display.setCursor(0, 36);
    display.println("available yet");

    display.display();
}

static void draw_history_row(
    int y,
    const RunHistoryRecord &record,
    bool selected
) {
    if (selected) {
        display.fillRect(0, y - 1, 128, 12, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    } else {
        display.setTextColor(SSD1306_WHITE);
    }

    char dateShort[9];
    make_short_date(record.date, dateShort, sizeof(dateShort));

    float distanceKm = record.distance_m / 1000.0f;

    // Bên trái: ngày chạy
    display.setCursor(2, y);
    display.print(dateShort);

    // Bên phải: số km chạy
    // OLED 128px, font size 1 khoảng 6px/ký tự.
    // x = 82 đủ cho dạng "10.12km".
    display.setCursor(82, y);
    display.print(distanceKm, 2);
    display.print("km");

    display.setTextColor(SSD1306_WHITE);
}

void display_show_history_list(
    const RunHistoryRecord records[],
    uint8_t visibleCount,
    uint8_t selectedVisibleIndex,
    uint8_t totalCount,
    uint8_t topIndex
) {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Tiêu đề bên trái
    display.setCursor(0, 0);
    display.println("RUN HISTORY");

    // Chỉ số bên phải là bản ghi đang chọn / tổng số bản ghi
    uint8_t selectedGlobalIndex = topIndex + selectedVisibleIndex;

    display.setCursor(104, 0);
    display.print(selectedGlobalIndex + 1);
    display.print("/");
    display.print(totalCount);

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    for (uint8_t i = 0; i < visibleCount && i < HISTORY_VISIBLE_ROWS; i++) {
        int y = 14 + i * 12;

        draw_history_row(
            y,
            records[i],
            i == selectedVisibleIndex
        );
    }

    display.display();
}

static void draw_detail_line(
    uint8_t lineIndex,
    int y,
    const RunHistoryRecord &record
) {
    float distanceKm = record.distance_m / 1000.0f;
    float avgPace = calculate_average_pace(record);

    display.setCursor(0, y);

    switch (lineIndex) {
        case 0:
            display.print("Date: ");
            display.print(record.date);
            break;

        case 1:
            display.print("Start: ");
            display.print(record.start_time);
            break;

        case 2:
            display.print("Dist: ");
            display.print(distanceKm, 2);
            display.print(" km");
            break;

        case 3:
            display.print("Move: ");
            print_duration_hhmmss(record.moving_time_s);
            break;

        case 4:
            display.print("Total: ");
            print_duration_hhmmss(record.elapsed_time_s);
            break;

        case 5:
            display.print("Pace: ");
            print_pace(avgPace);
            break;

        case 6:
            display.print("Pause: ");
            display.print(record.pause_count);
            break;

        case 7:
            display.print("Max: ");
            display.print(record.max_speed_kmph, 1);
            display.print(" km/h");
            break;

        case 8:
            display.print("SLat: ");
            display.print(record.start_lat, 5);
            break;

        case 9:
            display.print("SLon: ");
            display.print(record.start_lon, 5);
            break;

        case 10:
            display.print("ELat: ");
            display.print(record.end_lat, 5);
            break;

        case 11:
            display.print("ELon: ");
            display.print(record.end_lon, 5);
            break;

        case 12:
            display.print("Note: ");
            display.print(record.note);
            break;

        default:
            break;
    }
}

void display_show_history_detail(
    const RunHistoryRecord &record,
    uint8_t topLine
) {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("DETAIL HISTORY");

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t lineIndex = topLine + i;

        if (lineIndex <= 12) {
            int y = 14 + i * 12;
            draw_detail_line(lineIndex, y, record);
        }
    }

    display.display();
}
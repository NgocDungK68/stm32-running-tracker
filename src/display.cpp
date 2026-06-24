#include "display.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 display(
    OLED_SCREEN_WIDTH,
    OLED_SCREEN_HEIGHT,
    &Wire,
    -1
);

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
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(22, 0);
    display.println("RUN HISTORY");

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 18);
    display.println("History viewer");

    display.setCursor(0, 30);
    display.println("will be added");

    display.setCursor(0, 46);
    display.println("Hold B2: Back");

    display.display();
}
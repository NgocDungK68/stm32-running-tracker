#ifndef SIMULATOR_H
#define SIMULATOR_H

struct RunData {
    float distance; // meters
    int seconds;    // total time
    float pace;     // min/km
};

void simulator_init();
RunData simulator_update();

#endif
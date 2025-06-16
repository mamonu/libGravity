#ifndef APP_STATE_H
#define APP_STATE_H

#include <gravity.h>

#include "channel.h"

// Global state for settings and app behavior.
struct AppState {
    int tempo = Clock::DEFAULT_TEMPO;
    bool encoder_reversed = false;
    bool refresh_screen = true;
    bool editing_param = false;
    int selected_param = 0;
    int selected_sub_param = 0;
    byte selected_channel = 0;  // 0=tempo, 1-6=output channel
    Clock::Source selected_source = Clock::SOURCE_INTERNAL;
    Channel channel[Gravity::OUTPUT_COUNT];
};

#endif  // APP_STATE_H
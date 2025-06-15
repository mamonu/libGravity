#ifndef APP_STATE_H
#define APP_STATE_H

#include <gravity.h>
#include "channel.h"

// Global state for settings and app behavior.
struct AppState {
    int tempo = 120;
    bool refresh_screen = true;
    bool editing_param = false;
    int selected_param = 0;
    byte selected_channel = 0;  // 0=tempo, 1-6=output channel
    Source selected_source = SOURCE_INTERNAL;
    Channel channel[OUTPUT_COUNT];
};

#endif // APP_STATE_H
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
    byte selected_shuffle = 0;  // index into shuffle template
    Clock::Source selected_source = Clock::SOURCE_INTERNAL;
    Channel channel[Gravity::OUTPUT_COUNT];
};

extern AppState app;

static Channel& GetSelectedChannel() {
    return app.channel[app.selected_channel - 1];
}

enum ParamsMainPage {
    PARAM_MAIN_TEMPO,
    PARAM_MAIN_SOURCE,
    PARAM_MAIN_ENCODER_DIR,
    PARAM_MAIN_RESET_STATE,
    PARAM_MAIN_LAST,
};

enum ParamsChannelPage {
    PARAM_CH_MOD,
    PARAM_CH_PROB,
    PARAM_CH_DUTY,
    PARAM_CH_OFFSET,
    PARAM_CH_SWING,
    PARAM_CH_CV_SRC,
    PARAM_CH_CV_DEST,
    PARAM_CH_LAST,
};

#endif  // APP_STATE_H
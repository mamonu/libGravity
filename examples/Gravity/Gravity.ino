/**
 * @file Gravity.ino
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Demo firmware for Sitka Instruments Gravity.
 * @version 0.1
 * @date 2025-05-04
 *
 * @copyright Copyright (c) 2025
 *
 * ENCODER:
 *      Press to change between selecting a parameter and editing the parameter.
 *      Hold & Rotate to change current output channel pattern.
 *
 * BTN1: Play/pause the internal clock.
 *
 * BTN2: Stop all clocks.
 *
 */

#include <gravity.h>

#include "app_state.h"
#include "channel.h"
#include "save_state.h"
#include "display.h"

AppState app;
StateManager stateManager;

//
// Arduino setup and loop.
//

void setup() {
    // Start Gravity.
    gravity.Init();

    // Initialize the state manager. This will load settings from EEPROM
    stateManager.initialize(app);
    InitAppState(app);

    // Clock handlers.
    gravity.clock.AttachIntHandler(HandleIntClockTick);
    gravity.clock.AttachExtHandler(HandleExtClockTick);

    // Encoder rotate and press handlers.
    gravity.encoder.AttachPressHandler(HandleEncoderPressed);
    gravity.encoder.AttachRotateHandler(HandleRotate);
    gravity.encoder.AttachPressRotateHandler(HandlePressedRotate);

    // Button press handlers.
    gravity.play_button.AttachPressHandler(HandlePlayPressed);
    gravity.shift_button.AttachPressHandler(HandleShiftPressed);
}

void loop() {
    // Process change in state of inputs and outputs.
    gravity.Process();

    // Read CVs and call the update function for each channel.
    int cv1 = gravity.cv1.Read();
    int cv2 = gravity.cv2.Read();
    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        app.channel[i].applyCvMod(cv1, cv2);
    }

    // Check for dirty state eligible to be saved.
    stateManager.update(app);

    if (app.refresh_screen) {
        UpdateDisplay();
    }
}

//
// Firmware handlers for clocks.
//

void HandleIntClockTick(uint32_t tick) {
    bool refresh = false;
    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        app.channel[i].processClockTick(tick, gravity.outputs[i]);

        if (app.channel[i].isCvModActive()) {
            refresh = true;
        }
    }

    if (!app.editing_param) {
        app.refresh_screen |= refresh;
    }
}

void HandleExtClockTick() {
    // Ignore tick if not using external source.
    if (!gravity.clock.ExternalSource()) {
        return;
    }
    gravity.clock.Tick();
    app.refresh_screen = true;
}

//
// UI handlers for encoder and buttons.
//

void HandlePlayPressed() {
    gravity.clock.IsPaused()
        ? gravity.clock.Start()
        : gravity.clock.Stop();
    ResetOutputs();
    app.refresh_screen = true;
}

void HandleShiftPressed() {
    gravity.clock.Stop();
    ResetOutputs();
    app.refresh_screen = true;
}

void HandleEncoderPressed() {
    // Check if leaving editing mode should apply a selection.
    if (app.editing_param) {
        if (app.selected_channel == 0) {  // main page
            if (app.selected_param == PARAM_MAIN_ENCODER_DIR) {
                bool reversed = app.selected_sub_param == 1;
                gravity.encoder.SetReverseDirection(reversed);
            }
            // Reset state
            if (app.selected_param == PARAM_MAIN_RESET_STATE) {
                if (app.selected_sub_param == 0) {  // Reset
                    stateManager.reset(app);
                    InitAppState(app);
                }
            }
        }
        // Only mark dirty when leaving editing mode.
        stateManager.markDirty();
    }
    app.selected_sub_param = 0;
    app.editing_param = !app.editing_param;
    app.refresh_screen = true;
}

void HandleRotate(Direction dir, int val) {
    if (!app.editing_param) {
        // Navigation Mode
        const int max_param = (app.selected_channel == 0) ? PARAM_MAIN_LAST : PARAM_CH_LAST;
        updateSelection(app.selected_param, val, max_param);
    } else {
        // Editing Mode
        if (app.selected_channel == 0) {
            editMainParameter(val);
        } else {
            editChannelParameter(val);
        }
    }
    app.refresh_screen = true;
}

void HandlePressedRotate(Direction dir, int val) {
    if (dir == DIRECTION_INCREMENT && app.selected_channel < Gravity::OUTPUT_COUNT) {
        app.selected_channel++;
    } else if (dir == DIRECTION_DECREMENT && app.selected_channel > 0) {
        app.selected_channel--;
    }
    app.selected_param = 0;
    stateManager.markDirty();
    app.refresh_screen = true;
}

void editMainParameter(int val) {
    switch (static_cast<ParamsMainPage>(app.selected_param)) {
        case PARAM_MAIN_TEMPO:
            if (gravity.clock.ExternalSource()) {
                break;
            }
            gravity.clock.SetTempo(gravity.clock.Tempo() + val);
            app.tempo = gravity.clock.Tempo();
            break;

        case PARAM_MAIN_SOURCE: {
            int source = static_cast<int>(app.selected_source);
            updateSelection(source, val, Clock::SOURCE_LAST);
            app.selected_source = static_cast<Clock::Source>(source);
            gravity.clock.SetSource(app.selected_source);
            break;
        }
        case PARAM_MAIN_ENCODER_DIR:
            updateSelection(app.selected_sub_param, val, 2);
            break;
        case PARAM_MAIN_RESET_STATE:
            updateSelection(app.selected_sub_param, val, 2);
            break;
    }
}

void editChannelParameter(int val) {
    auto& ch = GetSelectedChannel();
    switch (app.selected_param) {
        case PARAM_CH_MOD:
            ch.setClockMod(ch.getClockModIndex() + val);
            break;
        case PARAM_CH_PROB:
            ch.setProbability(ch.getProbability() + val);
            break;
        case PARAM_CH_DUTY:
            ch.setDutyCycle(ch.getDutyCycle() + val);
            break;
        case PARAM_CH_OFFSET:
            ch.setOffset(ch.getOffset() + val);
            break;
        case PARAM_CH_CV_SRC: {
            int source = static_cast<int>(ch.getCvSource());
            updateSelection(source, val, CV_LAST);
            ch.setCvSource(static_cast<CvSource>(source));
            break;
        }
        case PARAM_CH_CV_DEST: {
            int dest = static_cast<int>(ch.getCvDestination());
            updateSelection(dest, val, CV_DEST_LAST);
            ch.setCvDestination(static_cast<CvDestination>(dest));
            break;
        }
    }
}

void updateSelection(int& param, int change, int maxValue) {
    // This formula correctly handles positive and negative wrapping.
    param = (param + change % maxValue + maxValue) % maxValue;
}

//
// Helper functions.
//

void InitAppState(AppState& app) {
    gravity.clock.SetTempo(app.tempo);
    gravity.clock.SetSource(app.selected_source);
    gravity.encoder.SetReverseDirection(app.encoder_reversed);
}

void ResetOutputs() {
    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        gravity.outputs[i].Low();
    }
}

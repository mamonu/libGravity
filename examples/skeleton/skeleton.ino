/**
 * @file skeleton.ino
 * @author YOUR_NAME (<url>)
 * @brief Skeleton app for Sitka Instruments Gravity.
 * @version v1.0.0 - August 2025
 * @date 2025-08-12
 *
 * @copyright MIT - (c) 2025 - Adam Wonak - adam.wonak@gmail.com
 * 
 * Skeleton app for basic structure of a new firmware for Sitka Instruments
 * Gravity using the libGravity library.
 *
 * ENCODER:
 *      Press: change between selecting a parameter and editing the parameter.
 *      Hold & Rotate: change current selected output channel.
 *
 * BTN1:
 *      Play/pause - start or stop the internal clock.
 *
 * BTN2:
 *      Shift - hold and rotate encoder to change current selected output channel.
 *
 * EXT:
 *      External clock input. When Gravity is set to INTERNAL or MIDI clock
 *      source, this input is used to reset clocks.
 *
 * CV1:
 *      External analog input used to provide modulation to any channel parameter.
 *
 * CV2:
 *      External analog input used to provide modulation to any channel parameter.
 *
 */

#include <libGravity.h>



// Global state for settings and app behavior.
struct AppState {
    int tempo = Clock::DEFAULT_TEMPO;
    Clock::Source selected_source = Clock::SOURCE_INTERNAL;
    // Add app specific state variables here.
};

AppState app;

//
// Arduino setup and loop.
//

void setup() {
    // Start Gravity.
    gravity.Init();

    // Clock handlers.
    gravity.clock.AttachIntHandler(HandleIntClockTick);
    gravity.clock.AttachExtHandler(HandleExtClockTick);

    // Encoder rotate and press handlers.
    gravity.encoder.AttachPressHandler(HandleEncoderPressed);
    gravity.encoder.AttachRotateHandler(HandleRotate);
    gravity.encoder.AttachPressRotateHandler(HandlePressedRotate);

    // Button press handlers.
    gravity.play_button.AttachPressHandler(HandlePlayPressed);
}

void loop() {
    // Process change in state of inputs and outputs.
    gravity.Process();

    // Non-ISR loop behavior.
}

//
// Firmware handlers for clocks.
//

void HandleIntClockTick(uint32_t tick) {
    bool refresh = false;
    for (int i = 0; i < Gravity::OUTPUT_COUNT; i++) {
        // Process each output tick handlers.
    }
}

void HandleExtClockTick() {
    switch (app.selected_source) {
        case Clock::SOURCE_INTERNAL:
        case Clock::SOURCE_EXTERNAL_MIDI:
            // Use EXT as Reset when not used for clock source.
            gravity.clock.Reset();
            break;
        default:
            // Register EXT cv clock tick.
            gravity.clock.Tick();
    }
}

//
// UI handlers for encoder and buttons.
//

void HandlePlayPressed() {
}

void HandleEncoderPressed() {
}

void HandleRotate(int val) {
}

void HandlePressedRotate(int val) {
}

//
// Application logic goes here.
//

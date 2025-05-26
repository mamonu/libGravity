/**
 * @file clock_mod.ino
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
 * BTN2: Shift button for additional UI navigation (unused).
 *
 */

#include "gravity.h"

// Firmware state variables.
struct Channel {
    byte clock_mod_index = 7;  // x1
    byte probability = 100;
    byte duty_cycle = 50;
    // int duty_cycle_pulses = 12;  // 120 x1 24 PPQN
    int duty_cycle_pulses = 48;  // 120 x1 96 PPQN
    byte offset = 0;
    int offset_pulses = 0;
};
struct AppState {
    bool refresh_screen = true;
    byte selected_param = 0;
    byte selected_channel = 0;  // 0=tempo, 1-6=output channel
    Channel channel[OUTPUT_COUNT];
};
AppState app;

// The number of clock mod options, hepls validate choices and pulses arrays are the same size.
const int MOD_CHOICE_SIZE = 21;
// negative=multiply, positive=divide
const int clock_mod[MOD_CHOICE_SIZE] = {-24, -12, -8, -6, -4, -3, -2, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32, 64, 128};

// This represents the number of clock pulses for a 24 PPQN clock source that match the above div/mult mods.
// const int clock_mod_pulses[MOD_CHOICE_SIZE] = {1, 2, 3, 4, 6, 8, 12, 24, 48, 72, 96, 120, 144, 288, 168, 192, 384, 576, 768, 1536, 3072};  // LCM(322560)

// This represents the number of clock pulses for a 96 PPQN clock source that match the above div/mult mods.
const int clock_mod_pulses[MOD_CHOICE_SIZE] = {4, 8, 12, 16, 24, 32, 48, 96, 192, 288, 384, 480, 576, 1152, 672, 768, 1536, 2304, 3072, 6144, 12288};

const byte CHAR_PLAY = 0x10;
const byte CHAR_PAUSE = 0xB9;

//
// Arduino setup and loop.
//

void setup() {
    // Start Gravity.
    gravity.Init();

    // Clock handlers.
    gravity.clock.AttachExtHandler(ExtClock);
    gravity.clock.AttachIntHandler(IntClock);

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

    // Check if it's time to update the display
    if (app.refresh_screen) {
        UpdateDisplay();
    }
}

//
// Firmware handlers.
//

void ExtClock() {
    if (gravity.clock.ExternalSource()) {
        gravity.clock.Tick();
        app.refresh_screen = true;
    }
}

void IntClock(uint32_t tick) {
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        const auto& channel = app.channel[i];
        auto& output = gravity.outputs[i];
        
        const uint32_t mod_pulses = clock_mod_pulses[channel.clock_mod_index];
        const uint32_t current_tick_offset = tick + channel.offset_pulses;

        // Duty cycle high check.
        if (current_tick_offset % mod_pulses == 0) {
            if (channel.probability > random(0, 100)) {
                output.High();
            }
        }

        // Duty cycle low check.
        const uint32_t duty_cycle_end_tick = tick + channel.duty_cycle_pulses + channel.offset_pulses;

        if (duty_cycle_end_tick % mod_pulses == 0) {
            output.Low();
        }
    }
}

void HandlePlayPressed() {
    gravity.clock.Pause();
    if (gravity.clock.IsPaused()) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            gravity.outputs[i].Low();
        }
    }
    app.refresh_screen = true;
}

void HandleEncoderPressed() {
    // TODO: make this more generic/dynamic

    // Main Global Settings Page.
    if (app.selected_channel == 0) {
        app.selected_param = (app.selected_param + 1) % 2;
    }
    // Selected Output Channels 1-6 Settings.
    else {
        app.selected_param = (app.selected_param + 1) % 4;
    }
    app.refresh_screen = true;
}

void HandleRotate(Direction dir, int val) {
    // Execute the behavior of the current selected parameter.

    // Main Global Settings Page.
    if (app.selected_channel == 0) {
        switch (app.selected_param) {
            case 0:
                if (gravity.clock.ExternalSource()) {
                    break;
                }
                gravity.clock.SetTempo(gravity.clock.Tempo() + val);
                app.refresh_screen = true;
                break;

            case 1:
                if (gravity.clock.ExternalSource()) {
                    gravity.clock.SetSource(SOURCE_INTERNAL);
                } else {
                    gravity.clock.SetSource(SOURCE_EXTERNAL_PPQN_24);
                }
                app.refresh_screen = true;
                break;
        }
    }
    // Selected Output Channel Settings.
    else {
        auto& ch = GetSelectedChannel();

        switch (app.selected_param) {
            case 0:
                if (dir == DIRECTION_INCREMENT && ch.clock_mod_index < MOD_CHOICE_SIZE - 1) {
                    ch.clock_mod_index += 1;
                } else if (dir == DIRECTION_DECREMENT && ch.clock_mod_index > 0) {
                    ch.clock_mod_index -= 1;
                }
                break;
            case 1:
                ch.probability = constrain(ch.probability + val, 0, 100);
                break;
            case 2:
                ch.duty_cycle = constrain(ch.duty_cycle + val, 0, 100);
                break;
            case 3:
                ch.offset = constrain(ch.offset + val, 0, 100);
                break;
        }
        uint32_t mod_pulses = clock_mod_pulses[ch.clock_mod_index];
        ch.duty_cycle_pulses = max((int)((mod_pulses * (100L - ch.duty_cycle)) / 100L), 1);
        ch.offset_pulses = (int)(mod_pulses * (100L - ch.offset) / 100L);

        app.refresh_screen = true;
    }
}

void HandlePressedRotate(Direction dir, int val) {
    if (dir == DIRECTION_INCREMENT && app.selected_channel < OUTPUT_COUNT) {
        app.selected_channel++;
    } else if (dir == DIRECTION_DECREMENT && app.selected_channel > 0) {
        app.selected_channel--;
    }
    app.selected_param = 0;
    app.refresh_screen = true;
}

//
// Helper functions.
//

Channel& GetSelectedChannel() {
    return app.channel[app.selected_channel - 1];
}

//
// UI Display functions.
//

void UpdateDisplay() {
    app.refresh_screen = false;
    gravity.display.clearDisplay();

    if (app.selected_channel == 0) {
        DisplayMainPage();
    } else {
        DisplayChannelPage();
    }

    // Show selected param indicator
    gravity.display.drawChar(0, app.selected_param * 10, 0x10, 1, 0, 1);

    // Global channel select UI.
    DisplaySelectedChannel();

    gravity.display.display();
}

void DisplaySelectedChannel() {
    gravity.display.drawLine(1, 52, 126, 52, 1);
    for (int i = 0; i < 7; i++) {
        (app.selected_channel == i)
            ? gravity.display.fillRect(i * 18, 52, 18, 12, 1)
            : gravity.display.drawLine(i * 18, 52, i * 18, 64, 1);

        int selected = app.selected_channel == i;
        if (i == 0) {
            char icon = gravity.clock.IsPaused() ? CHAR_PAUSE : CHAR_PLAY;
            gravity.display.drawChar((i * 18) + 7, 55, icon, !selected, selected, 1);
        } else {
            gravity.display.drawChar((i * 18) + 7, 55, i + 48, !selected, selected, 1);
        }
    }
    gravity.display.drawLine(126, 52, 126, 64, 1);
}

void DisplayMainPage() {
    gravity.display.setCursor(10, 0);
    gravity.display.print(F("Tempo: "));
    gravity.display.print(gravity.clock.Tempo());

    gravity.display.setCursor(10, 10);
    gravity.display.print(F("Source: "));
    gravity.display.print((gravity.clock.InternalSource()) ? F("INT") : F("EXT"));
}

void DisplayChannelPage() {
    auto& ch = GetSelectedChannel();

    gravity.display.setCursor(10, 0);
    gravity.display.print(F("Mod: "));
    if (clock_mod[ch.clock_mod_index] > 1) {
        gravity.display.print(F("/ "));
        gravity.display.print(clock_mod[ch.clock_mod_index]);
    } else {
        gravity.display.print(F("X "));
        gravity.display.print(abs(clock_mod[ch.clock_mod_index]));
    }

    gravity.display.setCursor(10, 10);
    gravity.display.print(F("Probability: "));
    gravity.display.print(ch.probability);
    gravity.display.print(F("%"));

    gravity.display.setCursor(10, 20);
    gravity.display.print(F("Duty Cycle: "));
    gravity.display.print(ch.duty_cycle);
    gravity.display.print(F("%"));

    gravity.display.setCursor(10, 30);
    gravity.display.print(F("Offset: "));
    gravity.display.print(ch.offset);
    gravity.display.print(F("%"));
}
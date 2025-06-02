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
 * BTN2: Reset all clocks.
 *
 */

#include "gravity.h"

// Firmware state variables.
struct Channel {
    byte clock_mod_index = 7;  // x1
    byte probability = 100;
    byte duty_cycle = 50;
    int duty_cycle_pulses = 48;  // 120 x1 96 PPQN
    byte offset = 0;
    int offset_pulses = 0;
};
struct AppState {
    bool refresh_screen = true;
    byte selected_param = 0;
    byte selected_channel = 0;  // 0=tempo, 1-6=output channel
    Source selected_source = SOURCE_INTERNAL;
    Channel channel[OUTPUT_COUNT];
};
AppState app;

// The number of clock mod options, hepls validate choices and pulses arrays are the same size.
const int MOD_CHOICE_SIZE = 21;
// Negative for multiply, positive for divide.
const int clock_mod[MOD_CHOICE_SIZE] = {-24, -12, -8, -6, -4, -3, -2, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32, 64, 128};

// This represents the number of clock pulses for a 96 PPQN clock source that match the above div/mult mods.
const int clock_mod_pulses[MOD_CHOICE_SIZE] = {4, 8, 12, 16, 24, 32, 48, 96, 192, 288, 384, 480, 576, 1152, 672, 768, 1536, 2304, 3072, 6144, 12288};

const auto TEXT_FONT = u8g2_font_missingplanet_tr;
const auto LARGE_FONT = u8g2_font_maniac_tr;

#define play_icon_width 14
#define play_icon_height 14
static const unsigned char play_icon[] = {
    0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x7C, 0x00, 0xFC, 0x00, 0xFC, 0x03,
    0xFC, 0x0F, 0xFC, 0x0F, 0xFC, 0x03, 0xFC, 0x00, 0x7C, 0x00, 0x3C, 0x00,
    0x00, 0x00, 0x00, 0x00};
static const unsigned char pause_icon[] = {
    0x00, 0x00, 0x00, 0x00, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E,
    0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E,
    0x38, 0x0E, 0x00, 0x00};

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
    gravity.shift_button.AttachPressHandler(HandleShiftPressed);
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

void HandleShiftPressed() {
    gravity.clock.Reset();
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
                if (static_cast<Source>(app.selected_source) == 0 && val < 0) {
                    app.selected_source = static_cast<Source>(SOURCE_LAST - 1);
                } else {
                    app.selected_source = static_cast<Source>((app.selected_source + val) % SOURCE_LAST);
                }

                gravity.clock.SetSource(app.selected_source);
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
    gravity.display.firstPage();
    do {
        if (app.selected_channel == 0) {
            DisplayMainPage();
        } else {
            DisplayChannelPage();
        }
        // Global channel select UI.
        DisplaySelectedChannel();
    } while (gravity.display.nextPage());
}

void DisplaySelectedChannel() {
    int top = 50;
    int boxWidth = 18;
    int boxHeight = 14;
    gravity.display.drawHLine(1, top, 126);
    for (int i = 0; i < 7; i++) {
        gravity.display.setDrawColor(1);
        (app.selected_channel == i)
            ? gravity.display.drawBox(i * boxWidth, top, boxWidth, boxHeight)
            : gravity.display.drawVLine(i * boxWidth, top, boxHeight);

        gravity.display.setDrawColor(2);
        if (i == 0) {
            gravity.display.setDrawColor(2);
            gravity.display.setBitmapMode(1);
            auto icon = gravity.clock.IsPaused() ? pause_icon : play_icon;
            gravity.display.drawXBM(2, top, play_icon_width, play_icon_height, icon);
        } else {
            gravity.display.setFont(TEXT_FONT);
            gravity.display.setCursor((i * boxWidth) + 7, 63);
            gravity.display.print(i);
        }
    }
    gravity.display.drawVLine(126, top, boxHeight);
}

void DisplayMainPage() {
    gravity.display.setFontMode(1);
    gravity.display.setDrawColor(2);
    gravity.display.setFont(TEXT_FONT);

    int textWidth;
    int textY = 26;
    int subTextY = 42;

    // Display selected editable value.
    if (app.selected_param == 0) {
        gravity.display.setFont(LARGE_FONT);
        char num_str[3];
        sprintf(num_str, "%d", gravity.clock.Tempo());
        textWidth = gravity.display.getUTF8Width(num_str);
        gravity.display.drawStr(32 - (textWidth / 2), textY, num_str);
        gravity.display.setFont(TEXT_FONT);
        textWidth = gravity.display.getUTF8Width("BPM");
        gravity.display.drawStr(32 - (textWidth / 2), subTextY, "BPM");
    } else if (app.selected_param == 1) {
        switch (app.selected_source) {
            case SOURCE_INTERNAL:
                gravity.display.setFont(LARGE_FONT);
                textWidth = gravity.display.getUTF8Width("INT");
                gravity.display.drawStr(32 - (textWidth / 2), textY, "INT");
                gravity.display.setFont(TEXT_FONT);
                textWidth = gravity.display.getUTF8Width("Clock");
                gravity.display.drawStr(32 - (textWidth / 2), subTextY, "Clock");
                break;
            case SOURCE_EXTERNAL_PPQN_24:
                gravity.display.setFont(LARGE_FONT);
                textWidth = gravity.display.getUTF8Width("EXT");
                gravity.display.drawStr(32 - (textWidth / 2), textY, "EXT");
                gravity.display.setFont(TEXT_FONT);
                textWidth = gravity.display.getUTF8Width("24 PPQN");
                gravity.display.drawStr(32 - (textWidth / 2), subTextY, "24 PPQN");
                break;
            case SOURCE_EXTERNAL_PPQN_4:
                gravity.display.setFont(LARGE_FONT);
                textWidth = gravity.display.getUTF8Width("EXT");
                gravity.display.drawStr(32 - (textWidth / 2), textY, "EXT");
                gravity.display.setFont(TEXT_FONT);
                textWidth = gravity.display.getUTF8Width("4 PPQN");
                gravity.display.drawStr(32 - (textWidth / 2), subTextY, "4 PPQN");
                break;
            case SOURCE_EXTERNAL_MIDI:
                gravity.display.setFont(LARGE_FONT);
                textWidth = gravity.display.getUTF8Width("EXT");
                gravity.display.drawStr(32 - (textWidth / 2), textY, "EXT");
                gravity.display.setFont(TEXT_FONT);
                textWidth = gravity.display.getUTF8Width("MIDI");
                gravity.display.drawStr(32 - (textWidth / 2), subTextY, "MIDI");
                break;
        }
    }

    int idx;
    int drawX;
    int height = 14;
    int padding = 4;

    // Draw selected menu item box.
    gravity.display.drawBox(65, (height * app.selected_param) + 2, 63, height + 1);

    // Draw each menu item.
    textWidth = gravity.display.getUTF8Width("Tempo");
    drawX = (SCREEN_WIDTH - textWidth) - padding;
    gravity.display.drawStr(drawX, height * ++idx, "Tempo");

    textWidth = gravity.display.getUTF8Width("Source");
    drawX = (SCREEN_WIDTH - textWidth) - padding;
    gravity.display.drawStr(drawX, height * ++idx, "Source");
}

void DisplayChannelPage() {
    auto& ch = GetSelectedChannel();

    gravity.display.setFontMode(1);
    gravity.display.setDrawColor(2);
    gravity.display.setFont(LARGE_FONT);
    
    int textWidth;
    int textY = 26;
    int subTextY = 42;
    char num_str[4];

    // Display selected editable value.
    switch (app.selected_param) {
        case 0:  // Clock Mod
            char mod_str[4];
            if (clock_mod[ch.clock_mod_index] > 1) {
                sprintf(mod_str, "/%d", clock_mod[ch.clock_mod_index]);
                textWidth = gravity.display.getUTF8Width(mod_str);
                gravity.display.drawStr(32 - (textWidth / 2), textY, mod_str);
                gravity.display.setFont(TEXT_FONT);
                textWidth = gravity.display.getUTF8Width("Divide");
                gravity.display.drawStr(32 - (textWidth / 2), subTextY, "Divide");

            } else {
                sprintf(mod_str, "x%d", abs(clock_mod[ch.clock_mod_index]));
                textWidth = gravity.display.getUTF8Width(mod_str);
                gravity.display.drawStr(32 - (textWidth / 2), textY, mod_str);
                gravity.display.setFont(TEXT_FONT);
                textWidth = gravity.display.getUTF8Width("Multiply");
                gravity.display.drawStr(32 - (textWidth / 2), subTextY, "Multiply");
            }
            break;
        case 1:  // Probability
            sprintf(num_str, "%d%%", ch.probability);
            textWidth = gravity.display.getUTF8Width(num_str);
            gravity.display.drawStr(32 - (textWidth / 2), textY, num_str);
            gravity.display.setFont(TEXT_FONT);
            textWidth = gravity.display.getUTF8Width("Hit Chance");
            gravity.display.drawStr(32 - (textWidth / 2), subTextY, "Hit Chance");
            break;
        case 2:  // Duty Cycle
            sprintf(num_str, "%d%%", ch.duty_cycle);
            textWidth = gravity.display.getUTF8Width(num_str);
            gravity.display.drawStr(32 - (textWidth / 2), textY, num_str);
            gravity.display.setFont(TEXT_FONT);
            textWidth = gravity.display.getUTF8Width("Pulse Width");
            gravity.display.drawStr(32 - (textWidth / 2), subTextY, "Pulse Width");
            break;
        case 3:  // Offset
            sprintf(num_str, "%d%%", ch.offset);
            textWidth = gravity.display.getUTF8Width(num_str);
            gravity.display.drawStr(32 - (textWidth / 2), textY, num_str);
            gravity.display.setFont(TEXT_FONT);
            textWidth = gravity.display.getUTF8Width("Shift Hit");
            gravity.display.drawStr(32 - (textWidth / 2), subTextY, "Shift Hit");
            break;
    }

    int idx = 0;
    int drawX;
    int height = 14;
    int padding = 4;

    gravity.display.setFont(TEXT_FONT);
    int textHeight = gravity.display.getFontAscent();

    // Draw selected menu item box.
    gravity.display.drawBox(65, (height * min(2, app.selected_param)) + 2, 63, height + 1);

    // Draw each menu item.
    if (app.selected_param < 3) {
        textWidth = gravity.display.getUTF8Width("Mod");
        drawX = (SCREEN_WIDTH - textWidth) - padding;
        gravity.display.drawStr(drawX, (height * ++idx), "Mod");
    }

    textWidth = gravity.display.getUTF8Width("Probability");
    drawX = (SCREEN_WIDTH - textWidth) - padding;
    gravity.display.drawStr(drawX, (height * ++idx), "Probability");

    textWidth = gravity.display.getUTF8Width("Duty Cycle");
    drawX = (SCREEN_WIDTH - textWidth) - padding;
    gravity.display.drawStr(drawX, (height * ++idx), "Duty Cycle");

    if (app.selected_param > 2) {
        textWidth = gravity.display.getUTF8Width("Offset");
        drawX = (SCREEN_WIDTH - textWidth) - padding;
        gravity.display.drawStr(drawX, (height * ++idx), "Offset");
    }
}
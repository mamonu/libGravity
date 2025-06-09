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
 * BTN2: Stop all clocks.
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
    bool editing_param = false;
    byte selected_param = 0;
    byte selected_channel = 0;  // 0=tempo, 1-6=output channel
    Source selected_source = SOURCE_INTERNAL;
    Channel channel[OUTPUT_COUNT];
};
AppState app;

enum ParamsMainPage {
    PARAM_MAIN_TEMPO,
    PARAM_MAIN_SOURCE,
    PARAM_MAIN_LAST,
};
enum ParamsChannelPage {
    PARAM_CH_MOD,
    PARAM_CH_PROB,
    PARAM_CH_DUTY,
    PARAM_CH_OFFSET,
    PARAM_CH_LAST,
};

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

    // Check if it's time to update the display
    if (app.refresh_screen) {
        UpdateDisplay();
    }
}

//
// Firmware handlers for clocks.
//

void HandleIntClockTick(uint32_t tick) {
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        auto& channel = app.channel[i];
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
    app.editing_param = !app.editing_param;
    app.refresh_screen = true;
}

void HandleRotate(Direction dir, int val) {
    // Select a prameter when not in edit mode.
    if (!app.editing_param) {
        // Main Global Settings Page.
        if (app.selected_channel == 0) {
            if (app.selected_param == 0 && val < 0) {
                app.selected_param = PARAM_MAIN_LAST - 1;
            } else {
                app.selected_param = (app.selected_param + val) % PARAM_MAIN_LAST;
            }
        }
        // Selected Output Channels 1-6 Settings.
        else {
            if (app.selected_param == 0 && val < 0) {
                app.selected_param = PARAM_CH_LAST - 1;
            } else {
                app.selected_param = (app.selected_param + val) % PARAM_CH_LAST;
            }
        }
    }
    // Edit selected param.
    else {
        // Main Global Settings Page.
        if (app.selected_channel == 0) {
            switch (static_cast<ParamsMainPage>(app.selected_param)) {
                case PARAM_MAIN_TEMPO:
                    if (gravity.clock.ExternalSource()) {
                        break;
                    }
                    gravity.clock.SetTempo(gravity.clock.Tempo() + val);
                    app.refresh_screen = true;
                    break;

                case PARAM_MAIN_SOURCE:
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

            switch (static_cast<ParamsChannelPage>(app.selected_param)) {
                case PARAM_CH_MOD:
                    if (dir == DIRECTION_INCREMENT && ch.clock_mod_index < MOD_CHOICE_SIZE - 1) {
                        ch.clock_mod_index += 1;
                    } else if (dir == DIRECTION_DECREMENT && ch.clock_mod_index > 0) {
                        ch.clock_mod_index -= 1;
                    }
                    break;
                case PARAM_CH_PROB:
                    ch.probability = constrain(ch.probability + val, 0, 100);
                    break;
                case PARAM_CH_DUTY:
                    ch.duty_cycle = constrain(ch.duty_cycle + val, 0, 99);
                    break;
                case PARAM_CH_OFFSET:
                    ch.offset = constrain(ch.offset + val, 0, 99);
                    break;
            }
            uint32_t mod_pulses = clock_mod_pulses[ch.clock_mod_index];
            ch.duty_cycle_pulses = max((int)((mod_pulses * (100L - ch.duty_cycle)) / 100L), 1);
            ch.offset_pulses = (int)(mod_pulses * (100L - ch.offset) / 100L);
        }
    }
    app.refresh_screen = true;
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

void ResetOutputs() {
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        gravity.outputs[i].Low();
    }
}

//
// UI Display functions.
//

// Constants for screen layout and fonts
constexpr int SCREEN_CENTER_X = 32;
constexpr int MAIN_TEXT_Y = 26;
constexpr int SUB_TEXT_Y = 42;
constexpr int MENU_ITEM_HEIGHT = 14;
constexpr int MENU_BOX_PADDING = 4;
constexpr int MENU_BOX_WIDTH = 64;
constexpr int VISIBLE_MENU_ITEMS = 3;
constexpr int CHANNEL_BOXES_Y = 50;
constexpr int CHANNEL_BOX_WIDTH = 18;
constexpr int CHANNEL_BOX_HEIGHT = 14;

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

void DisplayMainPage() {
    gravity.display.setFontMode(1);
    gravity.display.setDrawColor(2);
    gravity.display.setFont(TEXT_FONT);

    // Display selected editable value
    char mainText[8];
    const char* subText;

    if (app.selected_param == 0) {
        // Serial MIID is too unstable to display bpm in real time.
        if (app.selected_source == SOURCE_EXTERNAL_MIDI) {
            sprintf(mainText, "%s", "EXT");
        } else {
            sprintf(mainText, "%d", gravity.clock.Tempo());
        }
        subText = "BPM";
    } else if (app.selected_param == 1) {
        switch (app.selected_source) {
            case SOURCE_INTERNAL:
                sprintf(mainText, "%s", "INT");
                subText = "Clock";
                break;
            case SOURCE_EXTERNAL_PPQN_24:
                sprintf(mainText, "%s", "EXT");
                subText = "24 PPQN";
                break;
            case SOURCE_EXTERNAL_PPQN_4:
                sprintf(mainText, "%s", "EXT");
                subText = "4 PPQN";
                break;
            case SOURCE_EXTERNAL_MIDI:
                sprintf(mainText, "%s", "EXT");
                subText = "MIDI";
                break;
        }
    }

    drawCenteredText(mainText, MAIN_TEXT_Y, LARGE_FONT);
    drawCenteredText(subText, SUB_TEXT_Y, TEXT_FONT);

    // Draw Main Page menu items
    const char* menu_items[PARAM_MAIN_LAST] = {"Tempo", "Source"};
    drawMenuItems(menu_items);
}

void DisplayChannelPage() {
    auto& ch = GetSelectedChannel();

    gravity.display.setFontMode(1);
    gravity.display.setDrawColor(2);

    // Display selected editable value
    char mainText[5];
    const char* subText;

    switch (app.selected_param) {
        case 0: {  // Clock Mod
            int mod_value = clock_mod[ch.clock_mod_index];
            if (mod_value > 1) {
                sprintf(mainText, "/%d", mod_value);
                subText = "Divide";
            } else {
                sprintf(mainText, "x%d", abs(mod_value));
                subText = "Multiply";
            }
            break;
        }
        case 1:  // Probability
            sprintf(mainText, "%d%%", ch.probability);
            subText = "Hit Chance";
            break;
        case 2:  // Duty Cycle
            sprintf(mainText, "%d%%", ch.duty_cycle);
            subText = "Pulse Width";
            break;
        case 3:  // Offset
            sprintf(mainText, "%d%%", ch.offset);
            subText = "Shift Hit";
            break;
    }

    drawCenteredText(mainText, MAIN_TEXT_Y, LARGE_FONT);
    drawCenteredText(subText, SUB_TEXT_Y, TEXT_FONT);

    // Draw Channel Page menu items
    const char* menu_items[PARAM_CH_LAST] = {
        "Mod", "Probability", "Duty Cycle", "Offset"};
    drawMenuItems(menu_items);
}

void DisplaySelectedChannel() {
    int boxX = CHANNEL_BOX_WIDTH;
    int boxY = CHANNEL_BOXES_Y;
    int boxWidth = CHANNEL_BOX_WIDTH;
    int boxHeight = CHANNEL_BOX_HEIGHT;
    int textOffset = 7;  // Half of font width

    // Draw top and right side of frame.
    gravity.display.drawHLine(1, boxY, SCREEN_WIDTH - 2);
    gravity.display.drawVLine(SCREEN_WIDTH - 2, boxY, boxHeight);

    for (int i = 0; i < OUTPUT_COUNT + 1; i++) {
        // Draw box frame or filled selected box.
        gravity.display.setDrawColor(1);
        (app.selected_channel == i)
            ? gravity.display.drawBox(i * boxWidth, boxY, boxWidth, boxHeight)
            : gravity.display.drawVLine(i * boxWidth, boxY, boxHeight);

        // Draw clock status icon or each channel number.
        gravity.display.setDrawColor(2);
        if (i == 0) {
            gravity.display.setBitmapMode(1);
            auto icon = gravity.clock.IsPaused() ? pause_icon : play_icon;
            gravity.display.drawXBM(2, boxY, play_icon_width, play_icon_height, icon);
        } else {
            gravity.display.setFont(TEXT_FONT);
            gravity.display.setCursor((i * boxWidth) + textOffset, SCREEN_HEIGHT - 1);
            gravity.display.print(i);
        }
    }
}

void drawMenuItems(const char* menu_items[]) {
    // Draw menu items
    gravity.display.setFont(TEXT_FONT);

    // Draw selected menu item box
    int selectedBoxY = 0;
    if (app.selected_param == PARAM_CH_LAST - 1) {
        selectedBoxY = MENU_ITEM_HEIGHT * min(2, app.selected_param);
    } else if (app.selected_param > 0) {
        selectedBoxY = MENU_ITEM_HEIGHT;
    }

    int boxX = MENU_BOX_WIDTH + 1;
    int boxY = selectedBoxY + 2;
    int boxWidth = MENU_BOX_WIDTH - 1;
    int boxHeight = MENU_ITEM_HEIGHT + 1;

    app.editing_param
        ? gravity.display.drawBox(boxX, boxY, boxWidth, boxHeight)
        : gravity.display.drawFrame(boxX, boxY, boxWidth, boxHeight);

    // Draw the visible menu items
    int start_index = 0;
    if (app.selected_param == PARAM_CH_LAST - 1) {
        start_index = PARAM_CH_LAST - VISIBLE_MENU_ITEMS;
    } else if (app.selected_param > 0) {
        start_index = app.selected_param - 1;
    }

    for (int i = 0; i < VISIBLE_MENU_ITEMS; ++i) {
        int idx = start_index + i;
        drawRightAlignedText(menu_items[idx], MENU_ITEM_HEIGHT * (i + 1));
    }
}

// Helper function to draw centered text
void drawCenteredText(const char* text, int y, const uint8_t* font) {
    gravity.display.setFont(font);
    int textWidth = gravity.display.getUTF8Width(text);
    gravity.display.drawStr(SCREEN_CENTER_X - (textWidth / 2), y, text);
}

// Helper function to draw right-aligned text
void drawRightAlignedText(const char* text, int y) {
    int textWidth = gravity.display.getUTF8Width(text);
    int drawX = (SCREEN_WIDTH - textWidth) - MENU_BOX_PADDING;
    gravity.display.drawStr(drawX, y, text);
}
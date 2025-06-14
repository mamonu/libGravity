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

#include <gravity.h>

#include "channel.h"

// Firmware state variables.
struct AppState {
    bool refresh_screen = true;
    bool editing_param = false;
    int selected_param = 0;
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
    PARAM_CH_CV_SRC,
    PARAM_CH_CV_DEST,
    PARAM_CH_LAST,
};

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

    // Read CVs and call the update function for each channel.
    int cv1 = gravity.cv1.Read();
    int cv2 = gravity.cv2.Read();
    for (int i = 0; i < OUTPUT_COUNT; i++) {
        app.channel[i].applyCvMod(cv1, cv2);
    }

    if (app.refresh_screen) {
        UpdateDisplay();
    }
}

//
// Firmware handlers for clocks.
//

void HandleIntClockTick(uint32_t tick) {
    bool refresh = false;
    for (int i = 0; i < OUTPUT_COUNT; i++) {
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
    if (dir == DIRECTION_INCREMENT && app.selected_channel < OUTPUT_COUNT) {
        app.selected_channel++;
    } else if (dir == DIRECTION_DECREMENT && app.selected_channel > 0) {
        app.selected_channel--;
    }
    app.selected_param = 0;
    app.refresh_screen = true;
}

void editMainParameter(int val) {
    switch (static_cast<ParamsMainPage>(app.selected_param)) {
        case PARAM_MAIN_TEMPO:
            if (gravity.clock.ExternalSource()) {
                break;
            }
            gravity.clock.SetTempo(gravity.clock.Tempo() + val);
            break;

        case PARAM_MAIN_SOURCE: {
            int source = static_cast<int>(app.selected_source);
            updateSelection(source, val, SOURCE_LAST);
            app.selected_source = static_cast<Source>(source);
            gravity.clock.SetSource(app.selected_source);
            break;
        }
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
constexpr int VISIBLE_MENU_ITEMS = 3;
constexpr int MENU_ITEM_HEIGHT = 14;
constexpr int MENU_BOX_PADDING = 4;
constexpr int MENU_BOX_WIDTH = 64;
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

    switch (app.selected_param) {
        case PARAM_MAIN_TEMPO:
            // Serial MIDI is too unstable to display bpm in real time.
            if (app.selected_source == SOURCE_EXTERNAL_MIDI) {
                sprintf(mainText, "%s", "EXT");
            } else {
                sprintf(mainText, "%d", gravity.clock.Tempo());
            }
            subText = "BPM";
            break;
        case PARAM_MAIN_SOURCE:
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
    drawMenuItems(menu_items, PARAM_MAIN_LAST);
}

void DisplayChannelPage() {
    auto& ch = GetSelectedChannel();

    gravity.display.setFontMode(1);
    gravity.display.setDrawColor(2);

    // Display selected editable value
    char mainText[5];
    const char* subText;

    // When editing a param, just show the base value. When not editing show
    // the value with cv mod.
    bool withCvMod = !app.editing_param;

    switch (app.selected_param) {
        case PARAM_CH_MOD: {
            int mod_value = ch.getClockMod(withCvMod);
            if (mod_value > 1) {
                sprintf(mainText, "/%d", mod_value);
                subText = "Divide";
            } else {
                sprintf(mainText, "x%d", abs(mod_value));
                subText = "Multiply";
            }
            break;
        }
        case PARAM_CH_PROB:
            sprintf(mainText, "%d%%", ch.getProbability(withCvMod));
            subText = "Hit Chance";
            break;
        case PARAM_CH_DUTY:
            sprintf(mainText, "%d%%", ch.getDutyCycle(withCvMod));
            subText = "Pulse Width";
            break;
        case PARAM_CH_OFFSET:
            sprintf(mainText, "%d%%", ch.getOffset(withCvMod));
            subText = "Shift Hit";
            break;
        case PARAM_CH_CV_SRC: {
            switch (ch.getCvSource()) {
                case CV_NONE:
                    sprintf(mainText, "SRC");
                    subText = "None";
                    break;
                case CV_1:
                    sprintf(mainText, "SRC");
                    subText = "CV 1";
                    break;
                case CV_2:
                    sprintf(mainText, "SRC");
                    subText = "CV 2";
                    break;
            }
            break;
        }
        case PARAM_CH_CV_DEST: {
            switch (ch.getCvDestination()) {
                case CV_DEST_NONE:
                    sprintf(mainText, "DEST");
                    subText = "None";
                    break;
                case CV_DEST_MOD:
                    sprintf(mainText, "DEST");
                    subText = "Clock Mod";
                    break;
                case CV_DEST_PROB:
                    sprintf(mainText, "DEST");
                    subText = "Probability";
                    break;
                case CV_DEST_DUTY:
                    sprintf(mainText, "DEST");
                    subText = "Duty Cycle";
                    break;
                case CV_DEST_OFFSET:
                    sprintf(mainText, "DEST");
                    subText = "Offset";
                    break;
            }
            break;
        }
    }

    drawCenteredText(mainText, MAIN_TEXT_Y, LARGE_FONT);
    drawCenteredText(subText, SUB_TEXT_Y, TEXT_FONT);

    // Draw Channel Page menu items
    const char* menu_items[PARAM_CH_LAST] = {
        "Mod", "Probability", "Duty", "Offset", "CV Source", "CV Dest"};
    drawMenuItems(menu_items, PARAM_CH_LAST);
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

void drawMenuItems(const char* menu_items[], int menu_size) {
    // Draw menu items
    gravity.display.setFont(TEXT_FONT);

    // Draw selected menu item box
    int selectedBoxY = 0;
    if (menu_size >= VISIBLE_MENU_ITEMS && app.selected_param == menu_size - 1) {
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
    if (menu_size >= VISIBLE_MENU_ITEMS && app.selected_param == menu_size - 1) {
        start_index = menu_size - VISIBLE_MENU_ITEMS;
    } else if (app.selected_param > 0) {
        start_index = app.selected_param - 1;
    }

    for (int i = 0; i < min(menu_size, VISIBLE_MENU_ITEMS); ++i) {
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
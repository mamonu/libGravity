#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

#include "app_state.h"

//
// UI Display functions for drawing the UI to the OLED display.
//

/*
 * Font: velvetscreen.bdf 9pt
 * https://stncrn.github.io/u8g2-unifont-helper/
 * "%/0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
 */
const uint8_t TEXT_FONT[437] U8G2_FONT_SECTION("velvetscreen") PROGMEM =
    "\64\0\2\2\3\3\2\3\4\5\5\0\0\5\0\5\0\0\221\0\0\1\230 \4\200\134%\11\255tT"
    "R\271RI(\6\252\334T\31)\7\252\134bJ\12+\7\233\345\322J\0,\5\221T\4-\5\213"
    "f\6.\5\211T\2/\6\244\354c\33\60\10\254\354T\64\223\2\61\7\353\354\222\254\6\62\11\254l"
    "\66J*\217\0\63\11\254l\66J\32\215\4\64\10\254l\242\34\272\0\65\11\254l\206\336h$\0\66"
    "\11\254\354T^\61)\0\67\10\254lF\216u\4\70\11\254\354TL*&\5\71\11\254\354TL;"
    ")\0:\6\231UR\0A\10\254\354T\34S\6B\11\254lV\34)\216\4C\11\254\354T\324\61"
    ")\0D\10\254lV\64G\2E\10\254l\206\36z\4F\10\254l\206^\71\3G\11\254\354TN"
    "\63)\0H\10\254l\242\34S\6I\6\251T\206\0J\10\254\354k\231\24\0K\11\254l\242J\62"
    "\225\1L\7\254lr{\4M\11\255t\362ZI\353\0N\11\255t\362TI\356\0O\10\254\354T"
    "\64\223\2P\11\254lV\34)g\0Q\10\254\354T\264b\12R\10\254lV\34\251\31S\11\254\354"
    "FF\32\215\4T\7\253dVl\1U\10\254l\242\63)\0V\11\255t\262Ne\312\21W\12\255"
    "t\262J*\251.\0X\11\254l\242L*\312\0Y\12\255tr\252\63\312(\2Z\7\253df*"
    "\7p\10\255\364V\266\323\2q\7\255\364\216\257\5r\10\253d\242\32*\2t\6\255t\376#w\11"
    "\255\364V\245FN\13x\6\233dR\7\0\0\0\4\377\377\0";

/*
 * Font: STK-L.bdf 36pt
 * https://stncrn.github.io/u8g2-unifont-helper/
 * "%/0123456789ACDEFINORSTUVXx"
 */
const uint8_t LARGE_FONT[715] U8G2_FONT_SECTION("stk-l") =
    "\33\0\4\4\4\5\2\1\6\20\30\0\0\27\0\0\0\1\77\0\0\2\256%'\17\37\313\330R#&"
    "\32!F\14\211I\310\24!\65\204(MF\21)Cd\304\10\62b\14\215\60Vb\334\20\0/\14"
    "\272\336\336d\244\350\263q\343\0\60\37|\377\216!%*\10\35\263\253ChD\30\21bB\14\242S"
    "\306lv\210\204\22Ef\0\61\24z\337\322\60R\205\314\234\31\61F\310\270\371\177\224\42\3\62\33|"
    "\377\216)\64*\10\35\63\66r\206\304\314`c\252\34\301\221\263|\360\300\0\63\34|\377\216)\64*"
    "\10\35\63\66r \71\332YIr\226\306\16\221P\203\312\14\0\64 |\377\226\220AC\306\20\31B"
    "f\310\240\21\204F\214\32\61j\304(cv\366\200\305\312\371\0\65\32|\377\206\212-F\316\27\204\224"
    "\254\30\65t\344,\215\35\42\241\6\225\31\0\66\33}\17\317\251\64+\206\235\63:/\314,aA\352"
    "\234\335\235\42\261&\325\31\0\67\23|\377\302\212\7)\347Crt\70\345\300\221\363\16\0\70 |\377"
    "\216)\64*\10\35\263\354\20\11\42d\20\235BC\204\4\241cvv\210\204\32Tf\0\71\32|\377"
    "\216)\64*\10\35\263\263C$\226\250I\71_\14\42\241\6\225\31\0A\26}\17S\271Si(\31"
    "\65d\324\210q\366\356\301w\366\273\1C\27}\17\317\251\64K\10!\63:\377\247\304F\20\42\261F"
    "\21\22\0D\33}\17C\42\65KF\15\31\66b\330\210q\366\77;\66b\24\211%j\22\1E\21"
    "|\377\302\7)\347%\42\214F\316/\37<\60F\20|\377\302\7)\347\313\64\331\214\234\177\11\0I"
    "\7so\302\37$N#}\17\203@s\346\216\35C\205*Q\42\23cL\214\61\62\304\310\20\63#"
    "\314\214\60\224\25f\327\231\33O\26}\17\317\251\64KF\215\30g\377\337\215\30\65dM\252\63\0R"
    "\61\216\37\203\242\65L\206\221\30\67b\334\210q#\306\215\30\67b\30\211QD\230(J\65d\330\230"
    "Qc\10\315j\314(\42\303H\214\33\61\356\340\0S\42\216\37\317\261DKH\221\30\67b\334\210\261"
    "c)M\226-\331\301c\307\32\64\207\212D\223Uh\0T\15}\17\303\7\251\206\316\377\377\12\0U"
    "\21|\377\302\60\373\377\317F\14\32\242\6\225\31\0V\26\177\375\302H\373\377\345\210qCH\221\241\212"
    "\4\271\223e\207\1X)~\37\303@\203\307H\14\33B\210\14\21RC\206\241\63h\222(I\203\346"
    "\220\15\31E\204\14!\42\303F\20;h\341\0x\24\312\336\302 CGH\240\61E\312\14\222)\6"
    "Y\64\0\0\0\0\4\377\377\0";

#define play_icon_width 14
#define play_icon_height 14
static const unsigned char play_icon[28] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x7C, 0x00, 0xFC, 0x00, 0xFC, 0x03,
    0xFC, 0x0F, 0xFC, 0x0F, 0xFC, 0x03, 0xFC, 0x00, 0x7C, 0x00, 0x3C, 0x00,
    0x00, 0x00, 0x00, 0x00};
static const unsigned char pause_icon[28] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E,
    0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E, 0x38, 0x0E,
    0x38, 0x0E, 0x00, 0x00};

// Constants for screen layout and fonts
constexpr uint8_t SCREEN_CENTER_X = 32;
constexpr uint8_t MAIN_TEXT_Y = 26;
constexpr uint8_t SUB_TEXT_Y = 40;
constexpr uint8_t VISIBLE_MENU_ITEMS = 3;
constexpr uint8_t MENU_ITEM_HEIGHT = 14;
constexpr uint8_t MENU_BOX_PADDING = 4;
constexpr uint8_t MENU_BOX_WIDTH = 64;
constexpr uint8_t CHANNEL_BOXES_Y = 50;
constexpr uint8_t CHANNEL_BOX_WIDTH = 18;
constexpr uint8_t CHANNEL_BOX_HEIGHT = 14;

// Helper function to draw centered text
void drawCenteredText(const char* text, int y, const uint8_t* font) {
    gravity.display.setFont(font);
    int textWidth = gravity.display.getStrWidth(text);
    gravity.display.drawStr(SCREEN_CENTER_X - (textWidth / 2), y, text);
}

// Helper function to draw right-aligned text
void drawRightAlignedText(const char* text, int y) {
    int textWidth = gravity.display.getStrWidth(text);
    int drawX = (SCREEN_WIDTH - textWidth) - MENU_BOX_PADDING;
    gravity.display.drawStr(drawX, y, text);
}

void drawMainSelection() {
    gravity.display.setDrawColor(1);
    const int tickSize = 3;
    const int mainWidth = SCREEN_WIDTH / 2;
    const int mainHeight = 49;
    gravity.display.drawLine(0, 0, tickSize, 0);
    gravity.display.drawLine(0, 0, 0, tickSize);
    gravity.display.drawLine(mainWidth, 0, mainWidth - tickSize, 0);
    gravity.display.drawLine(mainWidth, 0, mainWidth, tickSize);
    gravity.display.drawLine(mainWidth, mainHeight, mainWidth, mainHeight - tickSize);
    gravity.display.drawLine(mainWidth, mainHeight, mainWidth - tickSize, mainHeight);
    gravity.display.drawLine(0, mainHeight, tickSize, mainHeight);
    gravity.display.drawLine(0, mainHeight, 0, mainHeight - tickSize);
    gravity.display.setDrawColor(2);
}

void drawMenuItems(String menu_items[], int menu_size) {
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

    if (app.editing_param) {
        gravity.display.drawBox(boxX, boxY, boxWidth, boxHeight);
        drawMainSelection();
    } else {
        gravity.display.drawFrame(boxX, boxY, boxWidth, boxHeight);
    }

    // Draw the visible menu items
    int start_index = 0;
    if (menu_size >= VISIBLE_MENU_ITEMS && app.selected_param == menu_size - 1) {
        start_index = menu_size - VISIBLE_MENU_ITEMS;
    } else if (app.selected_param > 0) {
        start_index = app.selected_param - 1;
    }

    for (int i = 0; i < min(menu_size, VISIBLE_MENU_ITEMS); ++i) {
        int idx = start_index + i;
        drawRightAlignedText(menu_items[idx].c_str(), MENU_ITEM_HEIGHT * (i + 1) - 1);
    }
}

// Display an indicator when swing percentage matches a musical note.
void swingDivisionMark() {
    auto& ch = GetSelectedChannel();
    switch (ch.getSwing()) {
        case 58:  // 1/32nd
        case 66:  // 1/16th
        case 75:  // 1/8th
            gravity.display.drawBox(56, 4, 4, 4);
            break;
        case 54:  // 1/32nd tripplet
        case 62:  // 1/16th tripplet
        case 71:  // 1/8th tripplet
            gravity.display.drawBox(56, 4, 4, 4);
            gravity.display.drawBox(57, 5, 2, 2);
            break;
    }
}

// Main display functions

void DisplayMainPage() {
    gravity.display.setFontMode(1);
    gravity.display.setDrawColor(2);
    gravity.display.setFont(TEXT_FONT);

    // Display selected editable value
    String mainText;
    String subText;

    switch (app.selected_param) {
        case PARAM_MAIN_TEMPO:
            // Serial MIDI is too unstable to display bpm in real time.
            if (app.selected_source == Clock::SOURCE_EXTERNAL_MIDI) {
                mainText = F("EXT");
            } else {
                mainText = String(gravity.clock.Tempo());
            }
            subText = F("BPM");
            break;
        case PARAM_MAIN_SOURCE:
            mainText = F("EXT");
            switch (app.selected_source) {
                case Clock::SOURCE_INTERNAL:
                    mainText = F("INT");
                    subText = F("CLOCK");
                    break;
                case Clock::SOURCE_EXTERNAL_PPQN_24:
                    subText = F("24 PPQN");
                    break;
                case Clock::SOURCE_EXTERNAL_PPQN_4:
                    subText = F("4 PPQN");
                    break;
                case Clock::SOURCE_EXTERNAL_MIDI:
                    subText = F("MIDI");
                    break;
            }
            break;
        case PARAM_MAIN_PULSE:
            mainText = F("OUT");
            switch (app.selected_pulse) {
                case Clock::PULSE_NONE:
                    subText = F("PULSE OFF");
                    break;
                case Clock::PULSE_PPQN_24:
                    subText = F("24 PPQN PULSE");
                    break;
                case Clock::PULSE_PPQN_4:
                    subText = F("4 PPQN PULSE");
                    break;
                case Clock::PULSE_PPQN_1:
                    subText = F("1 PPQN PULSE");
                    break;
            }
            break;
        case PARAM_MAIN_ENCODER_DIR:
            mainText = F("DIR");
            subText = app.selected_sub_param == 0 ? F("DEFAULT") : F("REVERSED");
            break;
        case PARAM_MAIN_RESET_STATE:
            mainText = F("RST");
            subText = app.selected_sub_param == 0 ? F("RESET ALL") : F("BACK");
            break;
    }

    drawCenteredText(mainText.c_str(), MAIN_TEXT_Y, LARGE_FONT);
    drawCenteredText(subText.c_str(), SUB_TEXT_Y, TEXT_FONT);

    // Draw Main Page menu items
    String menu_items[PARAM_MAIN_LAST] = {F("TEMPO"), F("SOURCE"), F("PULSE OUT"), F("ENCODER DIR"), F("RESET")};
    drawMenuItems(menu_items, PARAM_MAIN_LAST);
}

void DisplayChannelPage() {
    auto& ch = GetSelectedChannel();

    gravity.display.setFontMode(1);
    gravity.display.setDrawColor(2);

    // Display selected editable value
    String mainText;
    String subText;

    // When editing a param, just show the base value. When not editing show
    // the value with cv mod.
    bool withCvMod = !app.editing_param;

    switch (app.selected_param) {
        case PARAM_CH_MOD: {
            int mod_value = ch.getClockMod(withCvMod);
            if (mod_value > 1) {
                mainText = F("/");
                mainText += String(mod_value);
                subText = F("DIVIDE");
            } else {
                mainText = F("x");
                mainText += String(abs(mod_value));
                subText = F("MULTIPLY");
            }
            break;
        }
        case PARAM_CH_PROB:
            mainText = String(ch.getProbability(withCvMod)) + F("%");
            subText = F("HIT CHANCE");
            break;
        case PARAM_CH_DUTY:
            mainText = String(ch.getDutyCycle(withCvMod)) + F("%");
            subText = F("PULSE WIDTH");
            break;
        case PARAM_CH_OFFSET:
            mainText = String(ch.getOffset(withCvMod)) + F("%");
            subText = F("SHIFT HIT");
            break;
        case PARAM_CH_SWING:
            ch.getSwing() == 50
                ? mainText = F("OFF")
                : mainText = String(ch.getSwing(withCvMod)) + F("%");
            subText = "DOWN BEAT";
            swingDivisionMark();
            break;
        case PARAM_CH_EUC_STEPS:
            mainText = String(ch.getSteps(withCvMod));
            subText = "EUCLID STEPS";
            break;
        case PARAM_CH_EUC_HITS:
            mainText = String(ch.getHits(withCvMod));
            subText = "EUCLID HITS";
            break;
        case PARAM_CH_CV1_DEST:
        case PARAM_CH_CV2_DEST: {
            mainText = (app.selected_param == PARAM_CH_CV1_DEST) ? F("CV1") : F("CV2");
            switch ((app.selected_param == PARAM_CH_CV1_DEST) ? ch.getCv1Dest() : ch.getCv2Dest()) {
                case CV_DEST_NONE:
                    subText = F("NONE");
                    break;
                case CV_DEST_MOD:
                    subText = F("CLOCK MOD");
                    break;
                case CV_DEST_PROB:
                    subText = F("PROBABILITY");
                    break;
                case CV_DEST_DUTY:
                    subText = F("DUTY CYCLE");
                    break;
                case CV_DEST_OFFSET:
                    subText = F("OFFSET");
                    break;
                case CV_DEST_SWING:
                    subText = F("SWING");
                    break;
                case CV_DEST_EUC_STEPS:
                    subText = F("EUCLID STEPS");
                    break;
                case CV_DEST_EUC_HITS:
                    subText = F("EUCLID HITS");
                    break;
            }
            break;
        }
    }

    drawCenteredText(mainText.c_str(), MAIN_TEXT_Y, LARGE_FONT);
    drawCenteredText(subText.c_str(), SUB_TEXT_Y, TEXT_FONT);

    // Draw Channel Page menu items
    String menu_items[PARAM_CH_LAST] = {
        F("MOD"), F("PROBABILITY"), F("DUTY"), F("OFFSET"), F("SWING"), F("EUCLID STEPS"),
        F("EUCLID HITS"), F("CV1 MOD"), F("CV2 MOD")};
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

    for (int i = 0; i < Gravity::OUTPUT_COUNT + 1; i++) {
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
            gravity.display.drawXBMP(2, boxY, play_icon_width, play_icon_height, icon);
        } else {
            gravity.display.setFont(TEXT_FONT);
            gravity.display.setCursor((i * boxWidth) + textOffset, SCREEN_HEIGHT - 3);
            gravity.display.print(i);
        }
    }
}

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

#endif  // DISPLAY_H

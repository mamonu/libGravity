/**
 * Analog Input Calibration Script
 *
 * Provide each CV input with a constant voltage of -5v, 0v, and 5v. For
 * each config point, provide the appropriate voltage value and then adjust
 * the encoder until you have the correct calibration value set.
 *
 * With the arrow on the left side of the bar, provide a -5v signal and adjust
 * the encoder until the display shows a matching -5v reading.
 *
 * With the arrow in the center of the bar, provide a 0v signal and adjust the
 * encoder until the display shows a matching 0v reading.
 *
 * With the arrow on the right side of the bar, provide a 5v signal and adjust
 * the encoder until the display shows a matching 5v reading.
 *
 * TODO: Store the calibration value in EEPROM.
 */

#include "libGravity.h"

#define TEXT_FONT u8g2_font_profont11_tf
#define INDICATOR_FONT u8g2_font_open_iconic_arrow_1x_t
#define U8G2_UP_ARROW_GLYPH 0x43 // caret-up '˄'

byte selected_param = 0;

// Initialize the gravity library and attach your handlers in the setup method.
void setup() {
    // Initialize Gravity.
    gravity.Init();
    gravity.encoder.AttachRotateHandler(CalibrateCV);
    gravity.encoder.AttachPressHandler(NextCalibrationPoint);
}

// The loop method must always call `gravity.Process()` to read any peripherial state changes.
void loop() {
    gravity.Process();
    UpdateDisplay();
}

void NextCalibrationPoint() {
    selected_param = (selected_param + 1) % 6;
}

void CalibrateCV(int val) {
    AnalogInput* cv = (selected_param > 2) ? &gravity.cv2 : &gravity.cv1;
    switch (selected_param % 3) {
        case 0:
            cv->AdjustCalibrationLow(val);
            break;
        case 1:
            cv->SetOffset(val);
            break;
        case 2:
            cv->AdjustCalibrationHigh(val);
            break;
    }
}

void UpdateDisplay() {
    gravity.display.firstPage();
    do {
        // Set default font mode and color for each page draw
        gravity.display.setFontMode(0); // Transparent font background
        gravity.display.setDrawColor(1); // Draw with color 1 (ON)
        gravity.display.setFont(TEXT_FONT);
        DisplayCalibration(&gravity.cv1, "CV1: ", 0);
        DisplayCalibration(&gravity.cv2, "CV2: ", 1);
    } while (gravity.display.nextPage());

}

void DisplayCalibration(AnalogInput* cv, const char* title, int index) {
    
    int barWidth = 100;
    int barHeight = 10;
    int textHeight = 10;
    int half = barWidth / 2;
    int offsetX = 16;
    int offsetY = (32 * index);

    // U8g2 draw color: 1 for foreground (white/on), 0 for background (black/off)
    gravity.display.setDrawColor(1);

    // CV value reading.
    int value = cv->Read();

    // Set cursor and print title and value
    gravity.display.setCursor(0, offsetY + textHeight); // Adjust y-position to align with text base line
    gravity.display.print(title);
    if (value >= 0) gravity.display.print(" "); // Add space for positive values for alignment
    gravity.display.print(value);

    // Print voltage
    gravity.display.setCursor(92, offsetY + textHeight); // Adjust x,y position
    if (cv->Voltage() >= 0) gravity.display.print(" ");
    gravity.display.print(cv->Voltage(), 1); // Print float with 1 decimal place
    gravity.display.print(F("V"));

    // Draw the main bar rectangle
    gravity.display.drawFrame(offsetX, textHeight + offsetY + 2, barWidth, barHeight); // Using drawFrame instead of drawRect

    if (value > 0) {
        // 0 to 512
        int x = map(value, 0, 512, 0, half); // Map value to bar fill width
        gravity.display.drawBox(half + offsetX, textHeight + offsetY + 2, x, barHeight); // Using drawBox instead of fillRect
    } else {
        // -512 to 0
        int x = map(abs(value), 0, 512, 0, half); // Map absolute value to bar fill width
        gravity.display.drawBox((half + offsetX) - x, textHeight + offsetY + 2, x, barHeight); // Using drawBox
    }

    // Display selected calibration point if selected calibration point belongs to current cv input.
    if (selected_param / 3 == index) {
        int charWidth = 6;
        int left = offsetX + (half * (selected_param % 3) - 2); // Adjust position
        int top = barHeight + textHeight + offsetY + 12;
        // Drawing an arrow character (ASCII 0x1E is often an up arrow in some fonts, might need adjustment)
        gravity.display.drawStr(left, top, "^"); // Draw the arrow character
    }
}

/**
 * Analog Input Calibration Script
 *
 * Provide each CV input with a constant voltage of -5v, 0v, and 5v. For
 * each config point, provide the appropriate voltage value and then adjust
 * the encoder until you have the correct calibration value set.
 *
 * With the arrow on the left side of the bar, provide a -5v signal and adjust
 * the encoder until you read -512.
 *
 * With the arrow in the center of the bar, provide a 0v signal and adjust the
 * encoder until you read 0.
 *
 * With the arrow on the right side of the bar, provide a 5v signal and adjust
 * the encoder until you read 512.
 *
 * TODO: store the calibration value in EEPROM.
 */

#include "gravity.h"

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

void CalibrateCV(Direction dir, int val) {
    AnalogInput* cv = (selected_param > 2) ? &gravity.cv2 : &gravity.cv1;
    switch (selected_param % 3) {
        case 0:
            cv->AdjustCalibrationLow(val);
            break;
        case 1:
            cv->AdjustCalibrationOffset(val);
            break;
        case 2:
            cv->AdjustCalibrationHigh(val);
            break;
    }
}

void UpdateDisplay() {
    gravity.display.clearDisplay();
    DisplayCalibration(&gravity.cv1, "CV1: ", 0);
    DisplayCalibration(&gravity.cv2, "CV2: ", 1);
    gravity.display.display();
}

void DisplayCalibration(AnalogInput* cv, String title, int index) {
    int barWidth = 100;
    int barHeight = 10;
    int textHeight = 10;
    int half = barWidth / 2;
    int offsetX = 16;
    int offsetY = (32 * index);
    int color = 1;

    // CV value reading.
    int value = constrain(cv->Read(), -512, 512);

    gravity.display.setCursor(0, offsetY);
    gravity.display.print(title);
    gravity.display.print(value >= 0 ? " " : "");
    gravity.display.print(value);

    gravity.display.setCursor(92, offsetY);
    gravity.display.print(value >= 0 ? " " : "");
    gravity.display.print(cv->Voltage());
    gravity.display.print(F("V"));

    gravity.display.drawRect(offsetX, textHeight + offsetY, barWidth, barHeight, color);
    if (value > 0) {
        // 0 to 512
        int x = (float(value) / 512.0) * half;
        int fill = min(x, 512);
        gravity.display.fillRect(half + offsetX, textHeight + offsetY, fill, barHeight, color);
    } else {
        // -512 to 0
        int x = (float(abs(value)) / 512.0) * half;
        int fill = min(half, x);
        gravity.display.fillRect((half + offsetX) - x, textHeight + offsetY, fill, barHeight, color);
    }

    // Display selected calibration point if selected calibration point belongs to current cv input.
    if (selected_param / 3 == index) {
        int left = offsetX + ((half - 2) * (selected_param % 3));
        int top = barHeight + textHeight + offsetY + 2;
        gravity.display.drawChar(left, top, 0x1E, 1, 0, 1);
    }
}

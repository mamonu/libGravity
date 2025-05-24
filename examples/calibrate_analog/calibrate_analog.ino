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

    int cv1 = gravity.cv1.Read();
    int cv2 = gravity.cv2.Read();

    // CV1 Value
    gravity.display.setCursor(10, 2);
    gravity.display.print(F("CV1: "));
    gravity.display.print(cv1);

    gravity.display.drawRect(10, 12, 100, 10, 1);
    if (cv1 > 0) {
        // 0 to 512
        int x = (float(cv1) / 512.0) * 50;
        gravity.display.fillRect(60, 12, x, 10, 1);
    } else {
        // -512 to 0
        int x = (float(abs(cv1)) / 512.0) * 50;
        gravity.display.fillRect(60 - x, 12, x, 10, 1);
    }

    // CV2 Value
    gravity.display.setCursor(10, 32);
    gravity.display.print(F("CV2: "));
    gravity.display.print(cv2);

    gravity.display.drawRect(10, 42, 100, 10, 1);
    if (cv2 >= 0) {
        int x = (float(cv2) / 512.0) * 50;
        gravity.display.fillRect(60, 42, x, 10, 1);
    } else {
        int x = (float(abs(cv2)) / 512.0) * 50;
        gravity.display.fillRect(60 - x, 42, x, 10, 1);
    }

    // Selected calibration point.
    int left = 10 + (48 * (selected_param % 3));
    int top = 22 + (selected_param > 2 ? 32 : 0);
    gravity.display.drawChar(left, top, 0x1E, 1, 0, 1);

    gravity.display.display();
}
#include "gravity.h"

byte idx = 0;
bool reversed = false;
bool freeze = false;
byte selected_param = 0;

// Initialize the gravity library and attach your handlers in the setup method.
void setup() {
    // Initialize Gravity.
    gravity.Init();

    // Attach handlers.
    gravity.clock.AttachIntHandler(IntClock);
}

// The loop method must always call `gravity.Process()` to read any peripherial state changes.
void loop() {
    gravity.Process();

    UpdateDisplay();
}

// The rest of the code is your apps logic!

void IntClock(uint32_t tick) {
    if (tick % 12 == 0  && ! freeze) {
        gravity.outputs[idx].Low();
        if (reversed) {
            idx = (idx == 0) ? OUTPUT_COUNT - 1 : idx - 1;
        } else {
            idx = (idx + 1) % OUTPUT_COUNT;
        }
        gravity.outputs[idx].High();
    }
}


void UpdateDisplay() {
    gravity.display.clearDisplay();

    int cv1 = gravity.cv1.Read();
    int cv2 = gravity.cv2.Read();
    
    gravity.display.setCursor(10, 10);
    gravity.display.print(F("CV1: "));
    gravity.display.print(cv1);

    gravity.display.drawRect(10, 22, 100, 10, 1);
    if (cv1 >= 512) {
        int x = (float(cv1 - 512) / 512.0) * 50; 
        gravity.display.fillRect(60, 22, x, 10, 1);
    } else {
        int x = (float(512 - cv1) / 512.0) * 50; 
        gravity.display.fillRect(60-x, 22, x, 10, 1);
    }

    gravity.display.setCursor(10, 42);
    gravity.display.print(F("CV2: "));
    gravity.display.print(cv2);
    if (cv2 >= 512) {
        int x = (float(cv2 - 512) / 512.0) * 50; 
        gravity.display.fillRect(60, 42, x, 10, 1);
    } else {
        int x = (float(512 - cv2) / 512.0) * 50; 
        gravity.display.fillRect(60-x, 42, x, 10, 1);
    }

    gravity.display.display();
}
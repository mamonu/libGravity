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
    gravity.encoder.AttachRotateHandler(HandleRotate);
    gravity.encoder.AttachPressHandler(ChangeSelectedParam);
    gravity.play_button.AttachPressHandler(HandlePlayPressed);

    // Initial state.
    gravity.outputs[idx].High();
}

// The loop method must always call `gravity.Process()` to read any peripherial state changes.
void loop() {
    gravity.Process();
    freeze = gravity.shift_button.On();
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

void HandlePlayPressed() {
    gravity.clock.Pause();
    if (gravity.clock.IsPaused()) {
        for (int i = 0; i < OUTPUT_COUNT; i++) {
            gravity.outputs[i].Low();
        }
    }
}

void HandleRotate(Direction dir, int val) {
    if (selected_param == 0) {
        gravity.clock.SetTempo(gravity.clock.Tempo() + val);
    } else if (selected_param == 1) {
        reversed = (dir == DIRECTION_DECREMENT);
    }
}

void ChangeSelectedParam() {
    selected_param = (selected_param + 1) % 2;
}

void UpdateDisplay() {
    gravity.display.clearDisplay();

    if (freeze) {
        gravity.display.setCursor(42, 30);
        gravity.display.print("FREEZE!");
        gravity.display.display();
        return;
    }

    gravity.display.setCursor(10, 0);
    gravity.display.print("Tempo: ");
    gravity.display.print(gravity.clock.Tempo());

    gravity.display.setCursor(10, 10);
    gravity.display.print("Direction: ");
    gravity.display.print((reversed) ? "Backward" : "Forward");

    gravity.display.drawChar(0, selected_param * 10, 0x10, 1, 0, 1);

    gravity.display.display();
}
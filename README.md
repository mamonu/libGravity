# Sitka Instruments Gravity Firmware Abstraction

This library helps make writing firmware for the [Sitka Instruments Gravity](https://sitkainstruments.com/gravity/) eurorack module easier by abstracting away the initialization and peripheral interactions. Now your firmware code can just focus on the logic and behavior of the app, and keep the low level code neatly tucked away in this library.

The latest releases of all Sitka Instruments Gravity firmware builds can be found on the [Updater](https://sitkainstruments.com/gravity/updater/) page. You can use this page to flash the latest build directly to the Arduino Nano on the back of your module.

## Project Code Layout

* [`src/`](src/) - **libGravity**: This is the hardware abstraction library used to simplify the creation of new Gravity module firmware by providing common reusable wrappers around the module peripherials like [DigitalOutput](src/digital_output.h#L18) providing methods like [`Update(uint8_t state)`](src/digital_output.h#L45) which allow you to set that output channel voltage high or low, and common module behavior like [Clock](src/clock.h#L30) which provides handlers like [`AttachExtHandler(callback)`](src/clock.h#L69) which takes a callback function to handle external clock tick behavior when receiving clock trigger.

* [`firmware/Gravity`](firmware/Gravity/) - **Alt Gravity**: This is the implementation of the default 6-channel trigger/gate clock modulation firmware. This is a full rewrite of the original firmware designed to use `libGravity` with a focus on open source friendlines.

* `firmware/GridSeq` - **GridSeq**:  Comming Soon.

* [`examples/skeleton`](examples/skeleton/skeleton.ino) - **Skeleton**: This is the bare bones scaffloding for a `libGravity` firmware app.

## Installation

Download or git clone this repository into your Arduino > libraries folder.

Common directory locations:

* [Windows] `C:\Users\{username}\Documents\Arduino\libraries`
* [Mac] `/Users/{username}/Documents/Arduino/libraries`
* [Linux] `~/Arduino/libraries`

## Required Third-party Libraries

* [uClock](https://github.com/midilab/uClock) [MIT] - (Included with this repo) Handle clock tempo, external clock input, and internal clock timer handler.
* [RotateEncoder](https://github.com/mathertel/RotaryEncoder) [BSD] - Library for reading and interpreting encoder rotation.
* [U8g2](https://github.com/olikraus/u8g2/) [MIT] - Graphics helper library.
* [NeoHWSerial](https://github.com/SlashDevin/NeoHWSerial) [GPL] - Hardware serial library with attachInterrupt.

## Example

Here's a trivial example showing some of the ways to interact with the library. This script rotates the active clock channel according to the set tempo. The encoder can change the temo or rotation direction. The play/pause button will toggle the clock activity on or off. The shift button will freeze the clock from advancing the channel rotation.

```cpp
#include "libGravity.h"

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

void HandleRotate(int val) {
    if (selected_param == 0) {
        gravity.clock.SetTempo(gravity.clock.Tempo() + val);
    } else if (selected_param == 1) {
        reversed = (val < 0);
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
```

**Building New Firmware Using libGravity**

When starting a new firmware sketch you can use the [skeleton](examples/skeleton/skeleton.ino) app as a place to start.

**Building New Firmware from scratch**

If you do not want to use the libGravity hardware abstraction library and want to roll your own vanilla firmware, take a look at the [peripherials.h](src/peripherials.h) file for the pinout definitions used by the module.

### Build for release

```
$ arduino-cli compile -v -b  arduino:avr:nano ./firmware/Gravity/Gravity.ino -e --output-dir=./build/
```

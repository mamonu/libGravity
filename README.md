# Sitka Instruments Gravity Firmware Abstraction

This library helps make writing firmware easier by abstracting away the initialization and peripheral interactions. Now your firmware code can just focus on the logic and behavior of the app, and keep the low level code neatly tucked away in this library.

## Required Third-party Libraries

* [uClock](https://github.com/midilab/uClock) [MIT] - Handle clock tempo, external clock input, and internal clock timer handler.
* [RotateEncoder](https://github.com/mathertel/RotaryEncoder) [BSD] - Library for reading and interpreting encoder rotation.
* [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library) [BSD] - Graphics helper library.
* [Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306) [BSD] - Library for interacting with the SSD1306 OLED display.

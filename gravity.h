#ifndef GRAVITY_H
#define GRAVITY_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

#include "analog_input.h"
#include "button.h"
#include "clock.h"
#include "digital_output.h"
#include "encoder_dir.h"
#include "peripherials.h"

// Hardware abstraction wrapper for the Gravity module.
class Gravity {
   public:
    // Constructor
    Gravity() {}

    // Deconstructor
    ~Gravity() {}

    // Initializes the Arduino, and Gravity hardware.
    void Init();

    // Polling check for state change of inputs and outputs.
    void Process();

    Adafruit_SSD1306 * display = nullptr;             // OLED display object.
    Clock * clock = nullptr;                          // Clock source wrapper.
    DigitalOutput outputs[OUTPUT_COUNT];  // An array containing each Output object.
    EncoderDir * encoder = nullptr;                   // Rotary encoder with button instance
    Button * shift_button = nullptr;
    Button * play_button = nullptr;
    AnalogInput * cv1 = nullptr;
    AnalogInput * cv2 = nullptr;

   private:
    void InitClock();
    void InitDisplay();
    void InitInputs();
    void InitOutputs();
};

extern Gravity gravity;

#endif

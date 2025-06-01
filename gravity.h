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
    Gravity()
        : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {}

    // Deconstructor
    ~Gravity() {}

    // Initializes the Arduino, and Gravity hardware.
    void Init();

    // Polling check for state change of inputs and outputs.
    void Process();

    Adafruit_SSD1306 display;             // OLED display object.
    Clock clock;                          // Clock source wrapper.
    DigitalOutput outputs[OUTPUT_COUNT];  // An array containing each Output object.
    EncoderDir encoder;                   // Rotary encoder with button instance
    Button shift_button;
    Button play_button;
    AnalogInput cv1;
    AnalogInput cv2;

   private:
    void InitClock();
    void InitDisplay();
    void InitInputs();
    void InitOutputs();
};

extern Gravity gravity;

#endif

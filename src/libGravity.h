/**
 * @file libGravity.h
 * @author Adam Wonak (https://github.com/awonak)
 * @brief Library for building custom scripts for the Sitka Instruments Gravity module.
 * @version 2.0.0
 * @date 2025-08-17
 * 
 * @copyright MIT - (c) 2025 - Adam Wonak - adam.wonak@gmail.com
 *
 */

#ifndef GRAVITY_H
#define GRAVITY_H

#include <Arduino.h>
#include <U8g2lib.h>

#include "analog_input.h"
#include "button.h"
#include "clock.h"
#include "digital_output.h"
#include "encoder.h"
#include "peripherials.h"

// Hardware abstraction wrapper for the Gravity module.
class Gravity {
   public:
    static const uint8_t OUTPUT_COUNT = 6;

    // Constructor
    Gravity()
        : display(U8G2_R2, SCL, SDA, U8X8_PIN_NONE) {}

    // Deconstructor
    ~Gravity() {}

    // Initializes the Arduino, and Gravity hardware.
    void Init();

    // Polling check for state change of inputs and outputs.
    void Process();

    U8G2_SSD1306_128X64_NONAME_1_HW_I2C display;  // OLED display object.
    Clock clock;                                  // Clock source wrapper.
    DigitalOutput outputs[OUTPUT_COUNT];          // An array containing each Output object.
    DigitalOutput pulse;                          // MIDI Expander module pulse output.
    Encoder encoder;                              // Rotary encoder with button instance
    Button shift_button;
    Button play_button;
    AnalogInput cv1;
    AnalogInput cv2;

   private:
    void initClock();
    void initDisplay();
    void initInputs();
    void initOutputs();
};

extern Gravity gravity;

#endif

/**
 * @file button.h
 * @author Adam Wonak (https://github.com/awonak)
 * @brief for interacting with trigger / gate inputs.
 * @version 0.1
 * @date 2025-04-20
 *
 * Provide methods to convey curent state (HIGH / LOW) and change in state (disengaged, engageing, engaged, disengaging).
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

const uint8_t DEBOUNCE_MS = 10;

class Button {
   protected:
    typedef void (*CallbackFunction)(void);
    CallbackFunction on_press;

   public:
    // Enum constants for active change in button state.
    enum ButtonChange {
        CHANGE_UNCHANGED,
        CHANGE_PRESSED,
        CHANGE_RELEASED,
    };

    Button() {}
    Button(int pin) { Init(pin); }
    ~Button() {}

    /**
     * Initializes a CV Input object.
     *
     * @param pin gpio pin for the cv output.
     */
    void Init(uint8_t pin) {
        pinMode(pin, INPUT_PULLUP);
        pin_ = pin;
    }

    /**
     * Provide a handler function for executing when button is pressed.
     * 
     * @param f Callback function to attach push behavior to this button.
     */
    void AttachPressHandler(CallbackFunction f) {
        on_press = f;
    }

    // Execute the press callback.
    void OnPress() {
        if (on_press != NULL) {
            on_press();
        }
    }

    /**
     * Read the state of the cv input.
     */
    void Process() {
        int read = digitalRead(pin_);

        bool debounced = (millis() - last_press_) > DEBOUNCE_MS;
        bool pressed = read == 0 && old_read_ == 1 && debounced;
        bool released = read == 1 && old_read_ == 0 && debounced;
        // Update variables for next loop
        last_press_ = (pressed || released) ? millis() : last_press_;

        // Determine current clock input state.
        change_ = CHANGE_UNCHANGED;
        if (pressed) {
            change_ = CHANGE_PRESSED;
            on_ = true;
        } else if (released) {
            change_ = CHANGE_RELEASED;
            on_ = false;
        }
        old_read_ = read;
    }

    /**
     * Get the state change for the button.
     *
     * @return ButtonChange
     */
    inline ButtonChange Change() { return change_; }

    /**
     * Current cv state represented as a bool.
     *
     * @return true if cv signal is high, false if cv signal is low
     */
    inline bool On() { return on_; }

   private:
    uint8_t pin_;
    uint8_t old_read_;
    unsigned long last_press_;
    ButtonChange change_ = CHANGE_UNCHANGED;
    bool on_;
};

#endif

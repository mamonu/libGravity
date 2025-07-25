/**
 * @file encoder_dir.h
 * @author Adam Wonak (https://github.com/awonak)
 * @brief Class for interacting with encoders.
 * @version 0.1
 * @date 2025-04-19
 *
 * @copyright MIT - (c) 2025 - Adam Wonak - adam.wonak@gmail.com
 *
 */

#ifndef ENCODER_DIR_H
#define ENCODER_DIR_H

#include <RotaryEncoder.h>

#include "button.h"
#include "peripherials.h"

/**
 * @brief Class for interacting with a rotary encoder that has a push button.
 */
class Encoder {
   protected:
    typedef void (*CallbackFunction)(void);
    typedef void (*RotateCallbackFunction)(int val);
    CallbackFunction on_press;
    RotateCallbackFunction on_press_rotate;
    RotateCallbackFunction on_rotate;
    int change;

   public:
    Encoder() : encoder_(ENCODER_PIN1, ENCODER_PIN2, RotaryEncoder::LatchMode::FOUR3),
                   button_(ENCODER_SW_PIN) {
        _instance = this;
    }
    ~Encoder() {}

    /**
     * @brief Set the direction of the encoder.
     *
     * @param reversed Set to true to reverse the direction of rotation.
     */
    void SetReverseDirection(bool reversed) {
        reversed_ = reversed;
    }

    /**
     * @brief Attach a handler for the encoder button press.
     *
     * This callback is triggered on a simple press and release of the button,
     * without any rotation occurring during the press.
     *
     * @param f The callback function to execute when a button press.
     */
    void AttachPressHandler(CallbackFunction f) {
        on_press = f;
    }

    /**
     * @brief Attach a handler for encoder rotation.
     *
     * This callback is triggered when the encoder is rotated while the button is not pressed.
     *
     * @param f The callback function to execute on rotation. It receives an integer
     * representing the change in position (can be positive or negative).
     */
    void AttachRotateHandler(RotateCallbackFunction f) {
        on_rotate = f;
    }

    /**
     * @brief Attach a handler for rotation while the button is pressed.
     *
     * This callback is triggered when the encoder is rotated while the button is being held down.
     *
     * @param f The callback function to execute. It receives an integer
     * representing the change in position.
     */
    void AttachPressRotateHandler(RotateCallbackFunction f) {
        on_press_rotate = f;
    }

    /**
     * @brief Processes encoder and button events.
     *
     * This method should be called repeatedly in the main loop to check for state
     * changes (rotation, button presses) and dispatch the appropriate callbacks.
     */
    void Process() {
        // Get encoder position change amount.
        int encoder_rotated = _rotate_change() != 0;
        bool button_pressed = button_.On();
        button_.Process();

        // Handle encoder position change and button press.
        if (button_pressed && encoder_rotated) {
            rotated_while_held_ = true;
            if (on_press_rotate != NULL) on_press_rotate(change);
        } else if (!button_pressed && encoder_rotated) {
            if (on_rotate != NULL) on_rotate(change);
        } else if (button_.Change() == Button::CHANGE_RELEASED && !rotated_while_held_) {
            if (on_press != NULL) on_press();
        }

        // Reset rotate while held state.
        if (button_.Change() == Button::CHANGE_RELEASED && rotated_while_held_) {
            rotated_while_held_ = false;
        }
    }

    static void isr() {
        // If the instance has been created, call its tick() method.
        if (_instance) {
            _instance->encoder_.tick();
        }
    }

   private:
    static Encoder* _instance;

    int previous_pos_;
    bool rotated_while_held_;
    bool reversed_ = false;
    RotaryEncoder encoder_;
    Button button_;

    // Return the number of ticks change since last polled.
    int _rotate_change() {
        int position = encoder_.getPosition();
        unsigned long ms = encoder_.getMillisBetweenRotations();

        if (previous_pos_ == position) {
            return 0;
        }

        // Update state variables.
        change = position - previous_pos_;
        previous_pos_ = position;

        // Encoder rotate acceleration.
        if (ms < 16) {
            change *= 3;
        } else if (ms < 32) {
            change *= 2;
        }

        if (reversed_) {
            change = -(change);
        }
        return change;
    }
};

#endif
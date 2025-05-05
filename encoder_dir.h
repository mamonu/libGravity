/**
 * @file encoder_dir.h
 * @author Adam Wonak (https://github.com/awonak)
 * @brief Class for interacting with encoders.
 * @version 0.1
 * @date 2025-04-19
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef ENCODER_DIR_H
#define ENCODER_DIR_H

#include <RotaryEncoder.h>

#include "button.h"
#include "peripherials.h"

enum Direction {
    DIRECTION_UNCHANGED,
    DIRECTION_INCREMENT,
    DIRECTION_DECREMENT,
};

class EncoderDir {
   protected:
    typedef void (*CallbackFunction)(void);
    typedef void (*RotateCallbackFunction)(Direction dir, int val);
    CallbackFunction on_press;
    RotateCallbackFunction on_press_rotate;
    RotateCallbackFunction on_rotate;
    int change;

   public:
    EncoderDir() : encoder_(ENCODER_PIN1, ENCODER_PIN2, RotaryEncoder::LatchMode::FOUR3),
                   button_(ENCODER_SW_PIN) {}
    ~EncoderDir() {}

    // Set to true if the encoder read direction should be reversed.
    void SetReverseDirection(bool reversed) {
        reversed_ = reversed;
    }
    void AttachPressHandler(CallbackFunction f) {
        button_.AttachPressHandler(f);
    }

    void AttachRotateHandler(RotateCallbackFunction f) {
        on_rotate = f;
    }

    void AttachPressRotateHandler(RotateCallbackFunction f) {
        on_press_rotate = f;
    }

    void OnRotate() {
        if (on_rotate != NULL) {
            on_rotate(RotateDirection(), change);
        }
    }

    void OnPressRotate() {
        if (on_press_rotate != NULL) {
            on_press_rotate(RotateDirection(), change);
        }
    }

    // Parse EncoderButton increment direction.
    Direction RotateDirection() {
        int dir = (int)(encoder_.getDirection());
        return rotate_(dir, reversed_);
    }

    void Process() {
        button_.Process();
        int change = _rotate_change();

        // Handle encoder position change and button press.
        if (button_.On() && change != 0) {
            OnPressRotate();
            rotated_while_held = true;
        } else if (button_.Change() == Button::CHANGE_RELEASED && !rotated_while_held) {
            button_.OnPress();
        }

        if (button_.Change() == Button::CHANGE_RELEASED) {
            rotated_while_held = false;
        }

        if (change != 0 && !button_.On()) {
            OnRotate();
        }
    }

    // Read the encoder state and update the read position.
    void UpdateEncoder() {
        encoder_.tick();
    }

   private:
    int previous_pos_;
    bool reversed_ = true;
    RotaryEncoder encoder_;
    Button button_;
    bool rotated_while_held;

    // Return the number of ticks change since last polled.
    int _rotate_change() {
        int position = encoder_.getPosition();
        unsigned long ms = encoder_.getMillisBetweenRotations();

        // Validation (TODO: add debounce check).
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

        return change;
    }

    inline Direction rotate_(int dir, bool reversed) {
        switch (dir) {
            case 1:
                return (reversed) ? DIRECTION_INCREMENT : DIRECTION_DECREMENT;
            case -1:
                return (reversed) ? DIRECTION_DECREMENT : DIRECTION_INCREMENT;
            default:
                return DIRECTION_UNCHANGED;
        }
    }
};

#endif
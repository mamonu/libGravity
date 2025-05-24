/**
 * @file analog_input.h
 * @author Adam Wonak (https://github.com/awonak)
 * @brief Class for interacting with analog inputs.
 * @version 0.1
 * @date 2025-05-23
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef ANALOG_INPUT_H
#define ANALOG_INPUT_H

const int MAX_INPUT = (1 << 10) - 1;  // Max 10 bit analog read resolution.

class AnalogInput {
   public:
    AnalogInput() {}
    ~AnalogInput() {}

    /**
    * @brief Initializes a analog input object.
    * 
    * @param pin gpio pin for the analog input.
    */
    void Init(uint8_t pin) {
        pinMode(pin, INPUT);
        pin_ = pin;
    }

    /**
     * @brief Read the value of the analog input.
     * 
     */
    void Process() {
        old_read_ = read_;
        int raw = analogRead(pin_);
        read_ = map(raw, calibration_offset_, MAX_INPUT, calibration_low_, calibration_high_);
    }

    // Set calibration values.

    void AdjustCalibrationLow(int val) { calibration_low_ += val; }
    void AdjustCalibrationOffset(int val) { calibration_offset_ += val; }
    void AdjustCalibrationHigh(int val) { calibration_high_ += val; }

    /**
     * @brief Get the current value of the analog input within a range of +/-512.
     * 
     * @return InputState 
     */
    inline int16_t Read() { return read_; }

   private:
    uint8_t pin_;
    int16_t read_;
    uint16_t old_read_;
    int calibration_offset_ = 0;
    int calibration_low_ = -512;
    int calibration_high_ = 512;
};

#endif

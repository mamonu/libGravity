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
    * Initializes a analog input object.
    * 
    * @param pin gpio pin for the analog input.
    */
    void Init(uint8_t pin) {
        pinMode(pin, INPUT);
        pin_ = pin;
    }

    /**
     * Read the value of the analog input and set instance state.
     * 
     */
    void Process() {
        old_read_ = read_;
        int raw = analogRead(pin_);
        read_ = map(raw, offset_, MAX_INPUT, low_, high_);
    }

    // Set calibration values.

    void AdjustCalibrationLow(int amount) { low_ += amount; }
    void AdjustCalibrationOffset(int amount) { offset_ -= amount; }
    void AdjustCalibrationHigh(int amount) { high_ += amount; }

    /**
     * Get the current value of the analog input within a range of +/-512.
     * 
     * @return read value within a range of +/-512.
     * 
     */
    inline int16_t Read() { return read_; }

    /**
     * Return the analog read value as voltage.
     * 
     * @return A float representing the voltage (-5.0 to +5.0).
     * 
     */
    inline float Voltage() { return ((read_ / 512.0) * 5.0); }

   private:
    uint8_t pin_;
    int16_t read_;
    uint16_t old_read_;
    // calibration values.
    int offset_ = 0;
    int low_ = -512;
    int high_ = 512;
};

#endif

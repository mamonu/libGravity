/**
 * @file analog_input.h
 * @author Adam Wonak (https://github.com/awonak)
 * @brief Class for interacting with analog inputs.
 * @version 0.1
 * @date 2025-05-23
 *
 * @copyright MIT - (c) 2025 - Adam Wonak - adam.wonak@gmail.com
 *
 */
#ifndef ANALOG_INPUT_H
#define ANALOG_INPUT_H

const int MAX_INPUT = (1 << 10) - 1;  // Max 10 bit analog read resolution.

// estimated default calibration value
const int CALIBRATED_LOW = -566;
const int CALIBRATED_HIGH = 512;

/**
 * @brief Class for interacting with analog inputs (CV).
 */
class AnalogInput {
   public:
    AnalogInput() {}
    ~AnalogInput() {}

    /**
     * @brief Initializes an analog input object.
     *
     * @param pin The GPIO pin for the analog input.
     */
    void Init(uint8_t pin) {
        pinMode(pin, INPUT);
        pin_ = pin;
    }

    /**
     * @brief Reads and processes the analog input.
     *
     * This method reads the raw value from the ADC, applies the current
     * calibration, offset, and attenuation/inversion settings. It should be
     * called regularly in the main loop to update the input's state.
     */
    void Process() {
        old_read_ = read_;
        int raw = analogRead(pin_);
        read_ = map(raw, 0, MAX_INPUT, low_, high_);
        read_ = constrain(read_ - offset_, -512, 512);
        if (inverted_) read_ = -read_;
    }

    /**
     * @brief Adjusts the low calibration point.
     *
     * This is used to fine-tune the mapping of the raw analog input to the output range.
     *
     * @param amount The amount to add to the current low calibration value.
     */
    void AdjustCalibrationLow(int amount) { low_ += amount; }

    /**
     * @brief Adjusts the high calibration point.
     *
     * This is used to fine-tune the mapping of the raw analog input to the output range.
     *
     * @param amount The amount to add to the current high calibration value.
     */
    void AdjustCalibrationHigh(int amount) { high_ += amount; }

    /**
     * @brief Sets a DC offset for the input.
     *
     * @param percent A percentage (e.g., 0.5 for 50%) to shift the signal.
     */
    void SetOffset(float percent) { offset_ = -(percent)*512; }

    /**
     * @brief Sets the attenuation (scaling) of the input signal.
     *
     * This scales the input signal. A negative percentage will also invert the signal.
     *
     * @param percent The attenuation level, typically from 0.0 to 1.0.
     */
    void SetAttenuation(float percent) {
        low_ = abs(percent) * CALIBRATED_LOW;
        high_ = abs(percent) * CALIBRATED_HIGH;
        inverted_ = percent < 0;
    }

    /**
     * @brief Get the current processed value of the analog input.
     *
     * @return The read value within a range of +/-512.
     */
    inline int16_t Read() { return read_; }

    /**
     * @brief Return the analog read value as a voltage.
     *
     * @return A float representing the calculated voltage (-5.0 to +5.0).
     */
    inline float Voltage() { return ((read_ / 512.0) * 5.0); }

   private:
    uint8_t pin_;
    int16_t read_;
    uint16_t old_read_;
    // calibration values.
    int offset_ = 0;
    int low_ = CALIBRATED_LOW;
    int high_ = CALIBRATED_HIGH;
    bool inverted_ = false;
};

#endif
/**
 * @file channel.h
 * @author Adam Wonak (https://github.com/awonak/)
 * @brief Alt firmware version of Gravity by Sitka Instruments.
 * @version 2.0.1
 * @date 2025-07-04
 *
 * @copyright MIT - (c) 2025 - Adam Wonak - adam.wonak@gmail.com
 *
 */

#ifndef CHANNEL_H
#define CHANNEL_H

#include <Arduino.h>
#include <libGravity.h>

#include "euclidean.h"

// Enums for CV Mod destination
enum CvDestination : uint8_t {
    CV_DEST_NONE,
    CV_DEST_MOD,
    CV_DEST_PROB,
    CV_DEST_DUTY,
    CV_DEST_OFFSET,
    CV_DEST_SWING,
    CV_DEST_EUC_STEPS,
    CV_DEST_EUC_HITS,
    CV_DEST_LAST,
};

static const byte MOD_CHOICE_SIZE = 25;

// Negative numbers are multipliers, positive are divisors.
static const int CLOCK_MOD[MOD_CHOICE_SIZE] PROGMEM = {
    // Divisors
    128, 64, 32, 24, 16, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2,
    // Internal Clock Unity (quarter note)
    1,
    // Multipliers
    -2, -3, -4, -6, -8, -12, -16, -24};

// This represents the number of clock pulses for a 96 PPQN clock source
// that match the above div/mult mods.
static const int CLOCK_MOD_PULSES[MOD_CHOICE_SIZE] PROGMEM = {
    // Divisor Pulses (96 * X)
    12288, 6144, 3072, 2304, 1536, 1152, 1056, 960, 864, 768, 672, 576, 480, 384, 288, 192,
    // Internal Clock Pulses
    96,
    // Multiplier Pulses (96 / X)
    48, 32, 24, 16, 12, 8, 6, 4};

static const byte DEFAULT_CLOCK_MOD_INDEX = 16;  // x1 or 96 PPQN.

static const byte PULSE_PPQN_24_CLOCK_MOD_INDEX = MOD_CHOICE_SIZE - 1;
static const byte PULSE_PPQN_4_CLOCK_MOD_INDEX = MOD_CHOICE_SIZE - 6;
static const byte PULSE_PPQN_1_CLOCK_MOD_INDEX = MOD_CHOICE_SIZE - 9;

class Channel {
   public:
    Channel() {
        Init();
    }

    void Init() {
        // Reset base values to their defaults
        base_clock_mod_index = DEFAULT_CLOCK_MOD_INDEX;
        base_probability = 100;
        base_duty_cycle = 50;
        base_offset = 0;
        base_swing = 50;

        cv1_dest = CV_DEST_NONE;
        cv2_dest = CV_DEST_NONE;

        pattern.Init(DEFAULT_PATTERN);

        // Calcule the clock mod pulses on init.
        _recalculatePulses();
    }

    bool isCvModActive() const { return cv1_dest != CV_DEST_NONE || cv2_dest != CV_DEST_NONE; }

    // Setters (Set the BASE value)

    void setClockMod(int index) {
        base_clock_mod_index = constrain(index, 0, MOD_CHOICE_SIZE - 1);
    }

    void setProbability(int prob) {
        base_probability = constrain(prob, 0, 100);
    }

    void setDutyCycle(int duty) {
        base_duty_cycle = constrain(duty, 1, 99);
    }

    void setOffset(int off) {
        base_offset = constrain(off, 0, 99);
    }
    void setSwing(int val) {
        base_swing = constrain(val, 50, 95);
    }

    // Euclidean
    void setSteps(int val) {
        pattern.SetSteps(val);
    }
    void setHits(int val) {
        pattern.SetHits(val);
    }

    void setCv1Dest(CvDestination dest) { cv1_dest = dest; }
    void setCv2Dest(CvDestination dest) { cv2_dest = dest; }
    CvDestination getCv1Dest() const { return cv1_dest; }
    CvDestination getCv2Dest() const { return cv2_dest; }

    // Getters (Get the BASE value for editing or cv modded value for display)
    int getProbability() const { return base_probability; }
    int getDutyCycle() const { return base_duty_cycle; }
    int getOffset() const { return base_offset; }
    int getSwing() const { return base_swing; }
    int getClockMod() const { return pgm_read_word_near(&CLOCK_MOD[getClockModIndex()]); }
    int getClockModIndex() const { return base_clock_mod_index; }
    byte getSteps() const { return pattern.GetSteps(); }
    byte getHits() const { return pattern.GetHits(); }

    // Getters that calculate the value with CV modulation applied.
    int getClockModIndexWithMod(int cv1_val, int cv2_val) {
        int clock_mod_index = _calculateMod(CV_DEST_MOD, cv1_val, cv2_val, -(MOD_CHOICE_SIZE / 2), MOD_CHOICE_SIZE / 2);
        return constrain(base_clock_mod_index + clock_mod_index, 0, MOD_CHOICE_SIZE - 1);
    }

    int getClockModWithMod(int cv1_val, int cv2_val) {
        int clock_mod = _calculateMod(CV_DEST_MOD, cv1_val, cv2_val, -(MOD_CHOICE_SIZE / 2), MOD_CHOICE_SIZE / 2);
        return pgm_read_word_near(&CLOCK_MOD[getClockModIndexWithMod(cv1_val, cv2_val)]);
    }

    int getProbabilityWithMod(int cv1_val, int cv2_val) {
        int prob_mod = _calculateMod(CV_DEST_PROB, cv1_val, cv2_val, -50, 50);
        return constrain(base_probability + prob_mod, 0, 100);
    }

    int getDutyCycleWithMod(int cv1_val, int cv2_val) {
        int duty_mod = _calculateMod(CV_DEST_DUTY, cv1_val, cv2_val, -50, 50);
        return constrain(base_duty_cycle + duty_mod, 1, 99);
    }

    int getOffsetWithMod(int cv1_val, int cv2_val) {
        int offset_mod = _calculateMod(CV_DEST_OFFSET, cv1_val, cv2_val, -50, 50);
        return constrain(base_offset + offset_mod, 0, 99);
    }

    int getSwingWithMod(int cv1_val, int cv2_val) {
        int swing_mod = _calculateMod(CV_DEST_SWING, cv1_val, cv2_val, -25, 25);
        return constrain(base_swing + swing_mod, 50, 95);
    }

    byte getStepsWithMod(int cv1_val, int cv2_val) {
        int step_mod = _calculateMod(CV_DEST_EUC_STEPS, cv1_val, cv2_val, 0, MAX_PATTERN_LEN);
        return constrain(pattern.GetSteps() + step_mod, 1, MAX_PATTERN_LEN);
    }

    byte getHitsWithMod(int cv1_val, int cv2_val) {
        // The number of hits is dependent on the modulated number of steps.
        byte modulated_steps = getStepsWithMod(cv1_val, cv2_val);
        int hit_mod = _calculateMod(CV_DEST_EUC_HITS, cv1_val, cv2_val, 0, modulated_steps);
        return constrain(pattern.GetHits() + hit_mod, 1, modulated_steps);
    }

    void toggleMute() { mute = !mute; }

    /**
     * @brief Processes a clock tick and determines if the output should be high or low.
     * Note: this method is called from an ISR and must be kept as simple as possible.
     * @param tick The current clock tick count.
     * @param output The output object to be modified.
     */
    void processClockTick(uint32_t tick, DigitalOutput& output) {
        // Mute check
        if (mute) {
            output.Low();
            return;
        }

        if (isCvModActive()) _recalculatePulses();

        int cv1 = gravity.cv1.Read();
        int cv2 = gravity.cv2.Read();
        int cvmod_clock_mod_index = getClockModIndexWithMod(cv1, cv2);
        int cvmod_probability = getProbabilityWithMod(cv1, cv2);

        const uint16_t mod_pulses = pgm_read_word_near(&CLOCK_MOD_PULSES[cvmod_clock_mod_index]);

        // Conditionally apply swing on down beats.
        uint16_t swing_pulses = 0;
        if (_swing_pulse_amount > 0 && (tick / mod_pulses) % 2 == 1) {
            swing_pulses = _swing_pulse_amount;
        }

        // Duty cycle high check logic
        const uint32_t current_tick_offset = tick + _offset_pulses + swing_pulses;
        if (!output.On()) {
            // Step check
            if (current_tick_offset % mod_pulses == 0) {
                bool hit = cvmod_probability >= random(0, 100);
                // Euclidean rhythm hit check
                switch (pattern.NextStep()) {
                    case Pattern::REST:  // Rest when active or fall back to probability
                        hit = false;
                        break;
                    case Pattern::HIT:  // Hit if probability is true
                        hit &= true;
                        break;
                }
                if (hit) {
                    output.High();
                }
            }
        }

        // Duty cycle low check
        const uint32_t duty_cycle_end_tick = tick + _duty_pulses + _offset_pulses + swing_pulses;
        if (duty_cycle_end_tick % mod_pulses == 0) {
            output.Low();
        }
    }

   private:
    int _calculateMod(CvDestination dest, int cv1_val, int cv2_val, int min_range, int max_range) {
        int mod1 = (cv1_dest == dest) ? map(cv1_val, -512, 512, min_range, max_range) : 0;
        int mod2 = (cv2_dest == dest) ? map(cv2_val, -512, 512, min_range, max_range) : 0;
        return mod1 + mod2;
    }

    void _recalculatePulses() {
        int cv1 = gravity.cv1.Read();
        int cv2 = gravity.cv2.Read();
        int clock_mod_index = getClockModIndexWithMod(cv1, cv2);
        int duty_cycle = getDutyCycleWithMod(cv1, cv2);
        int offset = getOffsetWithMod(cv1, cv2);
        int swing = getSwingWithMod(cv1, cv2);
        const uint16_t mod_pulses = pgm_read_word_near(&CLOCK_MOD_PULSES[clock_mod_index]);
        _duty_pulses = max((long)((mod_pulses * (100L - duty_cycle)) / 100L), 1L);
        _offset_pulses = (long)((mod_pulses * (100L - offset)) / 100L);

        // Calculate the down beat swing amount.
        if (swing > 50) {
            int shifted_swing = swing - 50;
            _swing_pulse_amount = (long)((mod_pulses * (100L - shifted_swing)) / 100L);
        } else {
            _swing_pulse_amount = 0;
        }
    }

    // User-settable base values.
    byte base_clock_mod_index;
    byte base_probability;
    byte base_duty_cycle;
    byte base_offset;
    byte base_swing;

    // CV mod configuration
    CvDestination cv1_dest;
    CvDestination cv2_dest;

    // Euclidean pattern
    Pattern pattern;

    // Mute channel flag
    bool mute;

    // Pre-calculated pulse values for ISR performance
    uint16_t _duty_pulses;
    uint16_t _offset_pulses;
    uint16_t _swing_pulse_amount;
};

#endif  // CHANNEL_H
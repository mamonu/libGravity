#ifndef CHANNEL_H
#define CHANNEL_H

#include <Arduino.h>
#include <gravity.h>
#include "euclidean.h"

// Enums for CV configuration
enum CvSource {
    CV_NONE,
    CV_1,
    CV_2,
    CV_LAST,
};

enum CvDestination {
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

static const int MOD_CHOICE_SIZE = 21;
// Negative for multiply, positive for divide.
static const int clock_mod[MOD_CHOICE_SIZE] = {-24, -12, -8, -6, -4, -3, -2, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32, 64, 128};
// This represents the number of clock pulses for a 96 PPQN clock source that match the above div/mult mods.
static const int clock_mod_pulses[MOD_CHOICE_SIZE] = {4, 8, 12, 16, 24, 32, 48, 96, 192, 288, 384, 480, 576, 1152, 672, 768, 1536, 2304, 3072, 6144, 12288};

class Channel {
   public:
    Channel() {
        Init();
    }

    void Init() {
        // Reset base values to their defaults
        base_clock_mod_index = 7;
        base_probability = 100;
        base_duty_cycle = 50;
        base_offset = 0;
        base_swing = 50;

        cv_source = CV_NONE;
        cv_destination = CV_DEST_NONE;

        cvmod_clock_mod_index = base_clock_mod_index;
        cvmod_probability = base_probability;
        cvmod_duty_cycle = base_duty_cycle;
        cvmod_offset = base_offset;
        cvmod_swing = base_swing;

        pattern.Init(DEFAULT_PATTERN);
    }

    // Setters (Set the BASE value)

    void setClockMod(int index) {
        base_clock_mod_index = constrain(index, 0, MOD_CHOICE_SIZE - 1);
        if (!isCvModActive()) {
            cvmod_clock_mod_index = base_clock_mod_index;
        }
    }

    void setProbability(int prob) { 
        base_probability = constrain(prob, 0, 100);
        if (!isCvModActive()) {
            cvmod_probability = base_probability;
        }
    }

    void setDutyCycle(int duty) {
        base_duty_cycle = constrain(duty, 1, 99); 
        if (!isCvModActive()) {
            cvmod_duty_cycle = base_duty_cycle;
        }
    }

    void setOffset(int off) { 
        base_offset = constrain(off, 0, 99);
        if (!isCvModActive()) {
            cvmod_offset = base_offset;
        }
    }
    void setSwing(int val) {
         base_swing = constrain(val, 50, 95); 
        if (!isCvModActive()) {
            cvmod_swing = base_swing;
        }
    }

    void setCvSource(CvSource source) { cv_source = source; }
    void setCvDestination(CvDestination dest) { cv_destination = dest; }

    // Getters (Get the BASE value for editing or cv modded value for display)

    int getProbability(bool withCvMod = false) const { return withCvMod ? cvmod_probability : base_probability; }
    int getDutyCycle(bool withCvMod = false) const { return withCvMod ? cvmod_duty_cycle : base_duty_cycle; }
    int getOffset(bool withCvMod = false) const { return withCvMod ? cvmod_offset : base_offset; }
    int getSwing(bool withCvMod = false) const { return withCvMod ? cvmod_swing : base_swing; }
    int getClockMod(bool withCvMod = false) const { return clock_mod[getClockModIndex(withCvMod)]; }
    int getClockModIndex(bool withCvMod = false) const { return withCvMod ? cvmod_clock_mod_index : base_clock_mod_index; }
    CvSource getCvSource() { return cv_source; }
    CvDestination getCvDestination() { return cv_destination; }
    bool isCvModActive() const { return cv_source != CV_NONE && cv_destination != CV_DEST_NONE; }

    // Euclidean
    void setSteps(int val) { pattern.SetSteps(val); }
    void setHits(int val) { pattern.SetHits(val); }
    byte getSteps() { return pattern.GetSteps(); }
    byte getHits() { return pattern.GetHits(); }

    /**
     * @brief Processes a clock tick and determines if the output should be high or low.
     * @param tick The current clock tick count.
     * @param output The output object to be modified.
     */
    void processClockTick(uint32_t tick, DigitalOutput& output) {
        // Calculate output duty cycle state using cv modded values to determine pulse counts.
        const uint16_t mod_pulses = clock_mod_pulses[cvmod_clock_mod_index];
        const uint16_t duty_pulses = max((long)((mod_pulses * (100L - cvmod_duty_cycle)) / 100L), 1L);
        const uint16_t offset_pulses = (long)((mod_pulses * (100L - cvmod_offset)) / 100L);

        uint16_t swing_pulses = 0;
        // Check step increment for odd beats.
        if (cvmod_swing > 50 && (tick / mod_pulses) % 2 == 1) {
            int shifted_swing = cvmod_swing - 50;
            swing_pulses = (long)((mod_pulses * (100L - shifted_swing)) / 100L);
        }

        const uint32_t current_tick_offset = tick + offset_pulses + swing_pulses;

        // Duty cycle high check logic
        if (!output.On()) {
            // Step check
            if (current_tick_offset % mod_pulses == 0) {
                bool hit = cvmod_probability >= random(0, 100);
                // Euclidean rhythm check
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
        const uint32_t duty_cycle_end_tick = tick + duty_pulses + offset_pulses + swing_pulses;
        if (duty_cycle_end_tick % mod_pulses == 0) {
            output.Low();
        }
    }

    void applyCvMod(int cv1_value, int cv2_value) {
        // Use the CV value for current selected cv source.
        int value = (cv_source == CV_1) ? cv1_value : cv2_value;

        // Calculate and store cv modded values using bipolar mapping.
        // Default to base value if not the current CV destination.

        cvmod_clock_mod_index =
            (cv_destination == CV_DEST_MOD)
                ? constrain(base_clock_mod_index + map(value, -512, 512, -10, 10), 0, MOD_CHOICE_SIZE - 1)
                : base_clock_mod_index;

        cvmod_probability =
            (cv_destination == CV_DEST_PROB)
                ? constrain(base_probability + map(value, -512, 512, -50, 50), 0, 100)
                : base_probability;

        cvmod_duty_cycle =
            (cv_destination == CV_DEST_DUTY)
                ? constrain(base_duty_cycle + map(value, -512, 512, -50, 50), 1, 99)
                : base_duty_cycle;

        cvmod_offset =
            (cv_destination == CV_DEST_OFFSET)
                ? constrain(base_offset + map(value, -512, 512, -50, 50), 0, 99)
                : base_offset;

        cvmod_swing =
            (cv_destination == CV_DEST_SWING)
                ? constrain(base_swing + map(value, -512, 512, -25, 25), 50, 95)
                : base_swing;
        
        if (cv_destination == CV_DEST_EUC_STEPS) {
            pattern.SetSteps(map(value, -512, 512, 0, MAX_PATTERN_LEN));
        }
        
        if (cv_destination == CV_DEST_EUC_HITS) {
            pattern.SetHits(map(value, -512, 512, 0, pattern.GetSteps()));
        }
    }

   private:
    // User-settable base values.
    byte base_clock_mod_index;
    byte base_probability;
    byte base_duty_cycle;
    byte base_offset;
    byte base_swing;

    // Base value with cv mod applied.
    byte cvmod_clock_mod_index;
    byte cvmod_probability;
    byte cvmod_duty_cycle;
    byte cvmod_offset;
    byte cvmod_swing;

    // CV configuration
    CvSource cv_source = CV_NONE;
    CvDestination cv_destination = CV_DEST_NONE;

    // Euclidean pattern
    Pattern pattern;
};

#endif  // CHANNEL_H
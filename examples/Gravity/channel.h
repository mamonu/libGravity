#ifndef CHANNEL_H
#define CHANNEL_H

#include <Arduino.h>
#include <gravity.h>

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

static const byte MOD_CHOICE_SIZE = 21;
// Negative for multiply, positive for divide.
static const int CLOCK_MOD[MOD_CHOICE_SIZE] PROGMEM = {-24, -12, -8, -6, -4, -3, -2, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32, 64, 128};
// This represents the number of clock pulses for a 96 PPQN clock source that match the above div/mult mods.
static const int CLOCK_MOD_PULSES[MOD_CHOICE_SIZE] PROGMEM = {4, 8, 12, 16, 24, 32, 48, 96, 192, 288, 384, 480, 576, 1152, 672, 768, 1536, 2304, 3072, 6144, 12288};

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
        base_euc_steps = 1;
        base_euc_hits = 1;

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
        if (cv1_dest != CV_DEST_MOD && cv2_dest != CV_DEST_MOD) {
            cvmod_clock_mod_index = base_clock_mod_index;
        }
    }

    void setProbability(int prob) {
        base_probability = constrain(prob, 0, 100);
        if (cv1_dest != CV_DEST_PROB && cv2_dest != CV_DEST_PROB) {
            cvmod_probability = base_probability;
        }
    }

    void setDutyCycle(int duty) {
        base_duty_cycle = constrain(duty, 1, 99);
        if (cv1_dest != CV_DEST_DUTY && cv2_dest != CV_DEST_DUTY) {
            cvmod_duty_cycle = base_duty_cycle;
        }
    }

    void setOffset(int off) {
        base_offset = constrain(off, 0, 99);
        if (cv1_dest != CV_DEST_OFFSET && cv2_dest != CV_DEST_OFFSET) {
            cvmod_offset = base_offset;
        }
    }
    void setSwing(int val) {
        base_swing = constrain(val, 50, 95);
        if (cv1_dest != CV_DEST_SWING && cv2_dest != CV_DEST_SWING) {
            cvmod_swing = base_swing;
        }
    }

    // Euclidean
    void setSteps(int val) {
        base_euc_steps = constrain(val, 1, MAX_PATTERN_LEN);
        if (cv1_dest != CV_DEST_EUC_STEPS && cv2_dest != CV_DEST_EUC_STEPS) {
            pattern.SetSteps(val);
        }
    }
    void setHits(int val) {
        base_euc_hits = constrain(val, 1, base_euc_steps);
        if (cv1_dest != CV_DEST_EUC_HITS && cv2_dest != CV_DEST_EUC_HITS) {
            pattern.SetHits(val);
        }
    }

    void setCv1Dest(CvDestination dest) { cv1_dest = dest; }
    void setCv2Dest(CvDestination dest) { cv2_dest = dest; }
    CvDestination getCv1Dest() const { return cv1_dest; }
    CvDestination getCv2Dest() const { return cv2_dest; }

    // Getters (Get the BASE value for editing or cv modded value for display)

    int getProbability(bool withCvMod = false) const { return withCvMod ? cvmod_probability : base_probability; }
    int getDutyCycle(bool withCvMod = false) const { return withCvMod ? cvmod_duty_cycle : base_duty_cycle; }
    int getOffset(bool withCvMod = false) const { return withCvMod ? cvmod_offset : base_offset; }
    int getSwing(bool withCvMod = false) const { return withCvMod ? cvmod_swing : base_swing; }
    int getClockMod(bool withCvMod = false) const { return pgm_read_word_near(&CLOCK_MOD[getClockModIndex(withCvMod)]); }
    int getClockModIndex(bool withCvMod = false) const { return withCvMod ? cvmod_clock_mod_index : base_clock_mod_index; }
    bool isCvModActive() const { return cv1_dest != CV_DEST_NONE || cv2_dest != CV_DEST_NONE; }

    byte getSteps(bool withCvMod = false) const { return withCvMod ? pattern.GetSteps() : base_euc_steps; }
    byte getHits(bool withCvMod = false) const { return withCvMod ? pattern.GetHits() : base_euc_hits; }

    /**
     * @brief Processes a clock tick and determines if the output should be high or low.
     * Note: this method is called from an ISR and must be kept as simple as possible.
     * @param tick The current clock tick count.
     * @param output The output object to be modified.
     */
    void processClockTick(uint32_t tick, DigitalOutput& output) {
        // Calculate output duty cycle state using cv modded values to determine pulse counts.
        const uint16_t mod_pulses = pgm_read_word_near(&CLOCK_MOD_PULSES[cvmod_clock_mod_index]);
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
        const uint32_t duty_cycle_end_tick = tick + duty_pulses + offset_pulses + swing_pulses;
        if (duty_cycle_end_tick % mod_pulses == 0) {
            output.Low();
        }
    }

    void applyCvMod(int cv1_val, int cv2_val) {
        // Calculate and store cv modded values using bipolar mapping.
        // Default to base value if not the current CV destination.

        // Note: This is optimized for cpu performance. This method is called
        // from the main loop and stores the cv mod values. This reduces CPU
        // cycles inside the internal clock interrupt, which is preferrable.
        // However, if RAM usage grows too much, we have an opportunity to
        // refactor this to store just the CV read values, and calculate the
        // cv mod value per channel inside the getter methods by passing cv
        // values. This would reduce RAM usage, but would introduce a
        // significant CPU cost, which may have undesirable performance issues.

        int dest_mod = calculateMod(CV_DEST_MOD, cv1_val, cv2_val, -10, 10);
        cvmod_clock_mod_index = constrain(base_clock_mod_index + dest_mod, 0, 100);

        int prob_mod = calculateMod(CV_DEST_PROB, cv1_val, cv2_val, -50, 50);
        cvmod_probability = constrain(base_probability + prob_mod, 0, 100);

        int duty_mod = calculateMod(CV_DEST_DUTY, cv1_val, cv2_val, -50, 50);
        cvmod_duty_cycle = constrain(base_duty_cycle + duty_mod, 1, 99);

        int offset_mod = calculateMod(CV_DEST_OFFSET, cv1_val, cv2_val, -50, 50);
        cvmod_offset = constrain(base_offset + offset_mod, 0, 99);

        int swing_mod = calculateMod(CV_DEST_SWING, cv1_val, cv2_val, -25, 25);
        cvmod_swing = constrain(base_swing + swing_mod, 50, 95);

        int step_mod = calculateMod(CV_DEST_EUC_STEPS, cv1_val, cv2_val, 0, MAX_PATTERN_LEN);
        pattern.SetSteps(base_euc_steps + step_mod);

        int hit_mod = calculateMod(CV_DEST_EUC_HITS, cv1_val, cv2_val, 0, MAX_PATTERN_LEN);
        pattern.SetHits(base_euc_hits + hit_mod);
    }

   private:
    int calculateMod(CvDestination dest, int cv1_val, int cv2_val, int min_range, int max_range) {
        int mod1 = (cv1_dest == dest) ? map(cv1_val, -512, 512, min_range, max_range) : 0;
        int mod2 = (cv2_dest == dest) ? map(cv2_val, -512, 512, min_range, max_range) : 0;
        return mod1 + mod2;
    }
    // User-settable base values.
    byte base_clock_mod_index;
    byte base_probability;
    byte base_duty_cycle;
    byte base_offset;
    byte base_swing;
    byte base_euc_steps;
    byte base_euc_hits;

    // Base value with cv mod applied.
    byte cvmod_clock_mod_index;
    byte cvmod_probability;
    byte cvmod_duty_cycle;
    byte cvmod_offset;
    byte cvmod_swing;

    // CV mod configuration
    CvDestination cv1_dest;
    CvDestination cv2_dest;

    // Euclidean pattern
    Pattern pattern;
};

#endif  // CHANNEL_H
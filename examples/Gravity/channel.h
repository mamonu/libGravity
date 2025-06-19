#ifndef CHANNEL_H
#define CHANNEL_H

#include <Arduino.h>
#include <gravity.h>

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
    CV_DEST_LAST,
};

static const int MOD_CHOICE_SIZE = 21;
// Negative for multiply, positive for divide.
static const int clock_mod[MOD_CHOICE_SIZE] = {-24, -12, -8, -6, -4, -3, -2, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32, 64, 128};
// This represents the number of clock pulses for a 96 PPQN clock source that match the above div/mult mods.
static const int clock_mod_pulses[MOD_CHOICE_SIZE] = {4, 8, 12, 16, 24, 32, 48, 96, 192, 288, 384, 480, 576, 1152, 672, 768, 1536, 2304, 3072, 6144, 12288};

static const int8_t shuffle_size = 2;

// MPC60 groove signatures?
static const int8_t shuffle_54[2] = {0, 2};
static const int8_t shuffle_58[2] = {0, 4};
static const int8_t shuffle_62[2] = {0, 6};
static const int8_t shuffle_66[2] = {0, 8};
static const int8_t shuffle_71[2] = {0, 10};
static const int8_t shuffle_75[2] = {0, 12};

// SWING Groove
static const int8_t swing_54[2] = {0, 1};
static const int8_t swing_58[2] = {-1, 1};
static const int8_t swing_62[2] = {-1, 2};
static const int8_t swing_66[2] = {-2, 2};
static const int8_t swing_71[2] = {-2, 3};
static const int8_t swing_75[2] = {-3, 3};

// static const String shuffle_name[6] = {"OFF", "54%", "58%", "62%", "66%", "71%"};
static const uint8_t SHUFFLE_SIZE = 6;
static const byte shuffle_amount[SHUFFLE_SIZE] = {54, 58, 62, 66, 71, 75};
static const int8_t* shuffle_templates[SHUFFLE_SIZE] = {shuffle_54, shuffle_58, shuffle_62, shuffle_66, shuffle_71, shuffle_75};
// static const int8_t* shuffle_templates[SHUFFLE_SIZE] = {swing_54, swing_58, swing_62, swing_66, swing_71, swing_75};

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
        cv_source = CV_NONE;
        cv_destination = CV_DEST_NONE;
        shuffle_index = 0;
        step_count = 0;

        cvmod_clock_mod_index = base_clock_mod_index;
        cvmod_probability = base_probability;
        cvmod_duty_cycle = base_duty_cycle;
        cvmod_offset = base_offset;
    }

    // Setters (Set the BASE value)

    void setClockMod(int index) {
        if (index >= 0 && index < MOD_CHOICE_SIZE) base_clock_mod_index = index;
    }
    void setProbability(int prob) { base_probability = constrain(prob, 0, 100); }
    void setDutyCycle(int duty) { base_duty_cycle = constrain(duty, 1, 99); }
    void setOffset(int off) { base_offset = constrain(off, 0, 100); }
    void setShuffleIndex(int val) { shuffle_index = constrain(val, 0, SHUFFLE_SIZE - 1); }
    void setCvSource(CvSource source) { cv_source = source; }
    void setCvDestination(CvDestination dest) { cv_destination = dest; }

    // Getters (Get the BASE value for editing or cv modded value for display)

    int getProbability(bool withCvMod = false) const { return withCvMod ? cvmod_probability : base_probability; }
    int getDutyCycle(bool withCvMod = false) const { return withCvMod ? cvmod_duty_cycle : base_duty_cycle; }
    int getOffset(bool withCvMod = false) const { return withCvMod ? cvmod_offset : base_offset; }
    int getShuffleIndex() const { return shuffle_index; }
    int getClockMod(bool withCvMod = false) const { return clock_mod[getClockModIndex(withCvMod)]; }
    int getClockModIndex(bool withCvMod = false) const { return withCvMod ? cvmod_clock_mod_index : base_clock_mod_index; }
    CvSource getCvSource() { return cv_source; }
    CvDestination getCvDestination() { return cv_destination; }
    bool isCvModActive() const { return cv_source != CV_NONE && cv_destination != CV_DEST_NONE; }
    int getStepCount() {return step_count;}

    /**
     * @brief Processes a clock tick and determines if the output should be high or low.
     * @param tick The current clock tick count.
     * @param output The output object to be modified.
     */
    void processClockTick(uint32_t tick, DigitalOutput& output) {
        // Calculate output duty cycle state using cv modded values to determine pulse counts.
        const uint32_t mod_pulses = clock_mod_pulses[cvmod_clock_mod_index];
        const uint32_t duty_pulses = max((long)((mod_pulses * (100L - cvmod_duty_cycle)) / 100L), 1L);
        const uint32_t offset_pulses = (long)((mod_pulses * (100L - cvmod_offset)) / 100L);

        uint32_t shuffle_pulses = 0;
        if (step_count % 2 == 0) {
            // shuffle_pulses = (long)((mod_pulses * (100L - shuffle_amount[shuffle_index])) / 100L);
            shuffle_pulses = 4 * shuffle_templates[shuffle_index][1];
        }

        const uint32_t current_tick_offset = tick + offset_pulses + shuffle_pulses;

        // Step check
        // TODO: Why is this incrementing twice?
        if (current_tick_offset % mod_pulses == 0) {
            // Duty cycle high check
            if (cvmod_probability >= random(0, 100)) {
                step_count += 1;
                output.High();
            }
        }

        // Duty cycle low check
        const uint32_t duty_cycle_end_tick = tick + duty_pulses + offset_pulses + shuffle_pulses;
        if (duty_cycle_end_tick % mod_pulses == 0) {
            output.Low();
        }
    }

    void applyCvMod(int cv1_value, int cv2_value) {
        if (!isCvModActive()) {
            // If CV is off, ensure cv modded values match the base values.
            cvmod_clock_mod_index = base_clock_mod_index;
            cvmod_probability = base_probability;
            cvmod_duty_cycle = base_duty_cycle;
            cvmod_offset = base_offset;
            return;
        }

        // Use the CV value for current selected cv source.
        int value = (cv_source == CV_1) ? cv1_value : cv2_value;

        // Calculate and store cv modded values using bipolar mapping.
        // Default to base value if not the current CV destination.

        cvmod_clock_mod_index = (cv_destination == CV_DEST_MOD)
                                    ? constrain(base_clock_mod_index + map(value, -512, 512, -10, 10), 0, MOD_CHOICE_SIZE - 1)
                                    : base_clock_mod_index;

        cvmod_probability = (cv_destination == CV_DEST_PROB)
                                ? constrain(base_probability + map(value, -512, 512, -50, 50), 0, 100)
                                : base_probability;

        cvmod_duty_cycle = (cv_destination == CV_DEST_DUTY)
                               ? constrain(base_duty_cycle + map(value, -512, 512, -50, 50), 1, 99)
                               : base_duty_cycle;

        cvmod_offset = (cv_destination == CV_DEST_OFFSET)
                           ? constrain(base_offset + map(value, -512, 512, -50, 50), 0, 99)
                           : base_offset;
    }

   private:
    uint32_t step_count;

    // User-settable base values.
    byte base_clock_mod_index;
    byte base_probability;
    byte base_duty_cycle;
    byte base_offset;
    byte shuffle_index;

    // Base value with cv mod applied.
    byte cvmod_clock_mod_index;
    byte cvmod_probability;
    byte cvmod_duty_cycle;
    byte cvmod_offset;

    // CV configuration
    CvSource cv_source = CV_NONE;
    CvDestination cv_destination = CV_DEST_NONE;
};

#endif  // CHANNEL_H
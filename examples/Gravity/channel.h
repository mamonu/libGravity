#ifndef CHANNEL_H
#define CHANNEL_H

#include <Arduino.h>
#include <gravity.h>

// Enums for CV configuration (still needed)
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

class Channel {
   public:
    Channel() {
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
    void setCvSource(CvSource source) { cv_source = source; }
    void setCvDestination(CvDestination dest) { cv_destination = dest; }

    // Getters (Get the BASE value for the UI)

    int getProbability(bool withCvMod = false) const { return withCvMod ? cvmod_probability : base_probability; }
    int getDutyCycle(bool withCvMod = false) const { return withCvMod ? cvmod_duty_cycle : base_duty_cycle; }
    int getOffset(bool withCvMod = false) const { return withCvMod ? cvmod_offset : base_offset; }
    int getClockMod(bool withCvMod = false) const { return clock_mod[getClockModIndex(withCvMod)]; }
    int getClockModIndex(bool withCvMod = false) const { return withCvMod ? cvmod_clock_mod_index : base_clock_mod_index; }
    uint32_t getDutyCyclePulses() const { return duty_cycle_pulses; }
    uint32_t getOffsetPulses() const { return offset_pulses; }
    CvSource getCvSource() { return cv_source; }
    CvDestination getCvDestination() { return cv_destination; }
    bool isCvModActive() const { return cv_source != CV_NONE && cv_destination != CV_DEST_NONE; }

    /**
     * @brief Processes a clock tick and determines if the output should be high or low.
     * @param tick The current clock tick count.
     * @param output The output object (or a reference to its state) to be modified.
     */
    void processClockTick(uint32_t tick, DigitalOutput& output) {
        // Use pre-calculated final values
        const uint32_t mod_pulses = clock_mod_pulses[cvmod_clock_mod_index];
        const uint32_t duty_pulses = max((long)((mod_pulses * (100L - cvmod_duty_cycle)) / 100L), 1L);
        const uint32_t offset_pulses = (long)((mod_pulses * (100L - cvmod_offset)) / 100L);

        const uint32_t current_tick_offset = tick + offset_pulses;

        // Duty cycle high check
        if (current_tick_offset % mod_pulses == 0) {
            if (cvmod_probability >= random(0, 100)) {
                output.High();
            }
        }

        // Duty cycle low check
        const uint32_t duty_cycle_end_tick = tick + duty_pulses + offset_pulses;
        if (duty_cycle_end_tick % mod_pulses == 0) {
            output.Low();
        }
    }

    void updateFinalValues(int cv1_value, int cv2_value) {
        if (!isCvModActive()) {
            // If CV is off, ensure final values match the base values.
            cvmod_clock_mod_index = base_clock_mod_index;
            cvmod_probability = base_probability;
            cvmod_duty_cycle = base_duty_cycle;
            cvmod_offset = base_offset;
            return;
        }

        // The channel knows its own config, so it selects the correct CV value.
        int active_cv_value = (cv_source == CV_1) ? cv1_value : cv2_value;

        // Calculate and store final values using bipolar mapping.
        // Default to base value if not the current CV destination.

        cvmod_clock_mod_index = (cv_destination == CV_DEST_MOD)
                                    ? constrain(base_clock_mod_index + map(active_cv_value, -512, 512, -10, 10), 0, MOD_CHOICE_SIZE - 1)
                                    : base_clock_mod_index;

        cvmod_probability = (cv_destination == CV_DEST_PROB)
                                ? constrain(base_probability + map(active_cv_value, -512, 512, -50, 50), 0, 100)
                                : base_probability;

        cvmod_duty_cycle = (cv_destination == CV_DEST_DUTY)
                               ? constrain(base_duty_cycle + map(active_cv_value, -512, 512, -50, 50), 0, 99)
                               : base_duty_cycle;

        cvmod_offset = (cv_destination == CV_DEST_OFFSET)
                           ? constrain(base_offset + map(active_cv_value, -512, 512, -50, 50), 0, 99)
                           : base_offset;
    }

   private:
    /**
     * @brief Recalculates pulse values based on current channel settings.
     * Should be called whenever mod, duty cycle, or offset changes.
     */
    void updatePulses() {
        const uint32_t mod_pulses = clock_mod_pulses[cvmod_clock_mod_index];
        duty_cycle_pulses = max((long)((mod_pulses * (100L - cvmod_duty_cycle)) / 100L), 1L);
        offset_pulses = (long)((mod_pulses * (100L - cvmod_offset)) / 100L);
    }

    // User-settable "base" values.
    byte base_clock_mod_index = 7;
    byte base_probability = 100;
    byte base_duty_cycle = 50;
    byte base_offset = 0;

    int duty_cycle_pulses;
    int offset_pulses;

    // CV configuration
    CvSource cv_source = CV_NONE;
    CvDestination cv_destination = CV_DEST_NONE;

    volatile byte cvmod_clock_mod_index;
    volatile byte cvmod_probability;
    volatile byte cvmod_duty_cycle;
    volatile byte cvmod_offset;
};

#endif  // CHANNEL_H
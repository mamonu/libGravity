#ifndef CHANNEL_H
#define CHANNEL_H

#include <Arduino.h>
#include <gravity.h>


static const int MOD_CHOICE_SIZE = 21;
// Negative for multiply, positive for divide.
static const int clock_mod[MOD_CHOICE_SIZE] = {-24, -12, -8, -6, -4, -3, -2, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32, 64, 128};
// This represents the number of clock pulses for a 96 PPQN clock source that match the above div/mult mods.
static const int clock_mod_pulses[MOD_CHOICE_SIZE] = {4, 8, 12, 16, 24, 32, 48, 96, 192, 288, 384, 480, 576, 1152, 672, 768, 1536, 2304, 3072, 6144, 12288};


class Channel {
public:
    /**
     * @brief Construct a new Channel object with default values.
     */
    Channel() {
        updatePulses();
    }

    // --- Setters for channel properties ---

    void setClockMod(int index) {
        if (index >= 0 && index < MOD_CHOICE_SIZE) {
            clock_mod_index = index;
            updatePulses();
        }
    }

    void setProbability(int prob) {
        probability = constrain(prob, 0, 100);
    }

    void setDutyCycle(int duty) {
        duty_cycle = constrain(duty, 0, 99);
        updatePulses();
    }

    void setOffset(int off) {
        offset = constrain(off, 0, 99);
        updatePulses();
    }

    // --- Getters for channel properties ---

    int getClockModIndex() const { return clock_mod_index; }
    int getProbability() const { return probability; }
    int getDutyCycle() const { return duty_cycle; }
    int getOffset() const { return offset; }
    int getClockMod() const { return clock_mod[clock_mod_index]; }
    uint32_t getDutyCyclePulses() const { return duty_cycle_pulses; }
    uint32_t getOffsetPulses() const { return offset_pulses; }

    /**
     * @brief Processes a clock tick and determines if the output should be high or low.
     * @param tick The current clock tick count.
     * @param output The output object (or a reference to its state) to be modified.
     */
    void processClockTick(uint32_t tick, DigitalOutput& output) {
        const uint32_t mod_pulses = clock_mod_pulses[clock_mod_index];
        const uint32_t current_tick_offset = tick + offset_pulses;

        // Duty cycle high check
        if (current_tick_offset % mod_pulses == 0) {
            if (probability >= random(0, 100)) {
                output.High();
            }
        }

        // Duty cycle low check
        const uint32_t duty_cycle_end_tick = tick + duty_cycle_pulses + offset_pulses;
        if (duty_cycle_end_tick % mod_pulses == 0) {
            output.Low();
        }
    }

private:
    /**
     * @brief Recalculates pulse values based on current channel settings.
     * Should be called whenever mod, duty cycle, or offset changes.
     */
    void updatePulses() {
        uint32_t mod_pulses = clock_mod_pulses[clock_mod_index];
        duty_cycle_pulses = max((long)((mod_pulses * (100L - duty_cycle)) / 100L), 1L);
        offset_pulses = (long)((mod_pulses * (100L - offset)) / 100L);
    }

    byte clock_mod_index = 7;  // 1x
    byte probability = 100;
    byte duty_cycle = 50;
    byte offset = 0;
    int duty_cycle_pulses;
    int offset_pulses;
};

#endif // CHANNEL_H
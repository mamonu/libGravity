/**
 * @file clock.h
 * @author Adam Wonak (https://github.com/awonak)
 * @brief Wrapper Class for clock timing functions.
 * @version 0.1
 * @date 2025-05-04
 *
 * @copyright MIT - (c) 2025 - Adam Wonak - adam.wonak@gmail.com
 *
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <NeoHWSerial.h>

#include "peripherials.h"
#include "uClock/uClock.h"

// MIDI clock, start, stop, and continue byte definitions - based on MIDI 1.0 Standards.
#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_STOP 0xFC
#define MIDI_CONTINUE 0xFB

typedef void (*ExtCallback)(void);
static ExtCallback extUserCallback = nullptr;
static void serialEventNoop(uint8_t msg, uint8_t status) {}

/**
 * @brief Wrapper Class for clock timing functions.
 */
class Clock {
   public:
    static constexpr int DEFAULT_TEMPO = 120;

    enum Source {
        SOURCE_INTERNAL,
        SOURCE_EXTERNAL_PPQN_24,
        SOURCE_EXTERNAL_PPQN_4,
        SOURCE_EXTERNAL_MIDI,
        SOURCE_LAST,
    };

    enum Pulse {
        PULSE_NONE,
        PULSE_PPQN_1,
        PULSE_PPQN_4,
        PULSE_PPQN_24,
        PULSE_LAST,
    };

    /**
     * @brief Initializes the clock, MIDI serial, and sets default values.
     */
    void Init() {
        NeoSerial.begin(31250);

        // Initialize the clock library
        uClock.init();
        uClock.setClockMode(uClock.INTERNAL_CLOCK);
        uClock.setOutputPPQN(uClock.PPQN_96);
        uClock.setTempo(DEFAULT_TEMPO);

        // MIDI events.
        uClock.setOnClockStart(sendMIDIStart);
        uClock.setOnClockStop(sendMIDIStop);
        uClock.setOnSync24(sendMIDIClock);

        uClock.start();
    }

    /**
     * @brief Attach a handler for external clock ticks.
     *
     * This function attaches a user-defined callback to the external clock input pin interrupt.
     * It is also called for incoming MIDI clock events.
     *
     * @param callback Function to call on an external clock event.
     */
    void AttachExtHandler(void (*callback)()) {
        extUserCallback = callback;
        attachInterrupt(digitalPinToInterrupt(EXT_PIN), callback, RISING);
    }

    /**
     * @brief Attach a handler for the internal high-resolution clock.
     *
     * Sets a callback function that is triggered at the internal PPQN_96 rate. This is the
     * main internal timing callback for all clock operations.
     *
     * @param callback Function to call on every internal clock tick. It receives the tick count as a parameter.
     */
    void AttachIntHandler(void (*callback)(uint32_t)) {
        uClock.setOnOutputPPQN(callback);
    }

    /**
     * @brief Set the source of the clock.
     *
     * @param source The new source for driving the clock. See the `Source` enum.
     */
    void SetSource(Source source) {
        bool was_playing = !IsPaused();
        uClock.stop();
        // If we are changing the source from MIDI, disable the serial interrupt handler.
        if (source_ == SOURCE_EXTERNAL_MIDI) {
            NeoSerial.attachInterrupt(serialEventNoop);
        }
        source_ = source;
        switch (source) {
            case SOURCE_INTERNAL:
                uClock.setClockMode(uClock.INTERNAL_CLOCK);
                break;
            case SOURCE_EXTERNAL_PPQN_24:
                uClock.setClockMode(uClock.EXTERNAL_CLOCK);
                uClock.setInputPPQN(uClock.PPQN_24);
                break;
            case SOURCE_EXTERNAL_PPQN_4:
                uClock.setClockMode(uClock.EXTERNAL_CLOCK);
                uClock.setInputPPQN(uClock.PPQN_4);
                break;
            case SOURCE_EXTERNAL_MIDI:
                uClock.setClockMode(uClock.EXTERNAL_CLOCK);
                uClock.setInputPPQN(uClock.PPQN_24);
                NeoSerial.attachInterrupt(onSerialEvent);
                break;
        }
        if (was_playing) {
            uClock.start();
        }
    }

    /**
     * @brief Checks if the clock source is external.
     *
     * @return true if the current source is external (PPQN_4, PPQN_24, or MIDI).
     * @return false if the source is internal.
     */
    bool ExternalSource() {
        return uClock.getClockMode() == uClock.EXTERNAL_CLOCK;
    }

    /**
     * @brief Checks if the clock source is internal.
     *
     * @return true if the current source is the internal master clock.
     * @return false if the source is external.
     */
    bool InternalSource() {
        return uClock.getClockMode() == uClock.INTERNAL_CLOCK;
    }

    /**
     * @brief Gets the current tempo.
     *
     * @return int The current tempo in beats per minute (BPM).
     */
    int Tempo() {
        return uClock.getTempo();
    }

    /**
     * @brief Set the clock tempo.
     *
     * @param tempo The new tempo in beats per minute (BPM).
     */
    void SetTempo(int tempo) {
        return uClock.setTempo(tempo);
    }

    /**
     * @brief Manually trigger a clock tick.
     *
     * This should be called when in an external clock mode to register an incoming
     * clock pulse and drive the internal timing.
     */
    void Tick() {
        uClock.clockMe();
    }

    /**
     * @brief Starts the clock.
     */
    void Start() {
        uClock.start();
    }

    /**
     * @brief Stops (pauses) the clock.
     */
    void Stop() {
        uClock.stop();
    }

    /**
     * @brief Resets all clock counters to zero.
     */
    void Reset() {
        uClock.resetCounters();
    }

    /**
     * @brief Checks if the clock is currently paused.
     *
     * @return true if the clock is stopped/paused.
     * @return false if the clock is running.
     */
    bool IsPaused() {
        return uClock.clock_state == uClock.PAUSED;
    }

   private:
    Source source_ = SOURCE_INTERNAL;

    static void onSerialEvent(uint8_t msg, uint8_t status) {
        // Note: uClock start and stop will echo to MIDI.
        switch (msg) {
            case MIDI_CLOCK:
                if (extUserCallback) {
                    extUserCallback();
                }
                break;
            case MIDI_STOP:
                uClock.stop();
                sendMIDIStop();
                break;
            case MIDI_START:
            case MIDI_CONTINUE:
                uClock.start();
                sendMIDIStart();
                break;
        }
    }

    static void sendMIDIStart() {
        NeoSerial.write(MIDI_START);
    }

    static void sendMIDIStop() {
        NeoSerial.write(MIDI_STOP);
    }

    static void sendMIDIClock(uint32_t tick) {
        NeoSerial.write(MIDI_CLOCK);
    }
};

#endif
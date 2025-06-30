/**
 * @file clock.h
 * @author Adam Wonak (https://github.com/awonak)
 * @brief Wrapper Class for clock timing functions.
 * @version 0.1
 * @date 2025-05-04
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <NeoHWSerial.h>
#include <uClock.h>

#include "peripherials.h"

// MIDI clock, start, stop, and continue byte definitions - based on MIDI 1.0 Standards.
#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_STOP 0xFC
#define MIDI_CONTINUE 0xFB

typedef void (*ExtCallback)(void);
static ExtCallback extUserCallback = nullptr;
static void serialEventNoop(uint8_t msg, uint8_t status) {}

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

    void Init() {
        NeoSerial.begin(31250);

        // Static pin definition for pulse out.
        pinMode(PULSE_OUT_PIN, OUTPUT);

        // Initialize the clock library
        uClock.init();
        uClock.setClockMode(uClock.INTERNAL_CLOCK);
        uClock.setOutputPPQN(uClock.PPQN_96);
        uClock.setTempo(DEFAULT_TEMPO);

        // MIDI events.
        uClock.setOnClockStart(sendMIDIStart);
        uClock.setOnClockStop(sendMIDIStop);
        // uClock.setOnSync24(sendMIDIClock);
        // uClock.setOnSync48(sendPulseOut);

        uClock.start();
    }

    // Handle external clock tick and call user callback when receiving clock trigger (PPQN_4, PPQN_24, or MIDI).
    void AttachExtHandler(void (*callback)()) {
        extUserCallback = callback;
        attachInterrupt(digitalPinToInterrupt(EXT_PIN), callback, RISING);
    }

    // Internal PPQN96 callback for all clock timer operations.
    void AttachIntHandler(void (*callback)(uint32_t)) {
        uClock.setOnOutputPPQN(callback);
    }

    // Set the source of the clock mode.
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

    // Return true if the current selected source is externl (PPQN_4, PPQN_24, or MIDI).
    bool ExternalSource() {
        return uClock.getClockMode() == uClock.EXTERNAL_CLOCK;
    }

    // Return true if the current selected source is the internal master clock.
    bool InternalSource() {
        return uClock.getClockMode() == uClock.INTERNAL_CLOCK;
    }

    // Returns the current BPM tempo.
    int Tempo() {
        return uClock.getTempo();
    }

    // Set the clock tempo to a int between 1 and 400.
    void SetTempo(int tempo) {
        return uClock.setTempo(tempo);
    }

    // Record an external clock tick received to process external/internal syncronization.
    void Tick() {
        uClock.clockMe();
    }

    // Start the internal clock.
    void Start() {
        uClock.start();
    }

    // Stop internal clock clock.
    void Stop() {
        uClock.stop();
    }

    // Returns true if the clock is not running.
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
                break;
            case MIDI_START:
            case MIDI_CONTINUE:
                uClock.start();
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

    static void sendPulseOut(uint32_t tick) {
        digitalWrite(PULSE_OUT_PIN, !digitalRead(PULSE_OUT_PIN));
    }
};

#endif
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

#include <uClock.h>

#include "peripherials.h"

const int DEFAULT_TEMPO = 120;

enum Source {
    SOURCE_INTERNAL,
    SOURCE_EXTERNAL_PPQN_24,
    SOURCE_EXTERNAL_PPQN_4,
    // SOURCE_MIDI,
    SOURCE_LAST,
};

class Clock {
   public:
    void Init() {
        // Initialize the clock library
        uClock.init();
        uClock.setClockMode(uClock.INTERNAL_CLOCK);
        uClock.setOutputPPQN(uClock.PPQN_96);
        uClock.setTempo(DEFAULT_TEMPO);
        uClock.start();
    }

    // Handler for receiving clock trigger(PPQN_4 or PPQN_24).
    void AttachExtHandler(void (*callback)(void)) {
        attachInterrupt(digitalPinToInterrupt(EXT_PIN), callback, RISING);
    }

    // Internal PPQN96 callback for all clock timer operations.
    void AttachIntHandler(void (*callback)(uint32_t)) {
        uClock.setOnOutputPPQN(callback);
    }

    // Set the source of the clock mode.
    void SetSource(Source source) {
        switch (source) {
            case SOURCE_INTERNAL:
                uClock.setClockMode(uClock.INTERNAL_CLOCK);
                break;
            case SOURCE_EXTERNAL_PPQN_24:
                uClock.setClockMode(uClock.EXTERNAL_CLOCK);
                uClock.setInputPPQN(uClock.PPQN_24);
            case SOURCE_EXTERNAL_PPQN_4:
                uClock.setClockMode(uClock.EXTERNAL_CLOCK);
                uClock.setInputPPQN(uClock.PPQN_4);
            default:
                break;
        }
    }

    bool ExternalSource() {
        return uClock.getClockMode() == uClock.EXTERNAL_CLOCK;
    }

    bool InternalSource() {
        return uClock.getClockMode() == uClock.INTERNAL_CLOCK;
    }

    int Tempo() {
        return uClock.getTempo();
    }

    void SetTempo(int tempo) {
        return uClock.setTempo(tempo);
    }

    void Tick() {
        uClock.clockMe();
    }

    void Pause() {
        uClock.pause();
    }

    bool IsPaused() {
        return uClock.clock_state == uClock.PAUSED;
    }
};

#endif
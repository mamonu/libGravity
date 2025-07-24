/*!
 *  @file       uClock.cpp
 *  Project     BPM clock generator for Arduino
 *  @brief      A Library to implement BPM clock tick calls using hardware interruption. Supported and tested on AVR boards(ATmega168/328, ATmega16u4/32u4 and ATmega2560) and ARM boards(RPI2040, Teensy, Seedstudio XIAO M0 and ESP32)
 *  @version    2.2.1
 *  @author     Romulo Silva
 *  @date       10/06/2017
 *  @license    MIT - (c) 2024 - Romulo Silva - contact@midilab.co
 * 
 *  2025-06-30 - https://github.com/awonak/uClock/tree/picoClock
 *               Modified by awonak to remove all unused sync callback
 *               methods and associated variables to dramatically reduce
 *               memory usage. 
 *               See: https://github.com/midilab/uClock/issues/58
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "uClock.h"
#include "platforms/avr.h"

//
// Platform specific timer setup/control
//
// initTimer(uint32_t us_interval) and setTimer(uint32_t us_interval)
// are called from architecture specific module included at the
// header of this file
void uclockInitTimer()
{
    // begin at 120bpm
    initTimer(uClock.bpmToMicroSeconds(120.00));
}

void setTimerTempo(float bpm)
{
    setTimer(uClock.bpmToMicroSeconds(bpm));
}

namespace umodular { namespace clock {

static inline uint32_t phase_mult(uint32_t val)
{
    return (val * PHASE_FACTOR) >> 8;
}

static inline uint32_t clock_diff(uint32_t old_clock, uint32_t new_clock)
{
    if (new_clock >= old_clock) {
        return new_clock - old_clock;
    } else {
        return new_clock + (4294967295 - old_clock);
    }
}

uClockClass::uClockClass()
{
    tempo = 120;
    start_timer = 0;
    last_interval = 0;
    sync_interval = 0;
    clock_state = PAUSED;
    clock_mode = INTERNAL_CLOCK;
    resetCounters();

    onOutputPPQNCallback = nullptr;
    onSync24Callback = nullptr;
    onClockStartCallback = nullptr;
    onClockStopCallback = nullptr;
    // initialize reference data
    calculateReferencedata();
}

void uClockClass::init()
{
    if (ext_interval_buffer == nullptr)
        setExtIntervalBuffer(1);

    uclockInitTimer();
    // first interval calculus
    setTempo(tempo);
}

uint32_t uClockClass::bpmToMicroSeconds(float bpm)
{
    return (60000000.0f / (float)output_ppqn / bpm);
}

void uClockClass::calculateReferencedata()
{
    mod_clock_ref = output_ppqn / input_ppqn;
    mod_sync24_ref = output_ppqn / PPQN_24;
}

void uClockClass::setOutputPPQN(PPQNResolution resolution)
{
    // dont allow PPQN lower than PPQN_4 for output clock (to avoid problems with mod_step_ref)
    if (resolution < PPQN_4)
        return;

    ATOMIC(
        output_ppqn = resolution;
        calculateReferencedata();
    )
}

void uClockClass::setInputPPQN(PPQNResolution resolution)
{
    ATOMIC(
        input_ppqn = resolution;
        calculateReferencedata();
    )
}

void uClockClass::start()
{
    resetCounters();
    start_timer = millis();

    if (onClockStartCallback) {
        onClockStartCallback();
    }

    if (clock_mode == INTERNAL_CLOCK) {
        clock_state = STARTED;
    } else {
        clock_state = STARTING;
    }
}

void uClockClass::stop()
{
    clock_state = PAUSED;
    start_timer = 0;
    resetCounters();
    if (onClockStopCallback) {
        onClockStopCallback();
    }
}

void uClockClass::pause()
{
    if (clock_mode == INTERNAL_CLOCK) {
        if (clock_state == PAUSED) {
            start();
        } else {
            stop();
        }
    }
}

void uClockClass::setTempo(float bpm)
{
    if (clock_mode == EXTERNAL_CLOCK) {
        return;
    }

    if (bpm < MIN_BPM || bpm > MAX_BPM) {
        return;
    }

    ATOMIC(
        tempo = bpm
    )

    setTimerTempo(bpm);
}

float uClockClass::getTempo()
{
    if (clock_mode == EXTERNAL_CLOCK) {
        uint32_t acc = 0;
        // wait the buffer to get full
        if (ext_interval_buffer[ext_interval_buffer_size-1] == 0) {
            return tempo;
        }
        for (uint8_t i=0; i < ext_interval_buffer_size; i++) {
            acc += ext_interval_buffer[i];
        }
        if (acc != 0) {
            return constrainBpm(freqToBpm(acc / ext_interval_buffer_size));
        }
    }
    return tempo;
}

// for software timer implementation(fallback for no board support)
void uClockClass::run() {}

float inline uClockClass::freqToBpm(uint32_t freq)
{
    return 60000000.0f / (float)(freq * input_ppqn);
}

float inline uClockClass::constrainBpm(float bpm)
{
    return (bpm < MIN_BPM) ? MIN_BPM : ( bpm > MAX_BPM ? MAX_BPM : bpm );
}

void uClockClass::setClockMode(ClockMode tempo_mode)
{
    clock_mode = tempo_mode;
}

uClockClass::ClockMode uClockClass::getClockMode()
{
    return clock_mode;
}

void uClockClass::clockMe()
{
    if (clock_mode == EXTERNAL_CLOCK) {
        ATOMIC(
            handleExternalClock()
        )
    }
}

void uClockClass::setExtIntervalBuffer(uint8_t buffer_size)
{
    if (ext_interval_buffer != nullptr)
        return;

    // alloc once and forever policy
    ext_interval_buffer_size = buffer_size;
    ext_interval_buffer = (uint32_t*) malloc( sizeof(uint32_t) * ext_interval_buffer_size );
}

void uClockClass::resetCounters()
{
    tick = 0;
    int_clock_tick = 0;
    mod_clock_counter = 0;

    mod_sync24_counter = 0;
    sync24_tick = 0;
    
    ext_clock_tick = 0;
    ext_clock_us = 0;
    ext_interval_idx = 0;

    for (uint8_t i=0; i < ext_interval_buffer_size; i++) {
        ext_interval_buffer[i] = 0;
    }
}

void uClockClass::handleExternalClock()
{
    switch (clock_state) {
        case PAUSED:
            break;

        case STARTING:
            clock_state = STARTED;
            ext_clock_us = micros();
            break;

        case STARTED:
            uint32_t now_clock_us = micros();
            last_interval = clock_diff(ext_clock_us, now_clock_us);
            ext_clock_us = now_clock_us;

            // external clock tick me!
            ext_clock_tick++;

            // accumulate interval incomming ticks data for getTempo() smooth reads on slave clock_mode
            if(++ext_interval_idx >= ext_interval_buffer_size) {
                ext_interval_idx = 0;
            }
            ext_interval_buffer[ext_interval_idx] = last_interval;

            if (ext_clock_tick == 1) {
                ext_interval = last_interval;
            } else {
                ext_interval = (((uint32_t)ext_interval * (uint32_t)PLL_X) + (uint32_t)(256 - PLL_X) * (uint32_t)last_interval) >> 8;
            }
            break;
    }
}

void uClockClass::handleTimerInt()
{
    // track main input clock counter
    if (mod_clock_counter == mod_clock_ref)
        mod_clock_counter = 0;

    // process sync signals first please...
    if (mod_clock_counter == 0) {

        if (clock_mode == EXTERNAL_CLOCK) {
            // sync tick position with external tick clock
            if ((int_clock_tick < ext_clock_tick) || (int_clock_tick > (ext_clock_tick + 1))) {
                int_clock_tick = ext_clock_tick;
                tick = int_clock_tick * mod_clock_ref;
                mod_clock_counter = tick % mod_clock_ref;
            }

            uint32_t counter = ext_interval;
            uint32_t now_clock_us = micros();
            sync_interval = clock_diff(ext_clock_us, now_clock_us);

            if (int_clock_tick <= ext_clock_tick) {
                counter -= phase_mult(sync_interval);
            } else {
                if (counter > sync_interval) {
                    counter += phase_mult(counter - sync_interval);
                }
            }

            // update internal clock timer frequency
            float bpm = constrainBpm(freqToBpm(counter));
            if (bpm != tempo) {
                tempo = bpm;
                setTimerTempo(bpm);
            }
        }

        // internal clock tick me!
        ++int_clock_tick;
    }
    ++mod_clock_counter;
    
    // Sync24 callback
    if (onSync24Callback) {
        if (mod_sync24_counter == mod_sync24_ref)
            mod_sync24_counter = 0;
        if (mod_sync24_counter == 0) {
            onSync24Callback(sync24_tick);
            ++sync24_tick;
        }
        ++mod_sync24_counter;
    }

    // main PPQNCallback
    if (onOutputPPQNCallback) {
        onOutputPPQNCallback(tick);
        ++tick;
    }
}

// elapsed time support
uint8_t uClockClass::getNumberOfSeconds(uint32_t time)
{
    if ( time == 0 ) {
        return time;
    }
    return ((_millis - time) / 1000) % SECS_PER_MIN;
}

uint8_t uClockClass::getNumberOfMinutes(uint32_t time)
{
    if ( time == 0 ) {
        return time;
    }
    return (((_millis - time) / 1000) / SECS_PER_MIN) % SECS_PER_MIN;
}

uint8_t uClockClass::getNumberOfHours(uint32_t time)
{
    if ( time == 0 ) {
        return time;
    }
    return (((_millis - time) / 1000) % SECS_PER_DAY) / SECS_PER_HOUR;
}

uint8_t uClockClass::getNumberOfDays(uint32_t time)
{
    if ( time == 0 ) {
        return time;
    }
    return ((_millis - time) / 1000) / SECS_PER_DAY;
}

uint32_t uClockClass::getNowTimer()
{
    return _millis;
}

uint32_t uClockClass::getPlayTime()
{
    return start_timer;
}

} } // end namespace umodular::clock

umodular::clock::uClockClass uClock;

volatile uint32_t _millis = 0;

//
// TIMER HANDLER
//
void uClockHandler()
{
    // global timer counter
    _millis = millis();

    if (uClock.clock_state == uClock.STARTED) {
        uClock.handleTimerInt();
    }
}

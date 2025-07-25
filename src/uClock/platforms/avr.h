/*!
 *  @file       avr.h
 *  Project     BPM clock generator for Arduino
 *  @brief      A Library to implement BPM clock tick calls using hardware interruption. Supported and tested on AVR boards(ATmega168/328, ATmega16u4/32u4 and ATmega2560) and ARM boards(RPI2040, Teensy, Seedstudio XIAO M0 and ESP32)
 *  @version    2.2.1
 *  @author     Romulo Silva
 *  @date       10/06/2017
 *  @license    MIT - (c) 2024 - Romulo Silva - contact@midilab.co
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
#include <Arduino.h>

#define ATOMIC(X) noInterrupts(); X; interrupts();

// want a different avr clock support?
// TODO: we should do this using macro guards for avrs different clocks freqeuncy setup at compile time
#define AVR_CLOCK_FREQ	16000000

// forward declaration of uClockHandler
void uClockHandler();

// AVR ISR Entrypoint
ISR(TIMER1_COMPA_vect)
{
    uClockHandler();
}

void initTimer(uint32_t init_clock)
{
    ATOMIC(
        // 16bits Timer1 init
        // begin at 120bpm (48.0007680122882 Hz)
        TCCR1A = 0; // set entire TCCR1A register to 0
        TCCR1B = 0; // same for TCCR1B
        TCNT1  = 0; // initialize counter value to 0
        // set compare match register for 48.0007680122882 Hz increments
        OCR1A = 41665; // = 16000000 / (8 * 48.0007680122882) - 1 (must be <65536)
        // turn on CTC mode
        TCCR1B |= (1 << WGM12);
        // Set CS12, CS11 and CS10 bits for 8 prescaler
        TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
        // enable timer compare interrupt
        TIMSK1 |= (1 << OCIE1A);
    )
}

void setTimer(uint32_t us_interval)
{
    float tick_hertz_interval = 1/((float)us_interval/1000000);

    uint32_t ocr;
    uint8_t tccr = 0;

    // 16bits avr timer setup
    if ((ocr = AVR_CLOCK_FREQ / ( tick_hertz_interval * 1 )) < 65535) {
        // Set CS12, CS11 and CS10 bits for 1 prescaler
        tccr |= (0 << CS12) | (0 << CS11) | (1 << CS10);
    } else if ((ocr = AVR_CLOCK_FREQ / ( tick_hertz_interval * 8 )) < 65535) {
        // Set CS12, CS11 and CS10 bits for 8 prescaler
        tccr |= (0 << CS12) | (1 << CS11) | (0 << CS10);
    } else if ((ocr = AVR_CLOCK_FREQ / ( tick_hertz_interval * 64 )) < 65535) {
        // Set CS12, CS11 and CS10 bits for 64 prescaler
        tccr |= (0 << CS12) | (1 << CS11) | (1 << CS10);
    } else if ((ocr = AVR_CLOCK_FREQ / ( tick_hertz_interval * 256 )) < 65535) {
        // Set CS12, CS11 and CS10 bits for 256 prescaler
        tccr |= (1 << CS12) | (0 << CS11) | (0 << CS10);
    } else if ((ocr = AVR_CLOCK_FREQ / ( tick_hertz_interval * 1024 )) < 65535) {
        // Set CS12, CS11 and CS10 bits for 1024 prescaler
        tccr |= (1 << CS12) | (0 << CS11) | (1 << CS10);
    } else {
        // tempo not achiavable
        return;
    }

    ATOMIC(
        TCCR1B = 0;
        OCR1A = ocr-1;
        TCCR1B |= (1 << WGM12);
        TCCR1B |= tccr;
    )
}
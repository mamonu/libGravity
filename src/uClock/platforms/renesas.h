#pragma once

/**
 * @file nano_r4.h
 * @author Gemini (Based on the uClock AVR implementation)
 * @brief uClock platform support for the Arduino Nano R4 (Renesas RA4M1).
 *
 * This file implements the timer initialization and control functions
 * required by uClock using the FspTimer library, which provides a high-level
 * interface to the General PWM Timers (GPT) on the Renesas RA4M1
 * microcontroller. This approach replaces the direct register manipulation
 * used for AVR platforms.
 */

#include <Arduino.h>
#include <FspTimer.h>

// ATOMIC macro for defining critical sections where interrupts are disabled.
#define ATOMIC(X) noInterrupts(); X; interrupts();

// Forward declaration of the uClock's main handler function. This function
// must be defined in the main uClock library code and will be called by the timer interrupt.
void uClockHandler();

// Create an FspTimer instance for uClock.
// We use GPT channel 6, as it is less likely to conflict with the default
// analogWrite() (PWM) functionality on the Nano R4's pins.
FspTimer uClockTimer;

/**
 * @brief Initializes the hardware timer for uClock.
 *
 * This function configures and starts a hardware timer (GPT6) to fire
 * periodically. It attaches the uClockHandler as the interrupt service routine.
 * The initial tempo is set to a default of 120 BPM (48 Hz tick rate).
 *
 * @param init_clock This parameter is unused on this platform but is kept
 * for API compatibility with other uClock platforms.
 */
void initTimer(uint32_t init_clock)
{
    ATOMIC(
        // Configure the timer to be a periodic interrupt source.
        // The frequency/period arguments here are placeholders, as the actual
        // period is set precisely with the setPeriod() call below.
        uClockTimer.begin(TIMER_MODE_PERIODIC, GPT_TIMER, 6, 1.0f, STANDARD_PWM_FREQ_HZ);

        // Set the timer's period to the provided BPM period in microseconds.
        uClockTimer.set_period(init_clock);

        // Start the timer to begin generating ticks.
        uClockTimer.start();
    )
}

/**
 * @brief Sets the timer's interval in microseconds.
 *
 * This function dynamically updates the timer's period to match the specified
 * interval, which effectively changes the clock's tempo. The FspTimer library
 * automatically handles the complex low-level prescaler and counter adjustments.
 *
 * @param us_interval The desired interval between clock ticks in microseconds.
 */
void setTimer(uint32_t us_interval)
{
    // Atomically update the timer's period. The FspTimer library abstracts
    // away the manual prescaler math required on AVR platforms.
    ATOMIC(
        uClockTimer.set_period(us_interval);
    )
}
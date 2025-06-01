/**
 * @file peripherials.h
 * @author Adam Wonak (https://github.com/awonak)
 * @brief Arduino pin definitions for the Sitka Instruments Gravity module.
 * @version 0.1
 * @date 2025-04-19
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef PERIPHERIALS_H
#define PERIPHERIALS_H

// OLED Display config
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Peripheral input pins
#define ENCODER_PIN1 17  // A3
#define ENCODER_PIN2 4
#define ENCODER_SW_PIN 14

// Clock and CV Inputs
#define EXT_PIN 2
#define CV1_PIN A7
#define CV2_PIN A6

// Button pins
#define SHIFT_BTN_PIN 12
#define PLAY_BTN_PIN 5

// Output Pins
#define OUT_CH1 7
#define OUT_CH2 8
#define OUT_CH3 10
#define OUT_CH4 6
#define OUT_CH5 9
#define OUT_CH6 11

const uint8_t OUTPUT_COUNT = 6;

#endif

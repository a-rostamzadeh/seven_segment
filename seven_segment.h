/*
******************************************************************************
* @file    seven_segment.h
* @brief   Multiplexed 3-digit 7-segment display driver.
*
*          Drives a common-cathode (or common-anode, see SEG_ACTIVE_LOW)
*          3-digit multiplexed display using GPIO pins declared in main.h:
*
*              A_7S, B_7S, C_7S, D_7S, E_7S, F_7S, G_7S, P_7S
*              DIGIT1, DIGIT2, DIGIT3
*
*          Segments are named {DP,G,F,E,D,C,B,A} in the LSB-first bit order
*          used by SevenSegmentASCII[].
******************************************************************************
*/
#ifndef __SEVEN_SEGMENT_H
#define __SEVEN_SEGMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Configuration
 * ---------------------------------------------------------------------- */

/** Number of multiplexed digits on this display. */
#define SEG_NUM_DIGITS      3

/**
 * Set to 1 if the segment pins are active-LOW (common-anode display).
 * Set to 0 for common-cathode (active-HIGH) displays.
 */
#ifndef SEG_ACTIVE_LOW
#define SEG_ACTIVE_LOW      0
#endif

/**
 * Set to 1 if the digit-select pins are active-LOW.
 * Set to 0 if they are active-HIGH (typical when driven through a transistor).
 */
#ifndef SEG_DIGIT_ACTIVE_LOW
#define SEG_DIGIT_ACTIVE_LOW 0
#endif

/* -------------------------------------------------------------------------
 * Public state
 * ---------------------------------------------------------------------- */

/** ASCII characters to display, one per digit (e.g. '1', '2', '3'). */
extern volatile char digits[SEG_NUM_DIGITS];

/** Decimal-point flags, one per digit (0 = off, 1 = on). */
extern volatile char dots[SEG_NUM_DIGITS];

/** Index of the digit currently being multiplexed. */
extern volatile uint8_t active_digit;

/** ASCII → 7-segment lookup table (index 0 = ASCII 32 / space). */
extern const uint8_t SevenSegmentASCII[96];

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

/**
 * @brief  Convert a signed 16-bit number to ASCII digits in `digits[]`.
 *
 * Range is clamped to [-99, 999].  Leading zeros are replaced with spaces.
 * A negative number displays a '-' in the most significant non-blank digit.
 *
 * @param  number  Value to display.
 */
void seg_setNumber(int16_t number);

/**
 * @brief  Convert an unsigned 16-bit number to ASCII digits in `digits[]`.
 *
 * Range is clamped to [0, 999].  Leading zeros are replaced with spaces.
 *
 * @param  number  Value to display.
 */
void seg_setUnsigned(uint16_t number);

/**
 * @brief  Set all three digits from a NUL-terminated ASCII string.
 *
 * Digits beyond SEG_NUM_DIGITS are ignored. Short strings are space-padded.
 *
 * @param  text  Pointer to a NUL-terminated string.
 */
void seg_setText(const char *text);

/**
 * @brief  Set the decimal-point flag for one digit.
 *
 * @param  index  Digit index (0..SEG_NUM_DIGITS-1).
 * @param  on     0 = off, 1 = on.
 */
void seg_setDot(uint8_t index, uint8_t on);

/**
 * @brief  Clear all digits to spaces and turn off all decimal points.
 */
void seg_clear(void);

/**
 * @brief  Refresh one multiplexed digit and advance to the next.
 *
 * Call this at a fixed rate (typically 200 Hz – 1 kHz per digit) from a
 * timer ISR or a periodic task.
 *
 * @param  on  0 = blank the entire display, 1 = show digits normally.
 */
void seg_updateDisplay(uint8_t on);

/**
 * @brief  Low-level: drive the segment GPIOs for one character.
 *
 * @param  ascii  ASCII character (32..127).
 * @param  dot    0 = DP off, 1 = DP on.
 */
void seg_updateDigit(char ascii, char dot);

#ifdef __cplusplus
}
#endif

#endif /* __SEVEN_SEGMENT_H */
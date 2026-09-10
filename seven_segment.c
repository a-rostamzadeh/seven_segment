/*
******************************************************************************
* @file    seven_segment.c
* @brief   Multiplexed 3-digit 7-segment display driver.
******************************************************************************
*/
#include "seven_segment.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Module state
 * ---------------------------------------------------------------------- */

volatile char    digits[SEG_NUM_DIGITS]      = {0};
volatile char    dots[SEG_NUM_DIGITS]        = {0};
volatile uint8_t active_digit                = 0;

/* -------------------------------------------------------------------------
 * ASCII → 7-segment bitmap table.
 *
 * Bit layout (LSB first): DP G F E D C B A
 * Index 0 corresponds to ASCII 0x20 (space).
 * ---------------------------------------------------------------------- */
const uint8_t SevenSegmentASCII[96] = {
    0b00000000, /* (space) */
    0b10000110, /* ! */
    0b00100010, /* " */
    0b01111110, /* # */
    0b01101101, /* $ */
    0b11010010, /* % */
    0b01000110, /* & */
    0b00100000, /* ' */
    0b00101001, /* ( */
    0b00001011, /* ) */
    0b00100001, /* * */
    0b01110000, /* + */
    0b00010000, /* , */
    0b01000000, /* - */
    0b10000000, /* . */
    0b01010010, /* / */
    0b00111111, /* 0 */
    0b00000110, /* 1 */
    0b01011011, /* 2 */
    0b01001111, /* 3 */
    0b01100110, /* 4 */
    0b01101101, /* 5 */
    0b01111101, /* 6 */
    0b00000111, /* 7 */
    0b01111111, /* 8 */
    0b01101111, /* 9 */
    0b00001001, /* : */
    0b00001101, /* ; */
    0b01100001, /* < */
    0b01001000, /* = */
    0b01000011, /* > */
    0b11010011, /* ? */
    0b01011111, /* @ */
    0b01110111, /* A */
    0b01111100, /* B */
    0b00111001, /* C */
    0b01011110, /* D */
    0b01111001, /* E */
    0b01110001, /* F */
    0b00111101, /* G */
    0b01110110, /* H */
    0b00110000, /* I */
    0b00011110, /* J */
    0b01110101, /* K */
    0b00111000, /* L */
    0b00010101, /* M */
    0b00110111, /* N */
    0b00111111, /* O */
    0b01110011, /* P */
    0b01101011, /* Q */
    0b00110011, /* R */
    0b01101101, /* S */
    0b01111000, /* T */
    0b00111110, /* U */
    0b00111110, /* V */
    0b00101010, /* W */
    0b01110110, /* X */
    0b01101110, /* Y */
    0b01011011, /* Z */
    0b00111001, /* [ */
    0b01100100, /* \ */
    0b00001111, /* ] */
    0b00100011, /* ^ */
    0b00001000, /* _ */
    0b00000010, /* ` */
    0b01011111, /* a */
    0b01111100, /* b */
    0b01011000, /* c */
    0b01011110, /* d */
    0b01111011, /* e */
    0b01110001, /* f */
    0b01101111, /* g */
    0b01110100, /* h */
    0b00010000, /* i */
    0b00001100, /* j */
    0b01110101, /* k */
    0b00110000, /* l */
    0b00010100, /* m */
    0b01010100, /* n */
    0b01011100, /* o */
    0b01110011, /* p */
    0b01100111, /* q */
    0b01010000, /* r */
    0b01101101, /* s */
    0b01111000, /* t */
    0b00011100, /* u */
    0b00011100, /* v */
    0b00010100, /* w */
    0b01110110, /* x */
    0b01101110, /* y */
    0b01011011, /* z */
    0b01000110, /* { */
    0b00110000, /* | */
    0b01110000, /* } */
    0b00000001, /* ~ */
    0b00000000, /* (del) */
};

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/**
 * @brief  Write a single 7-segment GPIO according to the active-low setting.
 */
static inline void seg_writePin(GPIO_TypeDef *port, uint16_t pin, uint8_t on)
{
#if SEG_ACTIVE_LOW
    HAL_GPIO_WritePin(port, pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
#else
    HAL_GPIO_WritePin(port, pin, on ? GPIO_PIN_SET   : GPIO_PIN_RESET);
#endif
}

/**
 * @brief  Enable/disable a digit-select pin.
 */
static inline void seg_selectDigit(uint8_t index, uint8_t on)
{
    GPIO_TypeDef *port;
    uint16_t      pin;

    switch (index) {
        case 0: port = DIGIT1_GPIO_Port; pin = DIGIT1_Pin; break;
        case 1: port = DIGIT2_GPIO_Port; pin = DIGIT2_Pin; break;
        case 2: port = DIGIT3_GPIO_Port; pin = DIGIT3_Pin; break;
        default: return;
    }

#if SEG_DIGIT_ACTIVE_LOW
    HAL_GPIO_WritePin(port, pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
#else
    HAL_GPIO_WritePin(port, pin, on ? GPIO_PIN_SET   : GPIO_PIN_RESET);
#endif
}

/* -------------------------------------------------------------------------
 * Low-level API
 * ---------------------------------------------------------------------- */

void seg_updateDigit(char ascii, char dot)
{
    /* Reject characters outside the printable ASCII range. */
    if ((uint8_t)ascii < 32 || (uint8_t)ascii > 127) {
        ascii = ' ';
    }

    uint8_t c = SevenSegmentASCII[(uint8_t)ascii - 32];

    if (dot) {
        c |= (1u << 7);   /* DP is bit 7 */
    }

    /* Bit layout: DP G F E D C B A */
    seg_writePin(A_7S_GPIO_Port, A_7S_Pin, (c >> 0) & 0x1);
    seg_writePin(B_7S_GPIO_Port, B_7S_Pin, (c >> 1) & 0x1);
    seg_writePin(C_7S_GPIO_Port, C_7S_Pin, (c >> 2) & 0x1);
    seg_writePin(D_7S_GPIO_Port, D_7S_Pin, (c >> 3) & 0x1);
    seg_writePin(E_7S_GPIO_Port, E_7S_Pin, (c >> 4) & 0x1);
    seg_writePin(F_7S_GPIO_Port, F_7S_Pin, (c >> 5) & 0x1);
    seg_writePin(G_7S_GPIO_Port, G_7S_Pin, (c >> 6) & 0x1);
    seg_writePin(P_7S_GPIO_Port, P_7S_Pin, (c >> 7) & 0x1);
}

/* -------------------------------------------------------------------------
 * Multiplexing
 * ---------------------------------------------------------------------- */

void seg_updateDisplay(uint8_t on)
{
    /* 1. Blank the currently active digit to prevent ghosting. */
    seg_selectDigit(active_digit, 0);

    if (!on) {
        /* Display is off — leave all digits blanked. */
        return;
    }

    /* 2. Drive the segments for the next digit. */
    seg_updateDigit(digits[active_digit], dots[active_digit]);

    /* 3. Enable that digit. */
    seg_selectDigit(active_digit, 1);

    /* 4. Advance the multiplexer. */
    if (++active_digit >= SEG_NUM_DIGITS) {
        active_digit = 0;
    }
}

/* -------------------------------------------------------------------------
 * High-level content setters
 * ---------------------------------------------------------------------- */

void seg_setNumber(int16_t number)
{
    /* Clamp to the displayable range: -99 .. 999 */
    if (number >  999) number =  999;
    if (number <  -99) number =  -99;

    uint8_t negative = 0;
    if (number < 0) {
        negative = 1;
        number = -number;
    }

    /* Fill from right (least significant) to left. */
    uint16_t num = (uint16_t)number;
    uint8_t  i;

    for (i = SEG_NUM_DIGITS; i-- > 0; ) {
        if (num == 0 && i < (SEG_NUM_DIGITS - 1)) {
            digits[i] = ' ';
        } else {
            digits[i] = (char)('0' + (num % 10));
            num /= 10;
        }
    }

    /* If the number is negative, place the sign on the most significant
     * non-blank digit. */
    if (negative) {
        for (i = 0; i < SEG_NUM_DIGITS; ++i) {
            if (digits[i] != ' ') {
                digits[i] = '-';
                break;
            }
        }
        /* Handle the case where the number was 0 but signed. */
        if (i == SEG_NUM_DIGITS) {
            digits[0] = '-';
        }
    }
}

void seg_setUnsigned(uint16_t number)
{
    if (number > 999) {
        number = 999;
    }

    for (uint8_t i = SEG_NUM_DIGITS; i-- > 0; ) {
        if (number == 0 && i < (SEG_NUM_DIGITS - 1)) {
            digits[i] = ' ';
        } else {
            digits[i] = (char)('0' + (number % 10));
            number /= 10;
        }
    }
}

void seg_setText(const char *text)
{
    uint8_t i = 0;

    if (text == NULL) {
        seg_clear();
        return;
    }

    /* Copy up to SEG_NUM_DIGITS characters. */
    while (i < SEG_NUM_DIGITS && text[i] != '\0') {
        digits[i] = text[i];
        ++i;
    }

    /* Pad remaining positions with spaces. */
    while (i < SEG_NUM_DIGITS) {
        digits[i] = ' ';
        ++i;
    }
}

void seg_setDot(uint8_t index, uint8_t on)
{
    if (index < SEG_NUM_DIGITS) {
        dots[index] = (on != 0) ? 1 : 0;
    }
}

void seg_clear(void)
{
    for (uint8_t i = 0; i < SEG_NUM_DIGITS; ++i) {
        digits[i] = ' ';
        dots[i]   = 0;
    }
}
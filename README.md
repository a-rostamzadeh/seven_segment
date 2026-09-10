

```markdown
# 7-Segment Display Driver (STM32 HAL)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Language: C99](https://img.shields.io/badge/Language-C99-blue.svg)]()
[![Platform: STM32 HAL](https://img.shields.io/badge/Platform-STM32%20HAL-green.svg)]()

A lightweight, portable C driver for **multiplexed 3-digit 7-segment displays**
on STM32 microcontrollers. It supports both **common-cathode** and
**common-anode** displays, includes a full ASCII font table, and provides
helpers to display **numbers** and **text** with decimal-point control.

---

## Table of Contents

- [Features](#features)
- [Hardware Overview](#hardware-overview)
- [File Structure](#file-structure)
- [Requirements](#requirements)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
  - [Content Setters](#content-setters)
  - [Display Control](#display-control)
  - [Low-Level Access](#low-level-access)
- [ASCII Font Table](#ascii-font-table)
- [Multiplexing Explained](#multiplexing-explained)
- [Configuration](#configuration)
- [Example: Timer-Driven Refresh](#example-timer-driven-refresh)
- [Performance Tips](#performance-tips)
- [Troubleshooting](#troubleshooting)
- [References](#references)
- [License](#license)

---

## Features

- ✅ 3-digit multiplexed display (configurable via `SEG_NUM_DIGITS`)
- ✅ Supports both **common-cathode** and **common-anode** displays
- ✅ Signed integers (`-99` to `999`) and unsigned integers (`0` to `999`)
- ✅ Arbitrary text strings (up to 3 chars)
- ✅ Per-digit **decimal-point** control
- ✅ ISR-safe state (`volatile` globals)
- ✅ No dynamic memory, no float, no libc dependency
- ✅ Portable ANSI C99 — easily adapted to other HALs

---

## Hardware Overview

| Parameter              | Value                                       |
|------------------------|---------------------------------------------|
| Display type           | Multiplexed 7-segment, N digits             |
| Digits supported       | 3 (default, configurable)                   |
| Segment pins           | A, B, C, D, E, F, G, DP (8 GPIOs)           |
| Digit-select pins      | DIGIT1, DIGIT2, DIGIT3 (3 GPIOs)            |
| Drive polarity         | Active-HIGH or active-LOW (configurable)    |
| Multiplex frequency    | 200 Hz – 1 kHz per digit (recommended)      |
| MCU                    | Any STM32 with HAL (tested on STM32H7)      |

### Segment-to-bit Mapping

The font table uses the following bit order (LSB first), matching most
7-segment modules and the `main.h` pin names:

```text
Bit:   7    6    5    4    3    2    1    0
Seg:   DP   G    F    E    D    C    B    A
```

### Typical Wiring

```text
STM32 GPIO ──┬── 220 Ω ── A_7S ─┐
             ├── 220 Ω ── B_7S ─┤
             ├── 220 Ω ── C_7S ─┤
             ├── 220 Ω ── D_7S ─┤   ┌──────────────┐
             ├── 220 Ω ── E_7S ─┼───┤  7-segment   │
             ├── 220 Ω ── F_7S ─┤   │   display    │
             ├── 220 Ω ── G_7S ─┤   └──────────────┘
             └── 220 Ω ── P_7S ─┘
                                   
STM32 GPIO ──┬── NPN base ── DIGIT1 (common cathode)
             ├── NPN base ── DIGIT2
             └── NPN base ── DIGIT3
```

> **Note:** Current-limiting resistors on the segment lines are mandatory.
> Digit-select lines are usually driven through a transistor (NPN for
> common-cathode, PNP for common-anode displays).

---

## File Structure

```text
seven_segment/
├── seven_segment.c     # Driver implementation
├── seven_segment.h     # Public API and configuration
├── LICENSE             # MIT License
└── README.md           # This file
```

---

## Requirements

- **STM32 HAL** (any family; tested on STM32H7)
- **C99** or later
- GPIO pins declared in `main.h`:
  - `A_7S`, `B_7S`, `C_7S`, `D_7S`, `E_7S`, `F_7S`, `G_7S`, `P_7S`
  - `DIGIT1`, `DIGIT2`, `DIGIT3`
- A periodic timebase (timer ISR or RTOS task) calling `seg_updateDisplay()`

---

## Installation

1. Copy `seven_segment.c` and `seven_segment.h` into your project's source
   tree (e.g., `Core/Src/` and `Core/Inc/`).
2. Add `seven_segment.c` to your build (STM32CubeIDE does this automatically).
3. Ensure the GPIO labels exist in `main.h`:

   ```c
   /* Segment pins */
   #define A_7S_Pin       GPIO_PIN_0
   #define A_7S_GPIO_Port GPIOA
   /* ... B_7S ... P_7S ... */

   /* Digit-select pins */
   #define DIGIT1_Pin       GPIO_PIN_8
   #define DIGIT1_GPIO_Port GPIOA
   /* ... DIGIT2, DIGIT3 ... */
   ```

4. Include the header where needed:

   ```c
   #include "seven_segment.h"
   ```

---

## Quick Start

```c
#include "seven_segment.h"

void app_init(void)
{
    seg_clear();          // blank all digits
    seg_setNumber(123);   // show "123"
    seg_setDot(1, 1);     // turn on DP on the middle digit -> "1.23"
}

/* Call this at 1 kHz from a timer ISR or periodic task. */
void app_tick(void)
{
    seg_updateDisplay(1); // refresh one digit and advance
}
```

---

## API Reference

### Content Setters

#### Display a signed number

```c
void seg_setNumber(int16_t number);
```

- Clamps to the range `-99 … 999`
- Leading zeros are replaced with spaces
- Negative numbers show a `-` on the most significant non-blank digit

**Examples:**

| Input   | Display |
|---------|---------|
| `0`     | `  0`   |
| `42`    | ` 42`   |
| `123`   | `123`   |
| `-7`    | ` -7`   |
| `-42`   | `-42`   |
| `1000`  | `999`   |

#### Display an unsigned number

```c
void seg_setUnsigned(uint16_t number);
```

- Clamps to `0 … 999`
- Same leading-zero handling as `seg_setNumber()`

#### Display arbitrary text

```c
void seg_setText(const char *text);
```

- Copies up to `SEG_NUM_DIGITS` characters
- Short strings are space-padded
- `NULL` is treated as an empty string

#### Set a decimal point

```c
void seg_setDot(uint8_t index, uint8_t on);
```

- `index`: `0 … SEG_NUM_DIGITS - 1`
- `on`: `0` = off, `1` = on

#### Clear the display

```c
void seg_clear(void);
```

Fills all digits with spaces and turns off all decimal points.

---

### Display Control

```c
void seg_updateDisplay(uint8_t on);
```

Refreshes **one** digit and advances the multiplexer. Call this at a fixed
rate — typically `N × 100 Hz` where `N` is the number of digits (e.g., 300 Hz
for a 3-digit display at 100 Hz refresh rate).

- `on = 1` — normal operation
- `on = 0` — blanks the entire display (previous digit is switched off)

---

### Low-Level Access

```c
void seg_updateDigit(char ascii, char dot);
```

Drives the segment GPIOs directly for a single character. Rarely needed by
application code; exposed for advanced use cases (e.g., custom animations).

---

## ASCII Font Table

The driver ships with a **96-entry lookup table** (`SevenSegmentASCII[]`)
covering the printable ASCII range `0x20` (space) through `0x7F` (`DEL`).

- Index 0 → space (`0x20`)
- Index 95 → delete (`0x7F`)

Each entry is a bitmask with the layout `DP G F E D C B A`. To add a custom
symbol, insert it at the appropriate index or add a new lookup table.

---

## Multiplexing Explained

A 3-digit display has 8 segment lines shared across all digits. Only one
digit is lit at a time; persistence of vision makes all three appear
simultaneously lit.

```text
   ┌─ DIGIT1 ─┐   ┌─ DIGIT2 ─┐   ┌─ DIGIT3 ─┐
   │          │   │          │   │          │
  A│          │A  │          │A  │          │A
   └──────────┘   └──────────┘   └──────────┘
        │              │              │
        └──────────────┼──────────────┘
                       │
              (shared segment bus)
```

**Timing recommendation:**

| Refresh rate per digit | Full-frame refresh | Notes                    |
|------------------------|--------------------|--------------------------|
| 100 Hz                 | 33 Hz              | Minimum acceptable       |
| 200 Hz                 | 67 Hz              | Recommended              |
| 300 Hz                 | 100 Hz             | Smooth, no flicker       |
| 1 kHz                  | 333 Hz             | Overkill, more CPU load  |

At each tick, `seg_updateDisplay()` does the following:

1. **Blanks** the currently active digit (prevents ghosting).
2. Drives the **segment lines** for the next digit's character.
3. **Enables** the next digit.
4. Advances `active_digit`.

---

## Configuration

All configuration is done through macros in `seven_segment.h`:

```c
/* Number of multiplexed digits. */
#define SEG_NUM_DIGITS      3

/* Set to 1 for common-anode (active-LOW segment drive). */
#define SEG_ACTIVE_LOW      0

/* Set to 1 if digit-select pins are active-LOW. */
#define SEG_DIGIT_ACTIVE_LOW 0
```

| Display Type                | `SEG_ACTIVE_LOW` | `SEG_DIGIT_ACTIVE_LOW` |
|-----------------------------|------------------|------------------------|
| Common cathode, NPN digit drivers | `0`       | `0`                    |
| Common anode, PNP digit drivers   | `1`       | `1`                    |
| Common cathode, direct GPIO drive | `0`       | `0`                    |

You can override any of these from `main.h` or your compiler flags (e.g.
`-DSEG_ACTIVE_LOW=1`) without editing the header.

---

## Example: Timer-Driven Refresh

Hook `seg_updateDisplay()` into a 1 kHz timer interrupt. The example uses
`TIM6` on STM32H7 but works with any timer.

```c
#include "seven_segment.h"

/* --- 1 kHz timer ISR ------------------------------------------------ */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        seg_updateDisplay(1);
    }
}

/* --- Application ---------------------------------------------------- */
void app_main(void)
{
    HAL_TIM_Base_Start_IT(&htim6);   // start 1 kHz tick
    seg_clear();

    uint16_t counter = 0;
    while (1) {
        seg_setUnsigned(counter);
        seg_setDot(0, 0);
        seg_setDot(1, 1);            // format as "NN.N"
        seg_setDot(2, 0);

        counter = (counter + 1) % 1000;
        HAL_Delay(100);
    }
}
```

---

## Performance Tips

If your ISR budget is tight, replace the 8 individual `HAL_GPIO_WritePin()`
calls in `seg_updateDigit()` with a single BSRR write. This reduces the
refresh cost from ~10 µs to ~100 ns.

```c
/* Example: all segment pins on the same GPIO port.
 * Replace port, mask, and active-low flag with your own values. */
static inline void seg_writePort(GPIO_TypeDef *port, uint16_t mask,
                                 uint16_t value, uint8_t active_low)
{
    if (active_low) value = (uint16_t)~value;
    port->BSRR = (uint32_t)((value & mask))
               | ((uint32_t)(~value & mask) << 16);
}
```

Additional tips:

- Keep `seg_updateDisplay()` **short** — no `printf`, no blocking calls.
- Use `HAL_GPIO_WritePin()` on a `GPIO_TypeDef *` in RAM, not through a
  function pointer lookup table.
- If using an RTOS, run the refresh from a **high-priority** task, or
  better, from a hardware timer ISR.

---

## Troubleshooting

| Symptom                          | Likely Cause                       | Fix                                                                 |
|----------------------------------|------------------------------------|---------------------------------------------------------------------|
| Display completely dark          | Polarity mismatch                  | Toggle `SEG_ACTIVE_LOW` and/or `SEG_DIGIT_ACTIVE_LOW`               |
| All digits show the same char    | Digit-select wiring swapped        | Verify `DIGIT1/2/3_Pin` / `_GPIO_Port` in `main.h`                  |
| Ghosting between digits          | Missing blanking or slow refresh   | Ensure `seg_updateDisplay()` blanks the previous digit first        |
| Only one digit lit               | ISR not firing                     | Verify the timer callback calls `seg_updateDisplay()`               |
| Dim display                      | Refresh too fast, low duty cycle   | Slow down the tick to ~200–300 Hz per digit                         |
| Flicker                          | Refresh too slow                   | Increase tick rate to ≥ 200 Hz per digit                            |
| Random characters                | `digits[]` contains invalid ASCII  | Use `seg_setNumber()` / `seg_setText()` instead of writing globals  |
| Decimal point always on          | `dots[]` not cleared               | Call `seg_clear()` or `seg_setDot(i, 0)`                            |

---

## References

- **STM32 HAL GPIO Documentation** — STMicroelectronics
- **7-Segment Display Basics** — [Wikipedia](https://en.wikipedia.org/wiki/Seven-segment_display)
- **ASCII Table** — [asciitable.com](https://www.asciitable.com/)
- **Persistence of Vision** — [Wikipedia](https://en.wikipedia.org/wiki/Persistence_of_vision)

---

## License

This project is licensed under the **MIT License** — see the
[LICENSE](LICENSE) file for details.

---

## Contact

- **Author:** Ali Rostamzadeh
- **Issues:** [GitHub Issues](https://github.com/a-rostamzadeh/seven_segment/issues)
- **Email:** a.rostamzadeh@gmail.com
- **Documentation:** [Wiki](https://github.com/a-rostamzadeh/seven_segment/wiki)

---

*Last updated: 2024*
```

---


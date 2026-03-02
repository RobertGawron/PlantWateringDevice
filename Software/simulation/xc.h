/**
 * @file xc.h
 * @brief Mock PIC hardware registers for unit testing and web simulation
 *
 * This file provides mock implementations of PIC10F202 hardware
 * registers and XC8 compiler intrinsics for use in:
 * - Unit tests on Linux/PC
 * - Web simulation via Emscripten/WASM
 */

#ifndef XC_H
#define XC_H

#include <stdint.h>
#include <stdbool.h>

/* Mock GPIO bit structure */
typedef struct
{
    uint8_t GP0 : 1;
    uint8_t GP1 : 1;
    uint8_t GP2 : 1;
    uint8_t GP3 : 1;
    uint8_t : 4; /* Unused bits */
} GPIObits_t;

/* Mock hardware registers */
extern volatile GPIObits_t GPIObits;
extern volatile uint8_t OPTION;
extern volatile uint8_t TRISGPIO;
extern volatile uint8_t GPIO;

/* ================================================================
 * DELAY IMPLEMENTATION
 * ================================================================ */

/* Web simulation: Block until JS advances the tick */
#include <emscripten.h>

extern volatile int tick_gate;

/**
 * @brief Blocks execution until JavaScript calls advance_tick().
 *
 * This allows JavaScript to control the timing of the simulation.
 * The main loop will pause here until JS signals it to continue.
 */
static inline void hw_delay_stub(void)
{
    tick_gate = 0;

    /* Block until JS sets tick_gate = 1 */
    while (tick_gate == 0)
    {
        emscripten_sleep(1); /* Yields to browser event loop */
    }
}

#define HW_DELAY_MS(ms) hw_delay_stub()

#endif /* XC_H */
#include "hal.h"

#ifndef TARGET_HOST

/*
 * Firmware build:
 * No implementation is provided here.
 *
 * All hardware abstraction is implemented in the header file
 * using macros. Functions are avoided due to the limited hardware
 * call stack of the PIC10F202.
 */

#else

#include <stdbool.h>
#include <emscripten/emscripten.h>

/* Mock hardware registers */
volatile uint8_t OPTION;
volatile uint8_t TRISGPIO;
volatile GPIObits_t GPIObits;

/**
 * @brief Tick synchronization flag.
 *
 * Used to simulate blocking delay behavior.
 */
volatile bool tickGate = false;

/* ================================================================
 * JavaScript Interop (Emscripten)
 * ================================================================ */

/*
 * JavaScript code inside this .c file cannot be formatted automatically.
 * The '===' operator is not valid C syntax and would break formatting.
 */
/* clang-format off */
EM_JS(int, jsGetGpio, (int pin), {
    if (typeof getGPIOState === 'function') {
        return getGPIOState(pin);
    }
    return 0;
});

EM_JS(void, jsSetGpio, (int pin, int state), {
    if (typeof setGPIOState === 'function') {
        setGPIOState(pin, state);
    }
});
/* clang-format on */

/* ================================================================
 * Time Control Synchronization
 * ================================================================ */

/**
 * @brief Blocking delay abstraction for WebAssembly simulation.
 *
 * Blocks until advanceTick() sets tickGate to true.
 */
void HW_DELAY_MS(uint8_t DURATION_MS)
{
    (void)DURATION_MS; /* Unused in simulation */

    tickGate = false;

    while (!tickGate)
    {
        emscripten_sleep(1); /* Yield to browser event loop */
    }
}

/**
 * @brief Advance one simulated tick.
 *
 * Exposed to JavaScript.
 */
EMSCRIPTEN_KEEPALIVE
void advanceTick(void)
{
    tickGate = true;
}

/* ================================================================
 * GPIO Abstraction
 * ================================================================ */

void GPIO_SET(uint8_t GPIO_PIN, uint8_t STATE)
{
    jsSetGpio(GPIO_PIN, STATE);
}

bool GPIO_GET(uint8_t GPIO_PIN)
{
    return (bool)jsGetGpio(GPIO_PIN);
}

#endif
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

#include "logger.h"

#include <stdbool.h>
#include <emscripten/emscripten.h>

/* Mock hardware registers */
volatile uint8_t OPTION;
volatile uint8_t TRISGPIO;
volatile GPIObits_t GPIObits;

/* ================================================================
 * Time Control Synchronization
 * ================================================================ */

/* Empty - timing controlled by JavaScript calling _main() repeatedly */
void HW_DELAY_MS(uint8_t DURATION_MS)
{
    (void)DURATION_MS;
}

/* ================================================================
 * GPIO Abstraction (using EM_JS - synchronous, no threads needed)
 * ================================================================ */
/* clang-format off */
EM_JS(void, jsSetGPIOState, (int pin, int state), {
    if (typeof window.setGPIOState === 'function') {
        window.setGPIOState(pin, state);
    } else {
        console.warn('[HAL] setGPIOState not available');
    }
});

EM_JS(int, jsGetGPIOState, (int pin), {
    if (typeof window.getGPIOState === 'function') {
        return window.getGPIOState(pin);
    } else {
        console.warn('[HAL] getGPIOState not available');
        return 0;
    }
});

void GPIO_SET(uint8_t GPIO_PIN, uint8_t STATE)
{
    jsSetGPIOState(GPIO_PIN, STATE);
}

bool GPIO_GET(uint8_t GPIO_PIN)
{
    return jsGetGPIOState(GPIO_PIN) != 0;
}
/* clang-format on */
#endif
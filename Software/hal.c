#include "hal.h"

#if !defined(TARGET_HOST) && !defined(__FRAMAC__)

/*
 * Firmware build:
 * No implementation is provided here.
 *
 * All hardware abstraction is implemented in the header file
 * using macros. Functions are avoided due to the limited hardware
 * call stack of the PIC10F202.
 */

#elif defined(__FRAMAC__)

/* Mock hardware registers */
__HAL_VOLATILE uint8_t OPTION;
__HAL_VOLATILE uint8_t TRISGPIO;
__HAL_VOLATILE GPIObits_t GPIObits;

#else /* TARGET_HOST */

#include "logger.h"

#include <stdbool.h>
#include <emscripten/emscripten.h>

/* Mock hardware registers */
__HAL_VOLATILE uint8_t OPTION;
__HAL_VOLATILE uint8_t TRISGPIO;
__HAL_VOLATILE GPIObits_t GPIObits;

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
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

/**
 * @brief Tick synchronization flag.
 *
 * Used to simulate blocking delay behavior.
 */
volatile bool tickGate = false;

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

/* clang-format off */
void GPIO_SET(uint8_t GPIO_PIN, uint8_t STATE)
{
    MAIN_THREAD_EM_ASM({
        if (typeof window.setGPIOState === 'function') {
            window.setGPIOState($0, $1);
        }
        else
        {
            logDebugLow("jsSetGpio: no function");
        }
    }, GPIO_PIN, STATE);
}

bool GPIO_GET(uint8_t GPIO_PIN)
{
    bool result = MAIN_THREAD_EM_ASM_INT({
        if (typeof window.getGPIOState === 'function') {
            return window.getGPIOState($0);
        }
        else
        {
            logDebugLow("jsGetGpio: no function");
            return 0;
        }
    }, GPIO_PIN);

    return result;   
}

/* clang-format on */
#endif
/**
 * @file xc.c
 * @brief Implementation of mock PIC hardware registers and Emscripten interface
 */

#include "xc.h"

/* Define the actual storage for mock registers */
volatile GPIObits_t GPIObits = {0};
volatile uint8_t OPTION = 0;
volatile uint8_t TRISGPIO = 0;
volatile uint8_t GPIO = 0;

#include <emscripten.h>

/* Gate variable for blocking HW_DELAY_MS */
volatile int tick_gate = 0;

/* ================================================================
 * EXPORTED FUNCTIONS FOR JAVASCRIPT CONTROL
 * ================================================================ */

/**
 * @brief Advances the simulation by one tick (unblocks HW_DELAY_MS).
 *
 * Called from JavaScript to allow the next main loop iteration.
 */
EMSCRIPTEN_KEEPALIVE
void advance_tick(void)
{
    tick_gate = 1;
}

/**
 * @brief Simulates button press (sets GP3 to LOW/active).
 */
EMSCRIPTEN_KEEPALIVE
void button_press(void)
{
    GPIObits.GP3 = 0; /* Active LOW */
}

/**
 * @brief Simulates button release (sets GP3 to HIGH/inactive).
 */
EMSCRIPTEN_KEEPALIVE
void button_release(void)
{
    GPIObits.GP3 = 1; /* Inactive HIGH (pull-up) */
}

/**
 * @brief Sets soil moisture sensor state.
 *
 * @param dry 1 = soil is dry (HIGH), 0 = soil is wet (LOW)
 */
EMSCRIPTEN_KEEPALIVE
void set_soil_moisture(int dry)
{
    GPIObits.GP1 = dry ? 1 : 0;
}

/**
 * @brief Gets current pump state.
 *
 * @return 1 if pump is running (HIGH), 0 if stopped (LOW)
 */
EMSCRIPTEN_KEEPALIVE
int get_pump_state(void)
{
    return GPIObits.GP2;
}

/**
 * @brief Gets current display output state.
 *
 * @return 1 if display clock is HIGH, 0 if LOW
 */
EMSCRIPTEN_KEEPALIVE
int get_display_state(void)
{
    return GPIObits.GP0;
}

/**
 * @brief Gets current button state.
 *
 * @return 0 if pressed (active LOW), 1 if released
 */
EMSCRIPTEN_KEEPALIVE
int get_button_state(void)
{
    return GPIObits.GP3;
}

/**
 * @brief Gets soil moisture sensor state.
 *
 * @return 1 if dry (HIGH), 0 if wet (LOW)
 */
EMSCRIPTEN_KEEPALIVE
int get_soil_state(void)
{
    return GPIObits.GP1;
}

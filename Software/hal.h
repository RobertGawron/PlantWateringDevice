/**
 * @brief Hardware Abstraction Layer for PIC10F202.
 */
#ifndef HAL_H
#define HAL_H

#define TARGET_HOST
/**
 * MCU oscillator frequency in Hz.
 * Required by XC8 delay macros.
 */
#define MCU_CLOCK_FREQUENCY_HZ (4000000UL)

/**
 * XC8-required definition for delay calculation.
 */
#define _XTAL_FREQ MCU_CLOCK_FREQUENCY_HZ

// #ifndef TARGET_HOST
/* Note: _XTAL_FREQ must be defined before including <xc.h>. */
#include <xc.h>
// #else
#include <stdint.h>
#include <stdbool.h>
#include <emscripten/emscripten.h>

#if 0
uint8_t OPTION;
uint8_t TRISGPIO;

typedef union
{
    struct
    {
        unsigned GP0 : 1;
        unsigned GP1 : 1;
        unsigned GP2 : 1;
        unsigned GP3 : 1;
    };
} GPIObits_t;
/*extern volatile*/ GPIObits_t GPIObits;
#endif

// #endif

/* ================================================================
 * GPIO ABSTRACTION
 * ================================================================ */

#define GPIO_LEVEL_LOW (0U)
#define GPIO_LEVEL_HIGH (1U)

#ifndef TARGET_HOST
#define GPIO_SET(GPIO_PIN, STATE) ((GPIO_PIN) = (STATE))
#define GPIO_GET(GPIO_PIN) ((GPIO_PIN) == GPIO_LEVEL_HIGH)
#else
// #define GPIO_SET(GPIO_PIN, STATE) true

/*EM_JS(int, js_get_gpio, (int pin), {
    if (typeof getGPIOState == = 'function')
    {
        return getGPIOState(pin);
    }
    return 0;
});

EM_JS(void, js_set_gpio, (int pin, int state), {
    if (typeof setGPIOState == = 'function')
    {
        setGPIOState(pin, state);
    }
    //    return 0;
});
*/
void GPIO_SET(uint8_t GPIO_PIN, uint8_t STATE);
/*{
    js_set_gpio(GPIO_PIN, STATE);
}*/

// EMSCRIPTEN_KEEPALIVE
bool GPIO_GET(uint8_t GPIO_PIN);
/*{
    return js_get_gpio(GPIO_PIN);
}*/

// #define GPIO_GET(GPIO_PIN) true
#endif
/* ================================================================
 * TIME ABSTRACTION
 * ================================================================ */

#ifndef TARGET_HOST
#define HW_DELAY_MS(DURATION_MS) __delay_ms(DURATION_MS)

#else
// #define HW_DELAY_MS(DURATION_MS) true
void HW_DELAY_MS(int DURATION_MS);
//{}*/
void advanceTick(void); 
#endif

#endif /* HAL_H */
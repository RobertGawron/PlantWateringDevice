#ifndef HAL
#define HAL

#define MCU_CLOCK_FREQUENCY_HZ (4000000UL)
/* Oscillator frequency (required by __delay_ms) */
#define _XTAL_FREQ MCU_CLOCK_FREQUENCY_HZ

#include <xc.h>

/* ================================================================
 * GPIO LEVEL DEFINITIONS
 * ================================================================ */

#define GPIO_LEVEL_LOW (0U)
#define GPIO_LEVEL_HIGH (1U)


#define GPIO_SET(GPIO_PIN, STATE) (GPIO_PIN = (STATE))

#define GPIO_GET(GPIO_PIN) (GPIO_PIN == GPIO_LEVEL_HIGH)


#define HW_DELAY_MS(DURATION_MS) __delay_ms(DURATION_MS)

#endif
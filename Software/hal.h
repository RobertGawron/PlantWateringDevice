/**
 * @file hal.h
 * @brief Hardware Abstraction Layer for PIC10F202.
 *
 * @details
 * The PIC10F202 provides a hardware return stack depth of two levels only.
 * To prevent stack overflow and undefined behavior, hardware access is
 * implemented using macros rather than functions. Inline functions are
 * avoided because the compiler does not guarantee inlining.
 */

#ifndef HAL_H
#define HAL_H

/**
 * MCU oscillator frequency in Hz.
 * Required by XC8 delay macros.
 */
#define MCU_CLOCK_FREQUENCY_HZ (4000000UL)

/**
 * XC8-required definition for delay calculation.
 */
#define _XTAL_FREQ MCU_CLOCK_FREQUENCY_HZ

#ifndef TARGET_HOST

/* _XTAL_FREQ must be defined before including <xc.h>. */
#include <xc.h>

#else /* TARGET_HOST */

/*
 * Host build:
 * The Microchip-specific <xc.h> header is not available outside
 * the XC8 toolchain. For simulation and unit testing, the required
 * registers and types are redefined locally.
 */

#include <stdint.h>
#include <stdbool.h>

/* Mock hardware registers */
extern volatile uint8_t OPTION;
extern volatile uint8_t TRISGPIO;

typedef union
{
    struct
    {
        uint8_t GP0 : 1;
        uint8_t GP1 : 1;
        uint8_t GP2 : 1;
        uint8_t GP3 : 1;
    };
} GPIObits_t;

extern volatile GPIObits_t GPIObits;

#endif /* TARGET_HOST */

/* ================================================================
 * GPIO ABSTRACTION
 * ================================================================ */

#define GPIO_LEVEL_LOW (0U)
#define GPIO_LEVEL_HIGH (1U)

#ifndef TARGET_HOST

#define GPIO_SET(GPIO_PIN, STATE) ((GPIO_PIN) = (STATE))
#define GPIO_GET(GPIO_PIN) ((GPIO_PIN) == GPIO_LEVEL_HIGH)

#else /* TARGET_HOST */

void GPIO_SET(uint8_t GPIO_PIN, uint8_t STATE);
bool GPIO_GET(uint8_t GPIO_PIN);

#endif

/* ================================================================
 * TIME ABSTRACTION
 * ================================================================ */

#ifndef TARGET_HOST

#define HW_DELAY_MS(DURATION_MS) __delay_ms(DURATION_MS)

#else /* TARGET_HOST */

void HW_DELAY_MS(uint8_t DURATION_MS);
void advanceTick(void);

#endif

#endif /* HAL_H */
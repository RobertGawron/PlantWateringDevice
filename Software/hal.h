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

#if !defined(TARGET_HOST) && !defined(__FRAMAC__)
/* ================================================================
 * PIC HARDWARE BUILD
 * ================================================================ */

/* _XTAL_FREQ must be defined before including <xc.h>. */
#include <xc.h>

#else /* TARGET_HOST || __FRAMAC__ */
/* ================================================================
 * HOST BUILD (WASM Simulation) OR FRAMA-C (Formal Verification)
 *
 * The Microchip-specific <xc.h> header is not available outside
 * the XC8 toolchain. For simulation, unit testing, and formal
 * verification, the required registers and types are redefined.
 * ================================================================ */

#include <stdint.h>
#include <stdbool.h>

/* Mock hardware registers */
/*
 * Under Frama-C, volatile is removed so WP can reason about
 * written values. No real hardware exists during verification.
 */
#ifdef __FRAMAC__
#define __HAL_VOLATILE /* nothing */
#else
#define __HAL_VOLATILE volatile
#endif

/* Mock hardware registers */
extern __HAL_VOLATILE uint8_t OPTION;
extern __HAL_VOLATILE uint8_t TRISGPIO;

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

extern __HAL_VOLATILE GPIObits_t GPIObits;
#define __delay_ms(DURATION_MS) \
    do                          \
    { /* Nothing */             \
    } while (0)

#endif /* TARGET_HOST || __FRAMAC__ */

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
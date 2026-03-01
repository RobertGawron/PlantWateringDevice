/**
 * @file xc.h
 * @brief Mock PIC hardware registers for unit testing on Linux
 *
 * This file provides mock implementations of PIC10F202 hardware
 * registers and XC8 compiler intrinsics for use in unit tests.
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

/* Mock delay function */
#define __delay_ms(x)

#endif /* XC_H */
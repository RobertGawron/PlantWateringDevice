/**
 * @file xc.c
 * @brief Implementation of mock PIC hardware registers
 */

#include "xc.h"

/* Define the actual storage for mock registers */
volatile GPIObits_t GPIObits = {0};
volatile uint8_t OPTION = 0;
volatile uint8_t TRISGPIO = 0;
volatile uint8_t GPIO = 0;
#ifndef GPIO_MAPPING_H
#define GPIO_MAPPING_H

#include "hal.h"

/* ================================================================
 * GPIO PIN DEFINITIONS
 * ================================================================ */

#ifndef TARGET_HOST
/* Real hardware - direct register bit references */
#define GPIO_DISPLAY_DATA_OUTPUT GPIObits.GP0
#define GPIO_SOIL_SENSOR_INPUT GPIObits.GP1
#define GPIO_PUMP_MOSFET_OUTPUT GPIObits.GP2
#define GPIO_USER_BUTTON_INPUT GPIObits.GP3
#else
/* Simulation - pin numbers for JavaScript interop */
#define GPIO_DISPLAY_DATA_OUTPUT 0U
#define GPIO_SOIL_SENSOR_INPUT 1U
#define GPIO_PUMP_MOSFET_OUTPUT 2U
#define GPIO_USER_BUTTON_INPUT 3U
#endif

#endif /* GPIO_MAPPING_H */
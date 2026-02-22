/*
##############################################################################

FILE:         main.c
PROCESSOR:    PIC10F202
TOOLCHAIN:    PIC-AS (MPLAB X / VS Code)
DESCRIPTION:  Plant watering controller with user-adjustable pump duration

##############################################################################

HARDWARE CONFIGURATION:

    GP0 (pin 1) - Output: Bargraph clock pulse / duration indicator
                  Connected to CD4017 clock input for 10-step LED bargraph

    GP1 (pin 3) - Input:  Soil moisture sensor
                  HIGH = soil is dry (watering needed)
                  LOW  = soil is wet (no action)

    GP2 (pin 4) - Output: Pump relay control
                  HIGH = pump running
                  LOW  = pump stopped

    GP3 (pin 2) - Input:  User pushbutton (active LOW, 
                  external pull-up required)
                  Directly controls pump duration setting via bargraph

FUNCTIONAL DESCRIPTION:

    - Button press advances the pump duration setting (1-10 steps, wrapping)
    - Each step represents 5 seconds of pump run time (5s to 50s range)
    - GP0 outputs a pulse equal to current setting * 5 seconds as feedback
    - Every 60 seconds, sensor is checked; if dry, pump runs for set duration
    - Pump will not restart if already running

ARCHITECTURAL CONSTRAINTS
    - Hardware call stack: 2 levels (no overflow detection)
    - Compiled stack: DISABLED to conserve code space
    - Assembly output MUST be inspected after each build to verify 
      no unexpected CALL instructions are present

    - All functions use 'fastcall' linkage

    - OM-to-RAM initialization copy: DISABLED
      Rationale:  Saves ~10 words of program memory on a 512-word device. 
                  All variable initialization is performed explicitly 
                  at the start of main().
      Constraint: No global/static variable may use a compile-time initializer 
                  (e.g., "int x = 5;" is PROHIBITED). Use "int x;" and 
                  assign in main() instead.

TIMING ASSUMPTIONS:

    - Internal oscillator runs at nominal 4 MHz
    - Instruction cycle = 1 us (Fosc/4)
    - Base tick period = 20 ms (50 ticks = 1 second)
    - Timing accuracy depends on oscillator tolerance (+/- 1%)

MEMORY USAGE:

    - Program memory: < 256 words (device has 512 words)
    - RAM: 8 bytes (device has 24 bytes available: 0x08-0x1F)

############################################################################## 
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>


/***** TYPES *****/


void main(void) {
    while (true)
    {}
    
    // return;
}

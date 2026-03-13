/**
 * @file main.c
 * @brief Plant watering controller with user-adjustable pump duration.
 *
 * @processor PIC10F202
 * @toolchain PIC-AS (MPLAB X / VS Code)
 *
 * @section hardware_config Hardware Configuration
 *
 * - GP0 (pin 1): Output - Duration feedback pulse
 *   Connected to the CD4026 clock input of a single-digit seven-segment display.
 *
 * - GP1 (pin 3): Input - Soil moisture sensor
 *   HIGH = soil is dry (watering required)
 *   LOW  = soil is wet (no action)
 *
 * - GP2 (pin 4): Output - Pump MOSFT control
 *   HIGH = pump running
 *   LOW  = pump stopped
 *
 * - GP3 (pin 2): Input - User pushbutton (active LOW, external pull-up required)
 *   Controls pump duration setting.
 *
 * @section functional_description Functional Description
 *
 * - Soil sensor is checked every hour; if dry, pump runs for the selected duration.
 * - Button press advances selected pump duration setting (1-9 levels, wrapping).
 * - The current level is visible on the seven-segment display.
 * - Each level represents 5 seconds of pump runtime (5-45 seconds total).
 * - When watering is required, GP0 outputs a pulse equal to the selected
 *   duration level.
 *
 * @section architectural_constraints Architectural Constraints
 *
 * - Hardware call stack: 2 levels (no overflow detection).
 * - Compiled stack: DISABLED to conserve code space.
 * - Assembly output MUST be inspected after each build to verify that
 *   no unintended CALL instructions are generated.
 * - Button debouncing is done on hardware side.
 * - User is not pressing the button at startup.
 * - Pump runtime is shorter than the soil-check interval; therefore,
 *   the pump cannot be activated while already running.
 * - After reprogramming, a device reset is required to synchronize
 *   the display state with the internal duration level.
 * - After power-up, the first soil moisture check is delayed by 10 seconds
 *   to allow system stabilization.
 *
 * @section timing_assumptions Timing Assumptions
 *
 * - Internal oscillator: nominal 4 MHz (uncalibrated internal RC oscillator).
 * - Instruction cycle: 1 us (Fosc/4).
 * - Base tick period: 20 ms (50 ticks = 1 second).
 * - Timing accuracy depends on internal oscillator tolerance as specified
 *   in the device datasheet (temperature and voltage dependent).
 */

/* ================================================================
 * CLOCK CONFIGURATION
 * ================================================================ */

#include "hal.h"
#include "logger.h"
#include "gpio_mapping.h"

#include "watering.h"

#include <stdint.h>
#include <stdbool.h>

/**
 * @note Button and display handling depend directly on the 20 ms tick period.
 *       This could be decoupled, but doing so would require additional code
 *       that is not necessary. A 20 ms interval is acceptable for both
 *       button processing and driving the CD4026.
 */

/*@
    requires data.pump.configured_duration_level >= 1;
    requires data.pump.configured_duration_level <= 9;
    requires data.pump.remaining_cycle_levels > 0
        ==> data.pump.level_remaining_seconds > 0;
    terminates \false;
    assigns OPTION, TRISGPIO, GPIObits, data;
*/
int main(void)
{
    logInfo("PlantWatering firmware starting");

#ifdef TARGET_HOST
    static bool isInitialized = false;
    if (!isInitialized)
    {
#endif
        initialize();
#ifdef TARGET_HOST
        isInitialized = true;
    }
#endif

#ifndef TARGET_HOST
    /*@
      loop invariant tick_range:
          data.time.tick < TIME_TICKS_PER_SECOND;

      loop invariant seconds_range:
          data.time.seconds < TIME_SECONDS_PER_MINUTE;

      loop invariant minutes_range:
          data.time.minutes < TIME_MINUTES_PER_HOUR;

      loop invariant level_valid:
          data.pump.configured_duration_level >= 1 &&
          data.pump.configured_duration_level <= 9;

      loop invariant pump_safety:
          data.pump.remaining_cycle_levels > 0
          ==> data.pump.level_remaining_seconds > 0;

      loop invariant button_range:
          data.button_was_pressed == true ||
          data.button_was_pressed == false;

      loop invariant send_pulse_range:
          data.send_pulse_to_display == true ||
          data.send_pulse_to_display == false;

      loop invariant sending_pulse_range:
          data.sending_pulse_to_display == true ||
          data.sending_pulse_to_display == false;

      loop invariant overflow_pulse_range:
          data.display_overflow_pulse == true ||
          data.display_overflow_pulse == false;

      loop assigns data, GPIObits;
    */
    while (true)
#endif
    {
        HW_DELAY_MS(TIME_BASE_TICK_MS);

        /* Hardware guarantee: 1-bit GPIO bitfields can only hold 0 or 1.
            WP cannot deduce this after loop havoc of the GPIObits union,
            so we state it as an assumption. This is physically guaranteed
            by the PIC10F202 hardware register design. */
        //@ admit GPIObits.GP0 == 0 || GPIObits.GP0 == 1;
        //@ admit GPIObits.GP1 == 0 || GPIObits.GP1 == 1;
        //@ admit GPIObits.GP2 == 0 || GPIObits.GP2 == 1;
        //@ admit GPIObits.GP3 == 0 || GPIObits.GP3 == 1;

        handle_button();
        handle_display();

        if (++data.time.tick >= TIME_TICKS_PER_SECOND)
        {
            data.time.tick = 0;

            if (++data.time.seconds >= TIME_SECONDS_PER_MINUTE)
            {
                data.time.seconds = 0;
                logDebugLow("Minute elapsed: %02d", data.time.minutes + 1);

                if (++data.time.minutes >= TIME_MINUTES_PER_HOUR)
                {
                    data.time.minutes = 0;
                    logInfo("Hour elapsed - checking soil");
                    //@ admit GPIObits.GP1 == 0 || GPIObits.GP1 == 1;
                    handle_sensor_check();
                }
            }

            handle_pump();
        }
    }
}
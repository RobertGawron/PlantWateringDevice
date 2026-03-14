#include "watering.h"

#include "hal.h"
#include "gpio_mapping.h"
#include "logger.h"
#include <stdbool.h>

/* ================================================================
 * PUMP
 * ================================================================ */

/** Maximum selectable pump duration (in steps). */
#define PUMP_DURATION_LEVEL_MAX (9U)

#define PUMP_MAX_RUNTIME_SECONDS \
    (PUMP_DURATION_LEVEL_MAX * PUMP_STEP_DURATION_SECONDS)

#define SOIL_CHECK_INTERVAL_SECONDS \
    (TIME_SECONDS_PER_MINUTE * TIME_MINUTES_PER_HOUR)

/** Minimum selectable pump duration level (in steps). */
#define PUMP_DURATION_LEVEL_MIN (1U)

/** Duration of one pump level in seconds. */
#define PUMP_STEP_DURATION_SECONDS (5U)

/* ================================================================
 * ASSERTS
 * ================================================================ */

_Static_assert(
    PUMP_MAX_RUNTIME_SECONDS < SOIL_CHECK_INTERVAL_SECONDS,
    "Pump runtime must be shorter than soil check interval.");

_Static_assert(
    PUMP_DURATION_LEVEL_MIN == 1U,
    "Minimum pump duration level must be 1. If changed, the display driver "
    "startup pulse logic must be updated to ensure the displayed value "
    "matches the minimum level.");

_Static_assert(
    SOIL_CHECK_STARTUP_DELAY_SECONDS < TIME_SECONDS_PER_MINUTE,
    "Startup delay must be less than 60 seconds.");

_Static_assert(
    (TIME_MILLISECONDS_PER_SECOND % TIME_BASE_TICK_MS) == 0U,
    "TIME_BASE_TICK_MS must divide evenly into one second.");

_Static_assert(
    (TIME_MILLISECONDS_PER_SECOND % TIME_BASE_TICK_MS) == 0U,
    "TIME_BASE_TICK_MS must divide evenly into one second.");

/**
 * @note The 'Initialize data' option remains enabled.
 * Manual initialization at startup would produce similar code size
 * but increases risk of initialization error.
 */
PlantWateringData data =
    {
        .pump =
            {
                .configured_duration_level = PUMP_DURATION_LEVEL_MIN,
                .remaining_cycle_levels = 0U,
                .level_remaining_seconds = PUMP_STEP_DURATION_SECONDS},

        .time =
            {
                .tick = 0U,
                .seconds = SOIL_CHECK_STARTUP_SECONDS_INIT,
                .minutes = SOIL_CHECK_STARTUP_MINUTES_INIT},

        .button_was_pressed = false,
        .send_pulse_to_display = false,
        .sending_pulse_to_display = false,
        .display_overflow_pulse = false};

/**
 * @brief Increments the pump duration level with wrap-around.
 *
 * @note Assumption: Executed only when the pump is idle.
 */
void update_pump_duration(void);

/*@
    assigns OPTION, TRISGPIO,
        data.send_pulse_to_display,
        GPIObits;

    ensures OPTION == 0xCF;
    ensures TRISGPIO == 0x0A;
    ensures data.send_pulse_to_display == true;
    ensures GPIObits.GP2 == GPIO_LEVEL_LOW;
    ensures GPIObits.GP0 == GPIO_LEVEL_LOW;
*/
void initialize(void)
{
    /* Note: Taking advantage of XC8's support for binary literals (0b syntax). */

    /*
    Bit 7: GPWU = 1 (wake-up on pin change disabled)
    Bit 6: GPPU = 1 (weak pull-ups disabled)
    Bit 5: T0CS = 0 (Timer0 clock = internal Fosc/4, frees GP2)
    Bit 4: T0SE = 0 (Timer0 edge select, not used)
    Bit 3: PSA  = 1 (prescaler assigned to WDT, not Timer0)
    Bit 2-0: PS = 111 (prescaler rate, not used)
    */
    OPTION = 0b11001111;

    /*
    GPIO direction mask (1=input, 0=output)
    GP3=input
    GP2=output
    GP1=input
    GP0=output
    */
    TRISGPIO = 0b00001010;

    /*
    After startup, the CD4026 displays 0. However, the minimum soil watering
    duration level is 1; therefore, a pulse is sent to the display to show
    the correct initial level.
    */
    data.send_pulse_to_display = true;

    /* Defensive programming, should be not needed but be sure. */
    GPIO_SET(GPIO_PUMP_MOSFET_OUTPUT, GPIO_LEVEL_LOW);
    GPIO_SET(GPIO_DISPLAY_DATA_OUTPUT, GPIO_LEVEL_LOW);
}

/*@
    requires data.pump.remaining_cycle_levels > 0
        ==> data.pump.level_remaining_seconds > 0;

    assigns data.pump.remaining_cycle_levels,
        data.pump.level_remaining_seconds,
        GPIObits;

    ensures data.pump.remaining_cycle_levels > 0
        ==> data.pump.level_remaining_seconds > 0;

    behavior pump_idle:
        assumes data.pump.remaining_cycle_levels == 0;
        ensures GPIObits.GP2 == GPIO_LEVEL_LOW;
        ensures data.pump.remaining_cycle_levels == 0;
        ensures data.pump.level_remaining_seconds ==
            \old(data.pump.level_remaining_seconds);

    behavior pump_active_counting:
        assumes data.pump.remaining_cycle_levels > 0;
        assumes data.pump.level_remaining_seconds > 1;
        ensures GPIObits.GP2 == GPIO_LEVEL_HIGH;
        ensures data.pump.level_remaining_seconds ==
            \old(data.pump.level_remaining_seconds) - 1;
        ensures data.pump.remaining_cycle_levels ==
            \old(data.pump.remaining_cycle_levels);

    behavior pump_level_complete_continue:
        assumes data.pump.remaining_cycle_levels > 1;
        assumes data.pump.level_remaining_seconds == 1;
        ensures GPIObits.GP2 == GPIO_LEVEL_HIGH;
        ensures data.pump.remaining_cycle_levels ==
            \old(data.pump.remaining_cycle_levels) - 1;
        ensures data.pump.level_remaining_seconds == PUMP_STEP_DURATION_SECONDS;

    behavior pump_cycle_complete:
        assumes data.pump.remaining_cycle_levels == 1;
        assumes data.pump.level_remaining_seconds == 1;
        ensures GPIObits.GP2 == GPIO_LEVEL_HIGH;
        ensures data.pump.remaining_cycle_levels == 0;
        ensures data.pump.level_remaining_seconds == 0;

    complete behaviors;
    disjoint behaviors;
*/
void handle_pump(void)
{
    if (data.pump.remaining_cycle_levels > 0U)
    {
        GPIO_SET(GPIO_PUMP_MOSFET_OUTPUT, GPIO_LEVEL_HIGH);

        if (--data.pump.level_remaining_seconds == 0U)
        {
            data.pump.remaining_cycle_levels--;

            logDebugHigh("Pump level complete (%d remaining)",
                         data.pump.remaining_cycle_levels);

            if (data.pump.remaining_cycle_levels > 0U)
            {
                data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;
            }
            else
            {
                logInfo("Watering cycle complete");
            }
        }
    }
    else
    {
        GPIO_SET(GPIO_PUMP_MOSFET_OUTPUT, GPIO_LEVEL_LOW);
    }
}

/* The button is connected to GND with a pull-up resistor to VCC;
   therefore, pressing the button signals a LOW state. */

/*@
    requires data.pump.configured_duration_level >= PUMP_DURATION_LEVEL_MIN;
    requires data.pump.configured_duration_level <= PUMP_DURATION_LEVEL_MAX;
    requires GPIObits.GP3 == 0 || GPIObits.GP3 == 1;
    requires data.button_was_pressed == true || data.button_was_pressed == false;
    requires data.pump.remaining_cycle_levels > 0
        ==> data.pump.level_remaining_seconds > 0;

    assigns data.button_was_pressed,
        data.send_pulse_to_display,
        data.sending_pulse_to_display,
        data.pump.configured_duration_level,
        data.display_overflow_pulse;

    ensures data.pump.configured_duration_level >= PUMP_DURATION_LEVEL_MIN;
    ensures data.pump.configured_duration_level <= PUMP_DURATION_LEVEL_MAX;
    ensures data.pump.remaining_cycle_levels > 0
        ==> data.pump.level_remaining_seconds > 0;

    behavior button_pressed:
        assumes GPIObits.GP3 == GPIO_LEVEL_LOW;
        ensures data.button_was_pressed == true;
        ensures data.pump.configured_duration_level ==
            \old(data.pump.configured_duration_level);
        ensures data.display_overflow_pulse ==
            \old(data.display_overflow_pulse);

    behavior button_released_after_press:
        assumes GPIObits.GP3 == GPIO_LEVEL_HIGH;
        assumes data.button_was_pressed == true;
        ensures data.button_was_pressed == false;

    behavior button_released_idle:
        assumes GPIObits.GP3 == GPIO_LEVEL_HIGH;
        assumes data.button_was_pressed == false;
        assigns data.button_was_pressed;
        ensures data.button_was_pressed == false;
        ensures data.pump.configured_duration_level ==
            \old(data.pump.configured_duration_level);
        ensures data.display_overflow_pulse ==
            \old(data.display_overflow_pulse);

    complete behaviors;
    disjoint behaviors;
*/
void handle_button(void)
{
    /* Button is currently pressed (active LOW). */
    if (GPIO_GET(GPIO_USER_BUTTON_INPUT) == false)
    {
        data.button_was_pressed = true;
        return;
    }

    /* Button released: detect press-release cycle. */
    if (data.button_was_pressed)
    {
        logDebugLow("Button released");
        update_pump_duration();
    }

    /* Clear stored state for next detection cycle. */
    data.button_was_pressed = false;
}

/*@
    requires data.pump.configured_duration_level >= PUMP_DURATION_LEVEL_MIN;
    requires data.pump.configured_duration_level <= PUMP_DURATION_LEVEL_MAX;
    requires GPIObits.GP1 == 0 || GPIObits.GP1 == 1;
    requires data.pump.remaining_cycle_levels > 0
        ==> data.pump.level_remaining_seconds > 0;

    assigns data.pump.remaining_cycle_levels,
        data.pump.level_remaining_seconds;

    ensures data.pump.remaining_cycle_levels > 0
        ==> data.pump.level_remaining_seconds > 0;

    behavior soil_dry:
        assumes GPIObits.GP1 == GPIO_LEVEL_HIGH;
        ensures data.pump.remaining_cycle_levels ==
            data.pump.configured_duration_level;
        ensures data.pump.level_remaining_seconds == PUMP_STEP_DURATION_SECONDS;

    behavior soil_moist:
        assumes GPIObits.GP1 == GPIO_LEVEL_LOW;
        assigns \nothing;

    complete behaviors;
    disjoint behaviors;
*/
void handle_sensor_check(void)
{
    if (GPIO_GET(GPIO_SOIL_SENSOR_INPUT))
    {
        logWarning("Soil dry - starting watering (level %d, %ds)",
                   data.pump.configured_duration_level,
                   data.pump.configured_duration_level * PUMP_STEP_DURATION_SECONDS);

        /* Pump will be activated during the next handle_pump() call. */
        data.pump.remaining_cycle_levels = data.pump.configured_duration_level;
        data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;
    }
    else
    {
        logDebugHigh("Soil check OK - moisture sufficient");
    }
}

/*@
    requires data.send_pulse_to_display == true ||
        data.send_pulse_to_display == false;
    requires data.sending_pulse_to_display == true ||
        data.sending_pulse_to_display == false;
    requires data.display_overflow_pulse == true ||
        data.display_overflow_pulse == false;

    assigns data.send_pulse_to_display,
        data.sending_pulse_to_display,
        data.display_overflow_pulse,
        GPIObits;

    behavior start_pulse:
        assumes data.send_pulse_to_display == true;
        ensures data.send_pulse_to_display == false;
        ensures data.sending_pulse_to_display == true;
        ensures data.display_overflow_pulse ==
            \old(data.display_overflow_pulse);
        ensures GPIObits.GP0 == GPIO_LEVEL_HIGH;

    behavior complete_pulse_no_overflow:
        assumes data.send_pulse_to_display == false;
        assumes data.sending_pulse_to_display == true;
        assumes data.display_overflow_pulse == false;
        ensures data.sending_pulse_to_display == false;
        ensures data.send_pulse_to_display == false;
        ensures data.display_overflow_pulse == false;
        ensures GPIObits.GP0 == GPIO_LEVEL_LOW;

    behavior complete_pulse_with_overflow:
        assumes data.send_pulse_to_display == false;
        assumes data.sending_pulse_to_display == true;
        assumes data.display_overflow_pulse == true;
        ensures data.sending_pulse_to_display == false;
        ensures data.display_overflow_pulse == false;
        ensures data.send_pulse_to_display == true;
        ensures GPIObits.GP0 == GPIO_LEVEL_LOW;

    behavior idle:
        assumes data.send_pulse_to_display == false;
        assumes data.sending_pulse_to_display == false;
        assigns \nothing;

    complete behaviors;
    disjoint behaviors;
*/
void handle_display(void)
{
    if (data.send_pulse_to_display == true)
    {
        data.send_pulse_to_display = false;
        data.sending_pulse_to_display = true;
        GPIO_SET(GPIO_DISPLAY_DATA_OUTPUT, GPIO_LEVEL_HIGH);
    }
    else if (data.sending_pulse_to_display == true)
    {
        data.sending_pulse_to_display = false;
        GPIO_SET(GPIO_DISPLAY_DATA_OUTPUT, GPIO_LEVEL_LOW);

        /* After 9->0 overflow, send second pulse to reach 1 */
        if (data.display_overflow_pulse == true)
        {
            data.display_overflow_pulse = false;
            data.send_pulse_to_display = true;
        }
    }
}

/*@
    requires data.pump.configured_duration_level >= PUMP_DURATION_LEVEL_MIN;
    requires data.pump.configured_duration_level <= PUMP_DURATION_LEVEL_MAX;
    requires data.pump.remaining_cycle_levels > 0
        ==> data.pump.level_remaining_seconds > 0;

    assigns data.send_pulse_to_display,
        data.sending_pulse_to_display,
        data.pump.configured_duration_level,
        data.display_overflow_pulse;

    ensures data.pump.configured_duration_level >= PUMP_DURATION_LEVEL_MIN;
    ensures data.pump.configured_duration_level <= PUMP_DURATION_LEVEL_MAX;
    ensures data.pump.remaining_cycle_levels > 0
        ==> data.pump.level_remaining_seconds > 0;

    behavior pump_idle:
        assumes data.pump.remaining_cycle_levels == 0;
        ensures data.send_pulse_to_display == true;
        ensures data.sending_pulse_to_display == false;

        ensures \old(data.pump.configured_duration_level) < PUMP_DURATION_LEVEL_MAX
            ==> data.pump.configured_duration_level ==
                \old(data.pump.configured_duration_level) + 1;

        ensures \old(data.pump.configured_duration_level) < PUMP_DURATION_LEVEL_MAX
            ==> data.display_overflow_pulse ==
                \old(data.display_overflow_pulse);

        ensures \old(data.pump.configured_duration_level) == PUMP_DURATION_LEVEL_MAX
            ==> data.pump.configured_duration_level == PUMP_DURATION_LEVEL_MIN;

        ensures \old(data.pump.configured_duration_level) == PUMP_DURATION_LEVEL_MAX
            ==> data.display_overflow_pulse == true;

    behavior pump_running:
        assumes data.pump.remaining_cycle_levels > 0;
        assigns \nothing;

    complete behaviors;
    disjoint behaviors;
*/
void update_pump_duration(void)
{
    /* Update only when the pump is not running to avoid ambiguity
       between the previous and newly selected duration. */
    if (data.pump.remaining_cycle_levels == 0U)
    {
        /* Trigger a display pulse to reflect the new level. */
        data.send_pulse_to_display = true;
        data.sending_pulse_to_display = false;

        data.pump.configured_duration_level++;

        /* Explicit comparison is used instead of a modulo operation.
           The target device has no native modulo support, and the compiler
           would generate significantly more code for modulo handling. */
        if (data.pump.configured_duration_level > PUMP_DURATION_LEVEL_MAX)
        {
            data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;

            /* Display overflows 9->0; schedule second pulse to reach 1 */
            data.display_overflow_pulse = true;
        }

        logInfo("Duration set to level %d (%ds)",
                data.pump.configured_duration_level,
                data.pump.configured_duration_level * PUMP_STEP_DURATION_SECONDS);
    }
}
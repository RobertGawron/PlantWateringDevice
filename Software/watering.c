#include "watering.h"


/* ================================================================
 * GPIO LEVEL DEFINITIONS
 * ================================================================ */

#define GPIO_LEVEL_LOW (0U)
#define GPIO_LEVEL_HIGH (1U)

/* ================================================================
 * PUMP
 * ================================================================ */



/** Maximum selectable pump duration (in steps). */
#define PUMP_DURATION_LEVEL_MAX (9U)



#define PUMP_MOSFET_ON GPIO_LEVEL_HIGH
#define PUMP_MOSFET_OFF GPIO_LEVEL_LOW

#define PUMP_MOSFET_SET(STATE) (GPIO_PUMP_MOSFET_OUTPUT = (STATE))

#define PUMP_MAX_RUNTIME_SECONDS \
    (PUMP_DURATION_LEVEL_MAX * PUMP_STEP_DURATION_SECONDS)

/* ================================================================
 * SOIL SENSOR
 * ================================================================ */

#define SOIL_SENSOR_DRY GPIO_LEVEL_HIGH

#define SOIL_IS_DRY(SOIL_SENSOR) \
    ((SOIL_SENSOR) == SOIL_SENSOR_DRY)

#define SOIL_CHECK_INTERVAL_SECONDS \
    (TIME_SECONDS_PER_MINUTE * TIME_MINUTES_PER_HOUR)

/* ================================================================
 * DISPLAY
 * ================================================================ */

#define DISPLAY_SIGNAL_HIGH GPIO_LEVEL_HIGH
#define DISPLAY_SIGNAL_LOW GPIO_LEVEL_LOW

#define DISPLAY_SIGNAL_SET(state) \
    (GPIO_DISPLAY_DATA_OUTPUT = (state))

/* ================================================================
 * BUTTON
 * ================================================================ */

/* The button is connected to GND with a pull-up resistor to VCC;
   therefore, pressing the button signals a LOW state. */
#define BUTTON_ACTIVE_LEVEL GPIO_LEVEL_LOW

#define BUTTON_IS_ACTIVE(PIN_VALUE) \
    ((PIN_VALUE) == BUTTON_ACTIVE_LEVEL)

/* ================================================================
 * HARDWARE ABSTRACTION
 * ================================================================ */



/* GPIO SIGNAL MAPPING */
#define GPIO_DISPLAY_DATA_OUTPUT GPIObits.GP0
#define GPIO_SOIL_SENSOR_INPUT GPIObits.GP1
#define GPIO_PUMP_MOSFET_OUTPUT GPIObits.GP2
#define GPIO_USER_BUTTON_INPUT GPIObits.GP3




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
                .duration_level = PUMP_DURATION_LEVEL_MIN,
                .remaining_level = 0U,
                .level_remaining_seconds = PUMP_STEP_DURATION_SECONDS},

        .time =
            {
                .tick = 0U,
                .seconds = SOIL_CHECK_STARTUP_SECONDS_INIT,
                .minutes = SOIL_CHECK_STARTUP_MINUTES_INIT},

        .button_was_pressed = false,
        .send_pulse_to_display = false,
        .sending_pulse_to_display = false};



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
    PUMP_MOSFET_SET(PUMP_MOSFET_OFF);
    DISPLAY_SIGNAL_SET(DISPLAY_SIGNAL_LOW);
}

void handle_pump(void)
{
    if (data.pump.remaining_level > 0U)
    {
        PUMP_MOSFET_SET(PUMP_MOSFET_ON);

        if (--data.pump.level_remaining_seconds == 0U)
        {
            data.pump.remaining_level--;

            if (data.pump.remaining_level > 0U)
            {
                data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;
            }
        }
    }
    else
    {
        PUMP_MOSFET_SET(PUMP_MOSFET_OFF);
    }
}

void handle_button(void)
{
    /* Button is currently pressed (active LOW). */
    if (BUTTON_IS_ACTIVE(GPIO_USER_BUTTON_INPUT))
    {
        data.button_was_pressed = true;
        return;
    }

    /* Button released: detect press-release cycle. */
    if (data.button_was_pressed)
    {
        update_pump_duration();
    }

    /* Clear stored state for next detection cycle. */
    data.button_was_pressed = false;
}

void handle_sensor_check(void)
{
    if (SOIL_IS_DRY(GPIO_SOIL_SENSOR_INPUT))
    {
        /* Pump will be activated during the next handle_pump() call. */
        data.pump.remaining_level = data.pump.duration_level;
        data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;
    }
}

void handle_display(void)
{

    if (data.send_pulse_to_display == true)
    {
        data.send_pulse_to_display = false;
        data.sending_pulse_to_display = true;

        DISPLAY_SIGNAL_SET(DISPLAY_SIGNAL_HIGH);
    }

    if (data.sending_pulse_to_display == true)
    {
        data.sending_pulse_to_display = false;

        DISPLAY_SIGNAL_SET(DISPLAY_SIGNAL_LOW);
    }
}

void update_pump_duration(void)
{
    /* Update only when the pump is not running to avoid ambiguity
       between the previous and newly selected duration. */
    if (data.pump.remaining_level == 0U)
    {
        /* Trigger a display pulse to reflect the new level. */
        data.send_pulse_to_display = true;
        data.sending_pulse_to_display = false;

        data.pump.duration_level++;

        /* Explicit comparison is used instead of a modulo operation.
           The target device has no native modulo support, and the compiler
           would generate significantly more code for modulo handling. */
        if (data.pump.duration_level > PUMP_DURATION_LEVEL_MAX)
        {
            data.pump.duration_level = PUMP_DURATION_LEVEL_MIN;
        }
    }
}
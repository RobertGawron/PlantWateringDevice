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

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

/* ================================================================
 * CLOCK CONFIGURATION
 * ================================================================ */

#define MCU_CLOCK_FREQUENCY_HZ (4000000UL)
/* Oscillator frequency (required by __delay_ms) */
#define _XTAL_FREQ MCU_CLOCK_FREQUENCY_HZ

/* ================================================================
 * TIME CONSTANTS
 * ================================================================ */

#define TIME_BASE_TICK_MS (20U)
#define TIME_MILLISECONDS_PER_SECOND (1000U)

#define TIME_SECONDS_PER_MINUTE (60U)
#define TIME_MINUTES_PER_HOUR (60U)

#define TIME_TICKS_PER_SECOND \
    (TIME_MILLISECONDS_PER_SECOND / TIME_BASE_TICK_MS)

/* ================================================================
 * GPIO LEVEL DEFINITIONS
 * ================================================================ */

#define GPIO_LEVEL_LOW (0U)
#define GPIO_LEVEL_HIGH (1U)

/* ================================================================
 * PUMP
 * ================================================================ */

/** Minimum selectable pump duration (in steps). */
#define PUMP_DURATION_LEVEL_MIN (1U)

/** Maximum selectable pump duration (in steps). */
#define PUMP_DURATION_LEVEL_MAX (9U)

/** Duration of one step in seconds. */
#define PUMP_STEP_DURATION_SECONDS (5U)

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

#define HW_DELAY_MS(DURATION_MS) __delay_ms(DURATION_MS)

/* GPIO SIGNAL MAPPING */
#define GPIO_DISPLAY_DATA_OUTPUT GPIObits.GP0
#define GPIO_SOIL_SENSOR_INPUT GPIObits.GP1
#define GPIO_PUMP_MOSFET_OUTPUT GPIObits.GP2
#define GPIO_USER_BUTTON_INPUT GPIObits.GP3

/* ================================================================
 * STARTUP BEHAVIOR
 * ================================================================ */

/** Delay in seconds before the first soil moisture check after power-up. */
#define SOIL_CHECK_STARTUP_DELAY_SECONDS (10U)

#define SOIL_CHECK_STARTUP_MINUTES_INIT \
    (TIME_MINUTES_PER_HOUR - 1U)

#define SOIL_CHECK_STARTUP_SECONDS_INIT \
    (TIME_SECONDS_PER_MINUTE - SOIL_CHECK_STARTUP_DELAY_SECONDS)

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

typedef struct
{
    /** Configured pump duration level (1 to PUMP_DURATION_LEVEL_MAX). */
    uint8_t duration_level;

    /** Remaining duration levels in current watering cycle. */
    uint8_t remaining_level;

    /** Seconds remaining in current duration level. */
    uint8_t level_remaining_seconds;

} PumpState;

typedef struct
{
    /** Base tick accumulator (0 to TIME_TICKS_PER_SECOND - 1). */
    uint8_t tick;

    /** Seconds accumulator (0-59). */
    uint8_t seconds;

    /** Minutes accumulator (0-59). */
    uint8_t minutes;

} TimeBaseState;

/**
 * @brief Aggregated runtime state of the controller.
 *
 * Grouped to ensure deterministic layout and compact packing.
 */
typedef struct
{
    PumpState pump;

    TimeBaseState time;

    /**
     * @brief Previous button state for edge detection.
     *
     * @note Bitfields may increase code size due to bit offset handling.
     *       No overhead observed here, so it is left as is.
     */
    bool button_was_pressed : 1;
    bool send_pulse_to_display : 1;
    bool sending_pulse_to_display : 1;

} PlantWateringData;

/**
 * @note The 'Initialize data' option remains enabled.
 * Manual initialization at startup would produce similar code size
 * but increases risk of initialization error.
 */
static PlantWateringData data =
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

static inline void initialize(void);

static inline void handle_button(void);
static inline void handle_pump(void);
static inline void handle_sensor_check(void);
static inline void handle_display(void);

static inline void update_pump_duration(void);

/**
 * @note Button and display handling depend directly on the 20 ms tick period.
 *       This could be decoupled, but doing so would require additional code
 *       that is not necessary. A 20 ms interval is acceptable for both
 *       button processing and driving the CD4026.
 */
void main(void)
{
    initialize();

    while (true)
    {
        HW_DELAY_MS(TIME_BASE_TICK_MS);

        handle_button();
        handle_display();

        /* 20 ms base tick accumulation */
        if (++data.time.tick >= TIME_TICKS_PER_SECOND)
        {
            data.time.tick = 0;

            /* 1 second elapsed */
            if (++data.time.seconds >= TIME_SECONDS_PER_MINUTE)
            {
                data.time.seconds = 0;

                /* 1 minute elapsed */
                if (++data.time.minutes >= TIME_MINUTES_PER_HOUR)
                {
                    data.time.minutes = 0;

                    /* 1 hour elapsed */
                    handle_sensor_check();
                }
            }

            /* Call once per second */
            handle_pump();
        }
    }
}

/**
 * @brief Configures MCU registers and initializes hardware state.
 */
static inline void initialize(void)
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

/**
 * @brief Controls pump MOSFT output.
 * @note Called once per second.
 */
static inline void handle_pump(void)
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

/**
 * @brief Processes button state and detects a press–release event.
 *
 * Increments the pump duration level on the release edge only.
 *
 * @note Assumption: Called every tick (20 ms).
 */
static inline void handle_button(void)
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

/**
 * @brief Checks soil moisture and schedules pump activation if required.
 *
 * @note Assumption: Called every hour.
 */
static inline void handle_sensor_check(void)
{
    if (SOIL_IS_DRY(GPIO_SOIL_SENSOR_INPUT))
    {
        /* Pump will be activated during the next handle_pump() call. */
        data.pump.remaining_level = data.pump.duration_level;
        data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;
    }
}

/**
 * @brief Generates a clock pulse on the CD4026 CLK input.
 *
 * Increments the digit displayed on the seven-segment display.
 * Wrapping is handled internally by the CD4026; no additional
 * software logic is required.
 *
 * @note Assumption: Called every tick (20 ms).
 */
static inline void handle_display(void)
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

/**
 * @brief Increments the pump duration level with wrap-around.
 *
 * @note Assumption: Executed only when the pump is idle.
 */
static inline void update_pump_duration(void)
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
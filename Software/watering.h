#ifndef WATERING_H
#define WATERING_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

/* ================================================================
 * PUMP CONFIGURATION
 * ================================================================ */

/** Minimum selectable pump duration level (in steps). */
#define PUMP_DURATION_LEVEL_MIN (1U)

/** Duration of one pump level in seconds. */
#define PUMP_STEP_DURATION_SECONDS (5U)

/* ================================================================
 * TIMEBASE CONFIGURATION
 * ================================================================ */

/** Base system tick period in milliseconds. */
#define TIME_BASE_TICK_MS (20U)

/** Number of milliseconds in one second. */
#define TIME_MILLISECONDS_PER_SECOND (1000U)

/** Number of seconds in one minute. */
#define TIME_SECONDS_PER_MINUTE (60U)

/** Number of minutes in one hour. */
#define TIME_MINUTES_PER_HOUR (60U)

/* ================================================================
 * DERIVED TIME CONSTANTS
 * ================================================================ */

/**
 * Number of base ticks required to reach one second.
 * TIME_BASE_TICK_MS must divide evenly into 1000.
 */
#define TIME_TICKS_PER_SECOND \
    (TIME_MILLISECONDS_PER_SECOND / TIME_BASE_TICK_MS)

/* ================================================================
 * STARTUP BEHAVIOR CONFIGURATION
 * ================================================================ */

/**
 * Delay in seconds before the first soil moisture check
 * after power-up to allow system stabilization.
 */
#define SOIL_CHECK_STARTUP_DELAY_SECONDS (10U)

/**
 * Initial minute counter value used to schedule
 * the first soil check after startup delay.
 */
#define SOIL_CHECK_STARTUP_MINUTES_INIT \
    (TIME_MINUTES_PER_HOUR - 1U)

/**
 * Initial seconds counter value used to schedule
 * the first soil check after startup delay.
 */
#define SOIL_CHECK_STARTUP_SECONDS_INIT \
    (TIME_SECONDS_PER_MINUTE - SOIL_CHECK_STARTUP_DELAY_SECONDS)

/* ================================================================
 * HARDWARE ABSTRACTION
 * ================================================================ */

/**
 * Hardware delay abstraction.
 * On target, maps to __delay_ms().
 */
#define HW_DELAY_MS(DURATION_MS) __delay_ms(DURATION_MS)

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

extern PlantWateringData data;

/**
 * @brief Configures MCU registers and initializes hardware state.
 */
void initialize(void);

/**
 * @brief Controls pump MOSFT output.
 * @note Called once per second.
 */
void handle_pump(void);

/**
 * @brief Processes button state and detects a press–release event.
 *
 * Increments the pump duration level on the release edge only.
 *
 * @note Assumption: Called every tick (20 ms).
 */
void handle_button(void);

/**
 * @brief Checks soil moisture and schedules pump activation if required.
 *
 * @note Assumption: Called every hour.
 */
void handle_sensor_check(void);

/**
 * @brief Generates a clock pulse on the CD4026 CLK input.
 *
 * Increments the digit displayed on the seven-segment display.
 * Wrapping is handled internally by the CD4026; no additional
 * software logic is required.
 *
 * @note Assumption: Called every tick (20 ms).
 */
void handle_display(void);

/**
 * @brief Increments the pump duration level with wrap-around.
 *
 * @note Assumption: Executed only when the pump is idle.
 */
void update_pump_duration(void);

#endif
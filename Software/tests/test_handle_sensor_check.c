/**
 * @file test_handle_sensor_check.c
 *
 * @brief Unit tests for the soil moisture sensor check logic.
 *
 * @details
 * handle_sensor_check() is called once per hour by the main loop
 * timebase. It reads the soil moisture sensor and, if the soil is
 * dry, schedules a pump activation cycle.
 *
 * Behavior:
 *
 * 1. Sensor read: GPIO_SOIL_SENSOR_INPUT is sampled.
 *    HIGH = soil is dry (watering required).
 *    LOW  = soil is wet (no action).
 *
 * 2. Activation: When soil is dry:
 *    - remaining_cycle_levels is loaded from configured_duration_level.
 *    - level_remaining_seconds is reset to PUMP_STEP_DURATION_SECONDS.
 *
 * 3. No action: When soil is wet, no state is modified.
 *
 * Architectural constraints:
 * - Pump runtime is shorter than the soil-check interval; therefore,
 *   the pump will always be idle when handle_sensor_check is called
 *   under normal operation. However, the function itself does not
 *   enforce this.
 *
 * The tests below verify sensor response, state loading, boundary
 * conditions, and state isolation.
 */

#include "unity.h"
#include "xc.h"
#include "watering.h"

extern void handle_sensor_check(void);
extern PlantWateringData data;

#define SOIL_SENSOR_PIN GPIObits.GP1

#define GPIO_LEVEL_LOW (0U)
#define GPIO_LEVEL_HIGH (1U)

#define SOIL_DRY GPIO_LEVEL_HIGH
#define SOIL_WET GPIO_LEVEL_LOW

/**
 * @note PUMP_DURATION_LEVEL_MAX is defined in watering.c (translation
 *       unit scope). Duplicated here for boundary verification.
 *       Must be kept in sync with the production definition.
 */
#define PUMP_DURATION_LEVEL_MAX (9U)

/** Minimum selectable pump duration level (in steps). */
#define PUMP_DURATION_LEVEL_MIN (1U)

/** Duration of one pump level in seconds. */
#define PUMP_STEP_DURATION_SECONDS (5U)

void setUp(void)
{
    SOIL_SENSOR_PIN = SOIL_WET;
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 0U;
    data.send_pulse_to_display = false;
    data.sending_pulse_to_display = false;
    data.button_was_pressed = false;
}

void tearDown(void) {}

/* ================================================================
 * GROUP 1: SOIL WET — NO ACTION
 * ================================================================ */

/**
 * @brief Verify no activation when soil is wet.
 */
void test_wet_no_activation(void)
{
    SOIL_SENSOR_PIN = SOIL_WET;
    data.pump.configured_duration_level = 5U;
    data.pump.remaining_cycle_levels = 0U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
}

/**
 * @brief Verify level_remaining_seconds unchanged when wet.
 */
void test_wet_seconds_unchanged(void)
{
    SOIL_SENSOR_PIN = SOIL_WET;
    data.pump.level_remaining_seconds = 0U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.level_remaining_seconds);
}

/**
 * @brief Verify level_remaining_seconds with nonzero stale value
 *        is not modified when wet.
 *
 * Guards against the function resetting seconds to
 * PUMP_STEP_DURATION_SECONDS regardless of sensor state.
 */
void test_wet_stale_seconds_not_modified(void)
{
    SOIL_SENSOR_PIN = SOIL_WET;
    data.pump.level_remaining_seconds = 3U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.level_remaining_seconds);
}

/**
 * @brief Verify configured_duration_level is never modified
 *        when wet.
 */
void test_wet_configured_duration_unchanged(void)
{
    SOIL_SENSOR_PIN = SOIL_WET;
    data.pump.configured_duration_level = 7U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(7U, data.pump.configured_duration_level);
}

/**
 * @brief Verify remaining_cycle_levels with a stale nonzero value
 *        is not modified when wet.
 *
 * Although architecturally the pump is idle when this function is
 * called, the function itself must not clear remaining_cycle_levels.
 */
void test_wet_stale_remaining_not_modified(void)
{
    SOIL_SENSOR_PIN = SOIL_WET;
    data.pump.remaining_cycle_levels = 4U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(4U, data.pump.remaining_cycle_levels);
}

/**
 * @brief Verify display flags are not modified when wet.
 */
void test_wet_display_flags_unchanged(void)
{
    SOIL_SENSOR_PIN = SOIL_WET;
    data.send_pulse_to_display = false;
    data.sending_pulse_to_display = true;

    handle_sensor_check();

    TEST_ASSERT_FALSE(data.send_pulse_to_display);
    TEST_ASSERT_TRUE(data.sending_pulse_to_display);
}

/**
 * @brief Verify button state is not modified when wet.
 */
void test_wet_button_state_unchanged(void)
{
    SOIL_SENSOR_PIN = SOIL_WET;
    data.button_was_pressed = true;

    handle_sensor_check();

    TEST_ASSERT_TRUE(data.button_was_pressed);
}

/**
 * @brief Verify consecutive wet checks produce no state change.
 */
void test_wet_consecutive_calls_no_change(void)
{
    SOIL_SENSOR_PIN = SOIL_WET;
    data.pump.configured_duration_level = 3U;
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 0U;

    for (uint8_t i = 0; i < 10U; i++)
    {
        handle_sensor_check();
    }

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.level_remaining_seconds);
    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.configured_duration_level);
}

/* ================================================================
 * GROUP 2: SOIL DRY — ACTIVATION
 * ================================================================ */

/**
 * @brief Verify remaining_cycle_levels is loaded from
 *        configured_duration_level when dry.
 */
void test_dry_loads_remaining_from_configured(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = 5U;
    data.pump.remaining_cycle_levels = 0U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(5U, data.pump.remaining_cycle_levels);
}

/**
 * @brief Verify level_remaining_seconds is reset to
 *        PUMP_STEP_DURATION_SECONDS when dry.
 */
void test_dry_resets_seconds(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = 3U;
    data.pump.level_remaining_seconds = 0U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
}

/**
 * @brief Verify level_remaining_seconds is reset even if it
 *        already holds a stale nonzero value.
 */
void test_dry_resets_seconds_from_stale_value(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = 2U;
    data.pump.level_remaining_seconds = 99U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
}

/**
 * @brief Verify configured_duration_level is not modified when dry.
 *
 * The function must only read, not write, the user setting.
 */
void test_dry_configured_duration_unchanged(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = 8U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(8U, data.pump.configured_duration_level);
}

/* ================================================================
 * GROUP 3: BOUNDARY — MINIMUM CONFIGURED LEVEL
 * ================================================================ */

/**
 * @brief Verify activation with minimum configured level (1).
 *
 * remaining_cycle_levels must be exactly 1, not 0.
 */
void test_dry_min_level_loads_correctly(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN,
                            data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
}

/**
 * @brief Verify remaining_cycle_levels is not zero after min
 *        level activation.
 *
 * Explicit zero-exclusion. If remaining is zero, handle_pump
 * would never turn the pump on.
 */
void test_dry_min_level_remaining_not_zero(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    handle_sensor_check();

    TEST_ASSERT_NOT_EQUAL(0U, data.pump.remaining_cycle_levels);
}

/* ================================================================
 * GROUP 4: BOUNDARY — MAXIMUM CONFIGURED LEVEL
 * ================================================================ */

/**
 * @brief Verify activation with maximum configured level (9).
 */
void test_dry_max_level_loads_correctly(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MAX;
    data.pump.remaining_cycle_levels = 0U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MAX,
                            data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
}

/**
 * @brief Verify seconds is exactly PUMP_STEP_DURATION_SECONDS
 *        at max level, not scaled by level.
 *
 * Guards against an implementation that multiplies seconds by
 * the level count.
 */
void test_dry_max_level_seconds_not_scaled(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MAX;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
}

/* ================================================================
 * GROUP 5: EVERY CONFIGURED LEVEL
 * ================================================================ */

/**
 * @brief Verify correct activation for every level from MIN to MAX.
 *
 * Exhaustive sweep: detects any level-specific loading error.
 */
void test_dry_all_levels_load_correctly(void)
{
    for (uint8_t level = PUMP_DURATION_LEVEL_MIN;
         level <= PUMP_DURATION_LEVEL_MAX; level++)
    {
        SOIL_SENSOR_PIN = SOIL_DRY;
        data.pump.configured_duration_level = level;
        data.pump.remaining_cycle_levels = 0U;
        data.pump.level_remaining_seconds = 0U;

        handle_sensor_check();

        TEST_ASSERT_EQUAL_UINT8_MESSAGE(
            level, data.pump.remaining_cycle_levels,
            "remaining_cycle_levels mismatch");
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(
            PUMP_STEP_DURATION_SECONDS,
            data.pump.level_remaining_seconds,
            "level_remaining_seconds mismatch");
    }
}

/* ================================================================
 * GROUP 6: STATE ISOLATION — DRY PATH
 * ================================================================ */

/**
 * @brief Verify display flags are not modified when dry.
 *
 * handle_sensor_check must not trigger display pulses.
 */
void test_dry_display_flags_unchanged(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = 3U;
    data.send_pulse_to_display = false;
    data.sending_pulse_to_display = true;

    handle_sensor_check();

    TEST_ASSERT_FALSE(data.send_pulse_to_display);
    TEST_ASSERT_TRUE(data.sending_pulse_to_display);
}

/**
 * @brief Verify button state is not modified when dry.
 */
void test_dry_button_state_unchanged(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = 3U;
    data.button_was_pressed = true;

    handle_sensor_check();

    TEST_ASSERT_TRUE(data.button_was_pressed);
}

/* ================================================================
 * GROUP 7: SENSOR POLARITY VERIFICATION
 * ================================================================ */

/**
 * @brief Verify LOW sensor value is interpreted as wet (no action).
 *
 * Explicit polarity check. Detects inverted sensor logic.
 */
void test_sensor_low_is_wet(void)
{
    SOIL_SENSOR_PIN = GPIO_LEVEL_LOW;
    data.pump.configured_duration_level = 5U;
    data.pump.remaining_cycle_levels = 0U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
}

/**
 * @brief Verify HIGH sensor value is interpreted as dry (activate).
 *
 * Explicit polarity check. Detects inverted sensor logic.
 */
void test_sensor_high_is_dry(void)
{
    SOIL_SENSOR_PIN = GPIO_LEVEL_HIGH;
    data.pump.configured_duration_level = 5U;
    data.pump.remaining_cycle_levels = 0U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(5U, data.pump.remaining_cycle_levels);
}

/* ================================================================
 * GROUP 8: CONSECUTIVE DRY CHECKS (IDEMPOTENCY)
 * ================================================================ */

/**
 * @brief Verify consecutive dry checks reload state identically.
 *
 * If the sensor remains dry across multiple hourly checks and
 * the pump has already finished, the function must reload the
 * same values. This verifies no accumulation or drift occurs.
 */
void test_dry_consecutive_reloads_identical(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = 4U;

    for (uint8_t i = 0; i < 5U; i++)
    {
        data.pump.remaining_cycle_levels = 0U;
        data.pump.level_remaining_seconds = 0U;

        handle_sensor_check();

        TEST_ASSERT_EQUAL_UINT8(4U,
                                data.pump.remaining_cycle_levels);
        TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                                data.pump.level_remaining_seconds);
    }
}

/**
 * @brief Verify that calling handle_sensor_check while the pump
 *        is still running overwrites remaining_cycle_levels.
 *
 * Under normal operation this cannot happen (pump finishes before
 * next hourly check). However, the function has no guard against
 * it, so this test documents the actual behavior: an unconditional
 * overwrite occurs.
 */
void test_dry_overwrites_active_remaining(void)
{
    SOIL_SENSOR_PIN = SOIL_DRY;
    data.pump.configured_duration_level = 3U;
    data.pump.remaining_cycle_levels = 7U;
    data.pump.level_remaining_seconds = 2U;

    handle_sensor_check();

    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
}

/* ================================================================
 * GROUP 9: WET-DRY-WET TRANSITION SEQUENCE
 * ================================================================ */

/**
 * @brief Verify correct behavior across a wet-dry-wet sequence.
 *
 * Simulates a sensor transition: no action, activation, no action.
 * Verifies that only the dry check activates the pump and that
 * a subsequent wet check does not cancel or modify the activation.
 */
void test_wet_dry_wet_sequence(void)
{
    data.pump.configured_duration_level = 6U;
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 0U;

    /* Wet: no action */
    SOIL_SENSOR_PIN = SOIL_WET;
    handle_sensor_check();
    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);

    /* Dry: activate */
    SOIL_SENSOR_PIN = SOIL_DRY;
    handle_sensor_check();
    TEST_ASSERT_EQUAL_UINT8(6U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);

    /* Wet: must not cancel activation */
    SOIL_SENSOR_PIN = SOIL_WET;
    handle_sensor_check();
    TEST_ASSERT_EQUAL_UINT8(6U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
}

/* Unity test runner */
int main(void)
{
    UNITY_BEGIN();

    /* Group 1: Soil wet — no action */
    RUN_TEST(test_wet_no_activation);
    RUN_TEST(test_wet_seconds_unchanged);
    RUN_TEST(test_wet_stale_seconds_not_modified);
    RUN_TEST(test_wet_configured_duration_unchanged);
    RUN_TEST(test_wet_stale_remaining_not_modified);
    RUN_TEST(test_wet_display_flags_unchanged);
    RUN_TEST(test_wet_button_state_unchanged);
    RUN_TEST(test_wet_consecutive_calls_no_change);

    /* Group 2: Soil dry — activation */
    RUN_TEST(test_dry_loads_remaining_from_configured);
    RUN_TEST(test_dry_resets_seconds);
    RUN_TEST(test_dry_resets_seconds_from_stale_value);
    RUN_TEST(test_dry_configured_duration_unchanged);

    /* Group 3: Boundary — minimum level */
    RUN_TEST(test_dry_min_level_loads_correctly);
    RUN_TEST(test_dry_min_level_remaining_not_zero);

    /* Group 4: Boundary — maximum level */
    RUN_TEST(test_dry_max_level_loads_correctly);
    RUN_TEST(test_dry_max_level_seconds_not_scaled);

    /* Group 5: Every level */
    RUN_TEST(test_dry_all_levels_load_correctly);

    /* Group 6: State isolation — dry path */
    RUN_TEST(test_dry_display_flags_unchanged);
    RUN_TEST(test_dry_button_state_unchanged);

    /* Group 7: Sensor polarity */
    RUN_TEST(test_sensor_low_is_wet);
    RUN_TEST(test_sensor_high_is_dry);

    /* Group 8: Consecutive dry checks */
    RUN_TEST(test_dry_consecutive_reloads_identical);
    RUN_TEST(test_dry_overwrites_active_remaining);

    /* Group 9: Transition sequence */
    RUN_TEST(test_wet_dry_wet_sequence);

    return UNITY_END();
}
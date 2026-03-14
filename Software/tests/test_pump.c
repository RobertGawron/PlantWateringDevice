/**
 * @file test_pump.c
 *
 * @brief Unit tests for the pump runtime state machine.
 *
 * @details
 * The pump runtime logic is based on discrete "duration levels".
 * This approach avoids using multiplication, since the PIC10F202
 * does not provide a hardware multiplication instruction, and
 * software emulation would significantly increase memory and
 * program size.
 *
 * The user configures the watering duration by selecting a level.
 * Each level represents a fixed time step of 5 seconds.
 *
 * Level interpretation:
 *  - Level 1 -> 5 seconds
 *  - Level 2 -> 10 seconds
 *  - ...
 *  - Level 9 -> 45 seconds
 *
 * Each level corresponds to PUMP_STEP_DURATION_SECONDS seconds
 * of pump activity.
 *
 * Runtime model:
 *
 * 1. remaining_cycle_levels indicates how many duration levels remain
 *    in the current watering cycle.
 * 2. level_remaining_seconds indicates the remaining seconds within
 *    the current level.
 * 3. On each invocation (called once per second):
 *      - If remaining_cycle_levels == 0 -> pump is OFF.
 *      - If remaining_cycle_levels > 0 -> pump is ON.
 *      - level_remaining_seconds is decremented.
 *      - When level_remaining_seconds reaches 0:
 *            remaining_cycle_levels is decremented.
 *            If remaining_cycle_levels > 0, level_remaining_seconds
 *            resets to PUMP_STEP_DURATION_SECONDS.
 *
 * The tests below verify all state transitions and boundary conditions
 * of this state machine.
 */

#include "unity.h"
#include "xc.h"
#include "watering.h"

extern void handle_pump(void);
extern PlantWateringData data;

#define MOSFET_PIN GPIObits.GP2

/* Test-local definitions matching production polarity */
#define GPIO_LEVEL_LOW (0U)
#define GPIO_LEVEL_HIGH (1U)

#define PUMP_ON GPIO_LEVEL_HIGH
#define PUMP_OFF GPIO_LEVEL_LOW

/** Minimum selectable pump duration level (in steps). */
#define PUMP_DURATION_LEVEL_MIN (1U)

/** Duration of one pump level in seconds. */
#define PUMP_STEP_DURATION_SECONDS (5U)

void setUp(void)
{
    MOSFET_PIN = GPIO_LEVEL_LOW;
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 0U;
}

void tearDown(void) {}

/* ================================================================
 * GROUP 1: IDLE STATE BEHAVIOR
 * ================================================================ */

/**
 * @brief Verify pump remains OFF when no remaining levels exist.
 */
void test_idle_pump_remains_off(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 0U;

    handle_pump();

    TEST_ASSERT_EQUAL_UINT8(PUMP_OFF, MOSFET_PIN);
}

/**
 * @brief Verify pump remains OFF even if level_remaining_seconds
 *        contains a stale nonzero value.
 *
 * Guards against a logic error where level_remaining_seconds is
 * evaluated independently of remaining_cycle_levels.
 */
void test_idle_pump_off_despite_stale_seconds(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    handle_pump();

    TEST_ASSERT_EQUAL_UINT8(PUMP_OFF, MOSFET_PIN);
}

/**
 * @brief Verify level_remaining_seconds is not modified when idle.
 *
 * handle_pump must not decrement or reset level_remaining_seconds
 * when remaining_cycle_levels is zero.
 */
void test_idle_does_not_modify_level_remaining_seconds(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 3U;

    handle_pump();

    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.level_remaining_seconds);
}

/**
 * @brief Verify remaining_cycle_levels is not modified when idle.
 */
void test_idle_does_not_modify_remaining_cycle_levels(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 0U;

    handle_pump();

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
}

/**
 * @brief Verify pump remains OFF across multiple consecutive idle calls.
 */
void test_consecutive_idle_calls_remain_off(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 0U;

    for (uint8_t i = 0; i < 10U; i++)
    {
        handle_pump();
        TEST_ASSERT_EQUAL_UINT8(PUMP_OFF, MOSFET_PIN);
    }
}

/* ================================================================
 * GROUP 2: ACTIVE STATE — BASIC BEHAVIOR
 * ================================================================ */

/**
 * @brief Verify pump turns ON when remaining_cycle_levels > 0.
 */
void test_active_pump_turns_on(void)
{
    data.pump.remaining_cycle_levels = 1U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    handle_pump();

    TEST_ASSERT_EQUAL_UINT8(PUMP_ON, MOSFET_PIN);
}

/**
 * @brief Verify level_remaining_seconds decrements by exactly 1
 *        per call and remaining_cycle_levels is unchanged.
 */
void test_active_seconds_decrement_without_level_change(void)
{
    data.pump.remaining_cycle_levels = 2U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    handle_pump();

    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS - 1U,
                            data.pump.level_remaining_seconds);
    TEST_ASSERT_EQUAL_UINT8(2U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_ON, MOSFET_PIN);
}

/**
 * @brief Verify configured_duration_level is never modified by
 *        handle_pump across an entire watering cycle.
 *
 * configured_duration_level is a user setting and must remain
 * constant throughout pump operation.
 */
void test_active_configured_duration_unchanged(void)
{
    data.pump.configured_duration_level = 7U;
    data.pump.remaining_cycle_levels = 3U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    /* Run through the entire cycle and beyond into idle */
    const uint8_t total_calls =
        (3U * PUMP_STEP_DURATION_SECONDS) + 5U;

    for (uint8_t i = 0; i < total_calls; i++)
    {
        handle_pump();
    }

    TEST_ASSERT_EQUAL_UINT8(7U, data.pump.configured_duration_level);
}

/* ================================================================
 * GROUP 3: LEVEL TRANSITIONS
 * ================================================================ */

/**
 * @brief Verify level transition: when seconds expire with multiple
 *        remaining levels, the level decrements and seconds reset.
 */
void test_level_transition_decrements_and_resets_seconds(void)
{
    data.pump.remaining_cycle_levels = 2U;
    data.pump.level_remaining_seconds = 1U;

    handle_pump();

    TEST_ASSERT_EQUAL_UINT8(1U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
    TEST_ASSERT_EQUAL_UINT8(PUMP_ON, MOSFET_PIN);
}

/**
 * @brief Verify seconds countdown through an entire single level.
 *
 * Traces every second within one level to detect off-by-one errors
 * in the decrement sequence.
 */
void test_seconds_countdown_through_entire_level(void)
{
    data.pump.remaining_cycle_levels = 1U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    for (uint8_t expected = PUMP_STEP_DURATION_SECONDS;
         expected > 1U; expected--)
    {
        handle_pump();
        TEST_ASSERT_EQUAL_UINT8(expected - 1U,
                                data.pump.level_remaining_seconds);
        TEST_ASSERT_EQUAL_UINT8(1U,
                                data.pump.remaining_cycle_levels);
    }

    /* Final second of this level */
    handle_pump();
    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.level_remaining_seconds);
}

/**
 * @brief Verify final level completion: remaining reaches 0,
 *        seconds remain 0, and pump is ON during this call.
 *
 * Critical safety check: the pump must be ON for exactly the
 * configured number of seconds, including this final second.
 */
void test_final_level_completion_pump_on_and_state_zeroed(void)
{
    data.pump.remaining_cycle_levels = 1U;
    data.pump.level_remaining_seconds = 1U;

    handle_pump();

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.level_remaining_seconds);
    /* Pump must be ON during this final active call */
    TEST_ASSERT_EQUAL_UINT8(PUMP_ON, MOSFET_PIN);
}

/**
 * @brief Verify that level_remaining_seconds is NOT reset to
 *        PUMP_STEP_DURATION_SECONDS when remaining_cycle_levels
 *        reaches zero.
 *
 * Documents the correct terminal state. If seconds were
 * erroneously reset, subsequent re-activation logic could
 * inherit a stale value.
 */
void test_final_level_seconds_not_reset(void)
{
    data.pump.remaining_cycle_levels = 1U;
    data.pump.level_remaining_seconds = 1U;

    handle_pump();

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.level_remaining_seconds);
    TEST_ASSERT_NOT_EQUAL(PUMP_STEP_DURATION_SECONDS,
                          data.pump.level_remaining_seconds);
}

/* ================================================================
 * GROUP 4: ACTIVE-TO-IDLE TRANSITION
 * ================================================================ */

/**
 * @brief Verify pump turns OFF on the first call after cycle
 *        completion.
 *
 * This is the critical safety transition: after the last active
 * second, the next call must turn the MOSFET OFF.
 */
void test_pump_off_on_call_after_completion(void)
{
    data.pump.remaining_cycle_levels = 1U;
    data.pump.level_remaining_seconds = 1U;

    /* Final active call */
    handle_pump();
    TEST_ASSERT_EQUAL_UINT8(PUMP_ON, MOSFET_PIN);

    /* First idle call — pump must be OFF */
    handle_pump();
    TEST_ASSERT_EQUAL_UINT8(PUMP_OFF, MOSFET_PIN);
}

/**
 * @brief Verify pump can be re-activated after returning to idle.
 */
void test_reactivation_after_completion(void)
{
    /* Complete a cycle */
    data.pump.remaining_cycle_levels = 1U;
    data.pump.level_remaining_seconds = 1U;
    handle_pump();
    handle_pump();
    TEST_ASSERT_EQUAL_UINT8(PUMP_OFF, MOSFET_PIN);

    /* Re-activate */
    data.pump.remaining_cycle_levels = 2U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    handle_pump();
    TEST_ASSERT_EQUAL_UINT8(PUMP_ON, MOSFET_PIN);
    TEST_ASSERT_EQUAL_UINT8(2U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS - 1U,
                            data.pump.level_remaining_seconds);
}

/* ================================================================
 * GROUP 5: FULL CYCLE END-TO-END VERIFICATION
 * ================================================================ */

/**
 * @brief Verify exact pump ON duration for a single-level cycle
 *        (level 1 = 5 seconds).
 */
void test_full_cycle_single_level(void)
{
    data.pump.remaining_cycle_levels = 1U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    const uint8_t expected_on = PUMP_STEP_DURATION_SECONDS;
    const uint8_t total_calls = expected_on + 1U;
    uint8_t on_count = 0U;

    for (uint8_t i = 0; i < total_calls; i++)
    {
        handle_pump();
        if (MOSFET_PIN == PUMP_ON)
        {
            on_count++;
        }
    }

    TEST_ASSERT_EQUAL_UINT8(expected_on, on_count);
    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_OFF, MOSFET_PIN);
}

/**
 * @brief Verify exact pump ON duration for a two-level cycle
 *        (level 2 = 10 seconds).
 */
void test_full_cycle_two_levels(void)
{
    data.pump.remaining_cycle_levels = 2U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    const uint8_t expected_on = 2U * PUMP_STEP_DURATION_SECONDS;
    const uint8_t total_calls = expected_on + 1U;
    uint8_t on_count = 0U;

    for (uint8_t i = 0; i < total_calls; i++)
    {
        handle_pump();
        if (MOSFET_PIN == PUMP_ON)
        {
            on_count++;
        }
    }

    TEST_ASSERT_EQUAL_UINT8(expected_on, on_count);
    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_OFF, MOSFET_PIN);
}

/**
 * @brief Verify exact pump ON duration for maximum level cycle
 *        (level 9 = 45 seconds).
 *
 * Boundary test for the highest user-selectable duration.
 */
void test_full_cycle_max_level(void)
{
    const uint8_t max_level = 9U;

    data.pump.remaining_cycle_levels = max_level;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    const uint8_t expected_on =
        max_level * PUMP_STEP_DURATION_SECONDS;
    const uint8_t total_calls = expected_on + 1U;
    uint8_t on_count = 0U;

    for (uint8_t i = 0; i < total_calls; i++)
    {
        handle_pump();
        if (MOSFET_PIN == PUMP_ON)
        {
            on_count++;
        }
    }

    TEST_ASSERT_EQUAL_UINT8(expected_on, on_count);
    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
    TEST_ASSERT_EQUAL_UINT8(PUMP_OFF, MOSFET_PIN);
}

/**
 * @brief Verify MOSFET is ON for every single call during an
 *        active cycle and OFF immediately after.
 *
 * Traces every call individually to detect any premature OFF
 * glitch during level transitions.
 */
void test_mosfet_on_every_active_call(void)
{
    data.pump.remaining_cycle_levels = 3U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    const uint8_t expected_on = 3U * PUMP_STEP_DURATION_SECONDS;

    for (uint8_t i = 0; i < expected_on; i++)
    {
        handle_pump();
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(
            PUMP_ON, MOSFET_PIN,
            "MOSFET unexpectedly OFF during active cycle");
    }

    /* Next call must turn OFF */
    handle_pump();
    TEST_ASSERT_EQUAL_UINT8(PUMP_OFF, MOSFET_PIN);
}

/* ================================================================
 * GROUP 6: MINIMUM ACTIVE STATE
 * ================================================================ */

/**
 * @brief Verify correct behavior with the minimum possible active
 *        state: one level, one remaining second.
 *
 * This is the smallest unit of pump activity. The pump must be ON
 * for exactly one call and OFF on the next.
 */
void test_minimum_active_state(void)
{
    data.pump.remaining_cycle_levels = 1U;
    data.pump.level_remaining_seconds = 1U;

    handle_pump();
    TEST_ASSERT_EQUAL_UINT8(PUMP_ON, MOSFET_PIN);
    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);

    handle_pump();
    TEST_ASSERT_EQUAL_UINT8(PUMP_OFF, MOSFET_PIN);
}

/* Unity test runner */
int main(void)
{
    UNITY_BEGIN();

    /* Group 1: Idle state */
    RUN_TEST(test_idle_pump_remains_off);
    RUN_TEST(test_idle_pump_off_despite_stale_seconds);
    RUN_TEST(test_idle_does_not_modify_level_remaining_seconds);
    RUN_TEST(test_idle_does_not_modify_remaining_cycle_levels);
    RUN_TEST(test_consecutive_idle_calls_remain_off);

    /* Group 2: Active state basics */
    RUN_TEST(test_active_pump_turns_on);
    RUN_TEST(test_active_seconds_decrement_without_level_change);
    RUN_TEST(test_active_configured_duration_unchanged);

    /* Group 3: Level transitions */
    RUN_TEST(test_level_transition_decrements_and_resets_seconds);
    RUN_TEST(test_seconds_countdown_through_entire_level);
    RUN_TEST(test_final_level_completion_pump_on_and_state_zeroed);
    RUN_TEST(test_final_level_seconds_not_reset);

    /* Group 4: Active-to-idle transition */
    RUN_TEST(test_pump_off_on_call_after_completion);
    RUN_TEST(test_reactivation_after_completion);

    /* Group 5: Full cycle end-to-end */
    RUN_TEST(test_full_cycle_single_level);
    RUN_TEST(test_full_cycle_two_levels);
    RUN_TEST(test_full_cycle_max_level);
    RUN_TEST(test_mosfet_on_every_active_call);

    /* Group 6: Minimum active state */
    RUN_TEST(test_minimum_active_state);

    return UNITY_END();
}
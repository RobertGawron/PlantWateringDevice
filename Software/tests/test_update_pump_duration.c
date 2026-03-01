/**
 * @file test_update_pump_duration.c
 *
 * @brief Unit tests for the pump duration level update logic.
 *
 * @details
 * update_pump_duration() is called on a button release event and
 * advances the user-configured pump duration level.
 *
 * Behavior:
 *
 * 1. Guard: The update is only performed when the pump is idle
 *    (remaining_cycle_levels == 0). This prevents ambiguity
 *    between the previous and newly selected duration during
 *    an active watering cycle.
 *
 * 2. Increment: configured_duration_level is incremented by 1.
 *
 * 3. Wrap-around: When configured_duration_level exceeds
 *    PUMP_DURATION_LEVEL_MAX (9), it wraps to
 *    PUMP_DURATION_LEVEL_MIN (1).
 *
 * 4. Display signaling: On a successful update,
 *    send_pulse_to_display is set to true and
 *    sending_pulse_to_display is forced to false
 *    to ensure a clean pulse initiation.
 *
 * The tests below verify all state transitions, boundary
 * conditions, guard enforcement, and state isolation.
 */

#include "unity.h"
#include "xc.h"
#include "watering.h"

extern void update_pump_duration(void);
extern PlantWateringData data;

/**
 * @note PUMP_DURATION_LEVEL_MAX is defined in watering.c (translation
 *       unit scope). Duplicated here for boundary verification.
 *       Must be kept in sync with the production definition.
 */
#define PUMP_DURATION_LEVEL_MAX (9U)

void setUp(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 0U;
    data.send_pulse_to_display = false;
    data.sending_pulse_to_display = false;
}

void tearDown(void) {}

/* ================================================================
 * GROUP 1: GUARD — PUMP IDLE (UPDATE PERMITTED)
 * ================================================================ */

/**
 * @brief Verify level increments when pump is idle.
 */
void test_increments_when_idle(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN + 1U,
                            data.pump.configured_duration_level);
}

/**
 * @brief Verify increment from an arbitrary mid-range value.
 */
void test_increments_mid_range(void)
{
    data.pump.configured_duration_level = 5U;
    data.pump.remaining_cycle_levels = 0U;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(6U, data.pump.configured_duration_level);
}

/* ================================================================
 * GROUP 2: GUARD — PUMP ACTIVE (UPDATE BLOCKED)
 * ================================================================ */

/**
 * @brief Verify level is NOT modified when pump is active
 *        with minimum remaining levels.
 */
void test_blocked_when_pump_active_one_level(void)
{
    data.pump.configured_duration_level = 3U;
    data.pump.remaining_cycle_levels = 1U;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.configured_duration_level);
}

/**
 * @brief Verify level is NOT modified when pump is active
 *        with maximum remaining levels.
 */
void test_blocked_when_pump_active_max_levels(void)
{
    data.pump.configured_duration_level = 3U;
    data.pump.remaining_cycle_levels = PUMP_DURATION_LEVEL_MAX;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.configured_duration_level);
}

/**
 * @brief Verify no display pulse is triggered when pump is active.
 *
 * Guards against partial execution where the display pulse is
 * triggered but the level increment is skipped, or vice versa.
 */
void test_no_display_pulse_when_pump_active(void)
{
    data.pump.remaining_cycle_levels = 2U;
    data.send_pulse_to_display = false;
    data.sending_pulse_to_display = false;

    update_pump_duration();

    TEST_ASSERT_FALSE(data.send_pulse_to_display);
    TEST_ASSERT_FALSE(data.sending_pulse_to_display);
}

/**
 * @brief Verify sending_pulse_to_display is not cleared when
 *        pump is active and a pulse is already in progress.
 *
 * If the guard is bypassed for the display reset but not the
 * increment, an in-progress pulse would be corrupted.
 */
void test_in_progress_pulse_not_corrupted_when_active(void)
{
    data.pump.remaining_cycle_levels = 1U;
    data.send_pulse_to_display = false;
    data.sending_pulse_to_display = true;

    update_pump_duration();

    TEST_ASSERT_TRUE(data.sending_pulse_to_display);
    TEST_ASSERT_FALSE(data.send_pulse_to_display);
}

/**
 * @brief Verify remaining_cycle_levels is never modified.
 *
 * update_pump_duration must not alter the pump runtime state.
 */
void test_remaining_cycle_levels_not_modified_when_active(void)
{
    data.pump.remaining_cycle_levels = 4U;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(4U, data.pump.remaining_cycle_levels);
}

/**
 * @brief Verify level_remaining_seconds is never modified
 *        when pump is active.
 */
void test_level_remaining_seconds_not_modified_when_active(void)
{
    data.pump.remaining_cycle_levels = 2U;
    data.pump.level_remaining_seconds = 3U;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.level_remaining_seconds);
}

/* ================================================================
 * GROUP 3: WRAP-AROUND BEHAVIOR
 * ================================================================ */

/**
 * @brief Verify wrap from MAX to MIN.
 *
 * Critical boundary: configured_duration_level must wrap to
 * PUMP_DURATION_LEVEL_MIN (1), not to 0.
 */
void test_wraps_from_max_to_min(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MAX;
    data.pump.remaining_cycle_levels = 0U;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN,
                            data.pump.configured_duration_level);
}

/**
 * @brief Verify no wrap at MAX - 1.
 *
 * One step before the boundary: must increment to MAX, not wrap.
 */
void test_no_wrap_at_max_minus_one(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MAX - 1U;
    data.pump.remaining_cycle_levels = 0U;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MAX,
                            data.pump.configured_duration_level);
}

/**
 * @brief Verify value after wrap is exactly MIN, not zero.
 *
 * Explicit zero-exclusion check. A common error is resetting
 * to 0 instead of MIN when MIN != 0.
 */
void test_wrap_does_not_produce_zero(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MAX;
    data.pump.remaining_cycle_levels = 0U;

    update_pump_duration();

    TEST_ASSERT_NOT_EQUAL(0U, data.pump.configured_duration_level);
}

/**
 * @brief Verify full traversal from MIN through MAX and back to MIN.
 *
 * Calls update_pump_duration exactly PUMP_DURATION_LEVEL_MAX times
 * starting from MIN. The final value must be MIN (one full wrap).
 * Each intermediate value is verified.
 */
void test_full_cycle_traversal(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    /* MIN+1, MIN+2, ..., MAX (that is MAX - MIN calls) */
    for (uint8_t expected = PUMP_DURATION_LEVEL_MIN + 1U;
         expected <= PUMP_DURATION_LEVEL_MAX; expected++)
    {
        update_pump_duration();
        TEST_ASSERT_EQUAL_UINT8(expected,
                                data.pump.configured_duration_level);
    }

    /* One more call: MAX -> MIN (wrap) */
    update_pump_duration();
    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN,
                            data.pump.configured_duration_level);
}

/**
 * @brief Verify two consecutive full cycles produce correct values.
 *
 * Guards against state corruption that only manifests on the
 * second traversal.
 */
void test_two_full_cycles(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    const uint8_t calls_per_cycle =
        PUMP_DURATION_LEVEL_MAX - PUMP_DURATION_LEVEL_MIN + 1U;

    for (uint8_t cycle = 0; cycle < 2U; cycle++)
    {
        for (uint8_t i = 0; i < calls_per_cycle; i++)
        {
            update_pump_duration();
        }

        TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN,
                                data.pump.configured_duration_level);
    }
}

/* ================================================================
 * GROUP 4: DISPLAY SIGNALING
 * ================================================================ */

/**
 * @brief Verify send_pulse_to_display is set on successful update.
 */
void test_display_pulse_requested_on_update(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.send_pulse_to_display = false;

    update_pump_duration();

    TEST_ASSERT_TRUE(data.send_pulse_to_display);
}

/**
 * @brief Verify sending_pulse_to_display is cleared on update.
 *
 * If a previous pulse was still in the "sending" phase,
 * update_pump_duration must reset it to ensure a clean new pulse.
 */
void test_sending_pulse_cleared_on_update(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.sending_pulse_to_display = true;

    update_pump_duration();

    TEST_ASSERT_FALSE(data.sending_pulse_to_display);
}

/**
 * @brief Verify display flags are set correctly even when
 *        send_pulse_to_display was already true.
 *
 * Consecutive rapid button presses could cause this scenario.
 */
void test_display_pulse_idempotent_when_already_pending(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.send_pulse_to_display = true;
    data.sending_pulse_to_display = false;

    update_pump_duration();

    TEST_ASSERT_TRUE(data.send_pulse_to_display);
    TEST_ASSERT_FALSE(data.sending_pulse_to_display);
}

/**
 * @brief Verify display pulse is triggered on wrap-around.
 *
 * The display must advance even at the MAX->MIN transition.
 * The CD4026 handles its own 9->0 wrap internally.
 */
void test_display_pulse_on_wrap(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MAX;
    data.pump.remaining_cycle_levels = 0U;
    data.send_pulse_to_display = false;

    update_pump_duration();

    TEST_ASSERT_TRUE(data.send_pulse_to_display);
}

/**
 * @brief Verify display pulse is triggered on every call through
 *        a full cycle.
 *
 * Each call must produce exactly one pulse request regardless of
 * the current level.
 */
void test_display_pulse_every_increment(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    const uint8_t total_calls =
        PUMP_DURATION_LEVEL_MAX - PUMP_DURATION_LEVEL_MIN + 1U;

    for (uint8_t i = 0; i < total_calls; i++)
    {
        data.send_pulse_to_display = false;

        update_pump_duration();

        TEST_ASSERT_TRUE_MESSAGE(
            data.send_pulse_to_display,
            "Display pulse missing during cycle traversal");
    }
}

/* ================================================================
 * GROUP 5: STATE ISOLATION
 * ================================================================ */

/**
 * @brief Verify remaining_cycle_levels is not modified on
 *        successful update.
 */
void test_remaining_cycle_levels_unchanged_on_update(void)
{
    data.pump.remaining_cycle_levels = 0U;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
}

/**
 * @brief Verify level_remaining_seconds is not modified on
 *        successful update.
 */
void test_level_remaining_seconds_unchanged_on_update(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
}

/**
 * @brief Verify level_remaining_seconds zero value is preserved
 *        on successful update.
 */
void test_level_remaining_seconds_zero_unchanged_on_update(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 0U;

    update_pump_duration();

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.level_remaining_seconds);
}

/* ================================================================
 * GROUP 6: INCREMENT EXACTNESS
 * ================================================================ */

/**
 * @brief Verify increment is exactly 1, not 2 or more.
 *
 * Guards against double-increment bugs (e.g., pre-increment
 * followed by post-increment, or re-entrant call).
 */
void test_increment_is_exactly_one(void)
{
    data.pump.remaining_cycle_levels = 0U;

    for (uint8_t start = PUMP_DURATION_LEVEL_MIN;
         start < PUMP_DURATION_LEVEL_MAX; start++)
    {
        data.pump.configured_duration_level = start;

        update_pump_duration();

        TEST_ASSERT_EQUAL_UINT8(start + 1U,
                                data.pump.configured_duration_level);
    }
}

/**
 * @brief Verify calling update once produces exactly one increment
 *        and not zero increments (no-op bug).
 */
void test_single_call_does_not_noop(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    const uint8_t before = data.pump.configured_duration_level;
    update_pump_duration();
    const uint8_t after = data.pump.configured_duration_level;

    TEST_ASSERT_NOT_EQUAL(before, after);
}

/* Unity test runner */
int main(void)
{
    UNITY_BEGIN();

    /* Group 1: Guard — pump idle */
    RUN_TEST(test_increments_when_idle);
    RUN_TEST(test_increments_mid_range);

    /* Group 2: Guard — pump active */
    RUN_TEST(test_blocked_when_pump_active_one_level);
    RUN_TEST(test_blocked_when_pump_active_max_levels);
    RUN_TEST(test_no_display_pulse_when_pump_active);
    RUN_TEST(test_in_progress_pulse_not_corrupted_when_active);
    RUN_TEST(test_remaining_cycle_levels_not_modified_when_active);
    RUN_TEST(test_level_remaining_seconds_not_modified_when_active);

    /* Group 3: Wrap-around */
    RUN_TEST(test_wraps_from_max_to_min);
    RUN_TEST(test_no_wrap_at_max_minus_one);
    RUN_TEST(test_wrap_does_not_produce_zero);
    RUN_TEST(test_full_cycle_traversal);
    RUN_TEST(test_two_full_cycles);

    /* Group 4: Display signaling */
    RUN_TEST(test_display_pulse_requested_on_update);
    RUN_TEST(test_sending_pulse_cleared_on_update);
    RUN_TEST(test_display_pulse_idempotent_when_already_pending);
    RUN_TEST(test_display_pulse_on_wrap);
    RUN_TEST(test_display_pulse_every_increment);

    /* Group 5: State isolation */
    RUN_TEST(test_remaining_cycle_levels_unchanged_on_update);
    RUN_TEST(test_level_remaining_seconds_unchanged_on_update);
    RUN_TEST(test_level_remaining_seconds_zero_unchanged_on_update);

    /* Group 6: Increment exactness */
    RUN_TEST(test_increment_is_exactly_one);
    RUN_TEST(test_single_call_does_not_noop);

    return UNITY_END();
}
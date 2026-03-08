/**
 * @file test_handle_button.c
 *
 * @brief Unit tests for the button press–release edge detection logic.
 *
 * @details
 * handle_button() is called once per tick (every 20 ms) and implements
 * a simple press–release edge detector.
 *
 * Behavior:
 *
 * 1. Button pressed (active LOW):
 *    - button_was_pressed is set to true.
 *    - Function returns immediately; no further action.
 *
 * 2. Button released (HIGH) after a press:
 *    - update_pump_duration() is called.
 *    - button_was_pressed is cleared to false.
 *
 * 3. Button released without prior press:
 *    - button_was_pressed is cleared to false.
 *    - No call to update_pump_duration().
 *
 * Key properties:
 * - Action occurs on the release edge only.
 * - A single press–release cycle produces exactly one call to
 *   update_pump_duration().
 * - Holding the button does not produce repeated actions.
 * - Debouncing is handled in hardware; this function assumes
 *   a clean signal.
 *
 * The tests below verify edge detection, state transitions,
 * guard behavior, and state isolation.
 */

#include "unity.h"
#include "xc.h"
#include "watering.h"

extern void handle_button(void);
extern PlantWateringData data;

#define BUTTON_PIN GPIObits.GP3

#define GPIO_LEVEL_LOW (0U)
#define GPIO_LEVEL_HIGH (1U)

#define BUTTON_PRESSED GPIO_LEVEL_LOW
#define BUTTON_RELEASED GPIO_LEVEL_HIGH

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
    BUTTON_PIN = BUTTON_RELEASED;
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = 0U;
    data.button_was_pressed = false;
    data.send_pulse_to_display = false;
    data.sending_pulse_to_display = false;
}

void tearDown(void) {}

/* ================================================================
 * GROUP 1: IDLE — NO BUTTON ACTIVITY
 * ================================================================ */

/**
 * @brief Verify no action when button is released and no prior
 *        press exists.
 *
 * This is the steady-state idle condition. No state must change.
 */
void test_idle_no_action(void)
{
    BUTTON_PIN = BUTTON_RELEASED;
    data.button_was_pressed = false;
    data.pump.configured_duration_level = 3U;

    handle_button();

    TEST_ASSERT_FALSE(data.button_was_pressed);
    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.configured_duration_level);
}

/**
 * @brief Verify no display pulse in idle state.
 */
void test_idle_no_display_pulse(void)
{
    BUTTON_PIN = BUTTON_RELEASED;
    data.button_was_pressed = false;
    data.send_pulse_to_display = false;

    handle_button();

    TEST_ASSERT_FALSE(data.send_pulse_to_display);
}

/**
 * @brief Verify consecutive idle calls produce no state change.
 */
void test_idle_consecutive_calls_no_change(void)
{
    BUTTON_PIN = BUTTON_RELEASED;
    data.button_was_pressed = false;
    data.pump.configured_duration_level = 5U;

    for (uint8_t i = 0; i < 20U; i++)
    {
        handle_button();
    }

    TEST_ASSERT_FALSE(data.button_was_pressed);
    TEST_ASSERT_EQUAL_UINT8(5U, data.pump.configured_duration_level);
    TEST_ASSERT_FALSE(data.send_pulse_to_display);
}

/* ================================================================
 * GROUP 2: BUTTON PRESS DETECTION
 * ================================================================ */

/**
 * @brief Verify button_was_pressed is set when button is active.
 */
void test_press_sets_flag(void)
{
    BUTTON_PIN = BUTTON_PRESSED;
    data.button_was_pressed = false;

    handle_button();

    TEST_ASSERT_TRUE(data.button_was_pressed);
}

/**
 * @brief Verify button_was_pressed remains true on consecutive
 *        pressed calls (holding the button).
 */
void test_press_held_flag_stays_true(void)
{
    BUTTON_PIN = BUTTON_PRESSED;
    data.button_was_pressed = false;

    for (uint8_t i = 0; i < 50U; i++)
    {
        handle_button();
        TEST_ASSERT_TRUE(data.button_was_pressed);
    }
}

/**
 * @brief Verify update_pump_duration is NOT called while button
 *        is pressed.
 *
 * The action must occur on release only. If update_pump_duration
 * were called during press, configured_duration_level would change.
 */
void test_press_does_not_trigger_update(void)
{
    BUTTON_PIN = BUTTON_PRESSED;
    data.button_was_pressed = false;
    data.pump.configured_duration_level = 4U;

    handle_button();

    TEST_ASSERT_EQUAL_UINT8(4U, data.pump.configured_duration_level);
}

/**
 * @brief Verify no display pulse while button is pressed.
 */
void test_press_no_display_pulse(void)
{
    BUTTON_PIN = BUTTON_PRESSED;
    data.send_pulse_to_display = false;

    handle_button();

    TEST_ASSERT_FALSE(data.send_pulse_to_display);
}

/**
 * @brief Verify holding button for many ticks does not trigger
 *        update_pump_duration.
 *
 * Simulates a long press (1 second = 50 ticks at 20 ms).
 */
void test_long_press_no_update(void)
{
    BUTTON_PIN = BUTTON_PRESSED;
    data.pump.configured_duration_level = 2U;

    for (uint8_t i = 0; i < 50U; i++)
    {
        handle_button();
    }

    TEST_ASSERT_EQUAL_UINT8(2U, data.pump.configured_duration_level);
    TEST_ASSERT_FALSE(data.send_pulse_to_display);
}

/* ================================================================
 * GROUP 3: RELEASE EDGE — ACTION TRIGGER
 * ================================================================ */

/**
 * @brief Verify update_pump_duration is called on release after
 *        press.
 *
 * Observable effect: configured_duration_level increments by 1.
 */
void test_release_after_press_triggers_update(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    /* Press */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();

    /* Release */
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN + 1U,
                            data.pump.configured_duration_level);
}

/**
 * @brief Verify button_was_pressed is cleared after release.
 */
void test_release_clears_flag(void)
{
    data.button_was_pressed = true;
    BUTTON_PIN = BUTTON_RELEASED;

    handle_button();

    TEST_ASSERT_FALSE(data.button_was_pressed);
}

/**
 * @brief Verify display pulse is triggered on release after press.
 */
void test_release_triggers_display_pulse(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.send_pulse_to_display = false;

    /* Press */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();

    /* Release */
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_TRUE(data.send_pulse_to_display);
}

/**
 * @brief Verify exactly one increment per press–release cycle.
 *
 * configured_duration_level must change by exactly 1, not 0 or 2+.
 */
void test_release_increments_exactly_once(void)
{
    data.pump.configured_duration_level = 5U;
    data.pump.remaining_cycle_levels = 0U;

    /* Press */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();

    /* Release */
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_EQUAL_UINT8(6U, data.pump.configured_duration_level);
}

/* ================================================================
 * GROUP 4: RELEASE WITHOUT PRIOR PRESS — NO SPURIOUS TRIGGER
 * ================================================================ */

/**
 * @brief Verify no update when release occurs without prior press.
 */
void test_release_without_press_no_update(void)
{
    BUTTON_PIN = BUTTON_RELEASED;
    data.button_was_pressed = false;
    data.pump.configured_duration_level = 3U;
    data.pump.remaining_cycle_levels = 0U;

    handle_button();

    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.configured_duration_level);
}

/**
 * @brief Verify no display pulse on release without prior press.
 */
void test_release_without_press_no_display_pulse(void)
{
    BUTTON_PIN = BUTTON_RELEASED;
    data.button_was_pressed = false;
    data.send_pulse_to_display = false;

    handle_button();

    TEST_ASSERT_FALSE(data.send_pulse_to_display);
}

/* ================================================================
 * GROUP 5: SINGLE-ACTION GUARANTEE
 * ================================================================ */

/**
 * @brief Verify second consecutive release does not trigger
 *        another update.
 *
 * After a press–release cycle, subsequent released ticks must
 * not re-trigger update_pump_duration.
 */
void test_second_release_no_action(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    /* Press */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();

    /* First release — triggers update */
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();
    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN + 1U,
                            data.pump.configured_duration_level);

    /* Second release — must not trigger again */
    handle_button();
    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN + 1U,
                            data.pump.configured_duration_level);
}

/**
 * @brief Verify many consecutive released ticks after a single
 *        press produce only one update.
 */
void test_many_releases_after_press_single_update(void)
{
    data.pump.configured_duration_level = 4U;
    data.pump.remaining_cycle_levels = 0U;

    /* Press */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();

    /* Release and hold released for many ticks */
    BUTTON_PIN = BUTTON_RELEASED;
    for (uint8_t i = 0; i < 50U; i++)
    {
        handle_button();
    }

    /* Only one increment total */
    TEST_ASSERT_EQUAL_UINT8(5U, data.pump.configured_duration_level);
}

/* ================================================================
 * GROUP 6: MULTIPLE PRESS–RELEASE CYCLES
 * ================================================================ */

/**
 * @brief Verify two distinct press–release cycles produce two
 *        increments.
 */
void test_two_press_release_cycles(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    /* Cycle 1 */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN + 1U,
                            data.pump.configured_duration_level);

    /* Cycle 2 */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN + 2U,
                            data.pump.configured_duration_level);
}

/**
 * @brief Verify full traversal from MIN through MAX and back to MIN
 *        via press–release cycles.
 *
 * Each cycle is: one press tick, one release tick.
 */
void test_full_cycle_traversal_via_button(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    const uint8_t cycles_to_wrap =
        PUMP_DURATION_LEVEL_MAX - PUMP_DURATION_LEVEL_MIN + 1U;

    for (uint8_t i = 0; i < cycles_to_wrap; i++)
    {
        BUTTON_PIN = BUTTON_PRESSED;
        handle_button();
        BUTTON_PIN = BUTTON_RELEASED;
        handle_button();
    }

    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN,
                            data.pump.configured_duration_level);
}

/**
 * @brief Verify every intermediate level during full traversal.
 */
void test_every_level_during_traversal(void)
{
    data.pump.configured_duration_level = PUMP_DURATION_LEVEL_MIN;
    data.pump.remaining_cycle_levels = 0U;

    for (uint8_t expected = PUMP_DURATION_LEVEL_MIN + 1U;
         expected <= PUMP_DURATION_LEVEL_MAX; expected++)
    {
        BUTTON_PIN = BUTTON_PRESSED;
        handle_button();
        BUTTON_PIN = BUTTON_RELEASED;
        handle_button();

        TEST_ASSERT_EQUAL_UINT8(expected,
                                data.pump.configured_duration_level);
    }

    /* Wrap */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_EQUAL_UINT8(PUMP_DURATION_LEVEL_MIN,
                            data.pump.configured_duration_level);
}

/* ================================================================
 * GROUP 7: REALISTIC TIMING — MULTI-TICK PRESS AND RELEASE
 * ================================================================ */

/**
 * @brief Verify correct behavior with a realistic multi-tick
 *        press followed by multi-tick release.
 *
 * Simulates a physical button press held for 100 ms (5 ticks)
 * and released for 200 ms (10 ticks).
 */
void test_realistic_multi_tick_press_release(void)
{
    data.pump.configured_duration_level = 3U;
    data.pump.remaining_cycle_levels = 0U;

    /* 5 ticks pressed */
    BUTTON_PIN = BUTTON_PRESSED;
    for (uint8_t i = 0; i < 5U; i++)
    {
        handle_button();
    }

    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.configured_duration_level);
    TEST_ASSERT_TRUE(data.button_was_pressed);

    /* 10 ticks released */
    BUTTON_PIN = BUTTON_RELEASED;
    for (uint8_t i = 0; i < 10U; i++)
    {
        handle_button();
    }

    /* Exactly one increment */
    TEST_ASSERT_EQUAL_UINT8(4U, data.pump.configured_duration_level);
    TEST_ASSERT_FALSE(data.button_was_pressed);
}

/**
 * @brief Verify two realistic press–release cycles with multi-tick
 *        durations produce exactly two increments.
 */
void test_two_realistic_cycles(void)
{
    data.pump.configured_duration_level = 1U;
    data.pump.remaining_cycle_levels = 0U;

    for (uint8_t cycle = 0; cycle < 2U; cycle++)
    {
        /* Press for 3 ticks */
        BUTTON_PIN = BUTTON_PRESSED;
        for (uint8_t i = 0; i < 3U; i++)
        {
            handle_button();
        }

        /* Release for 5 ticks */
        BUTTON_PIN = BUTTON_RELEASED;
        for (uint8_t i = 0; i < 5U; i++)
        {
            handle_button();
        }
    }

    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.configured_duration_level);
}

/* ================================================================
 * GROUP 8: GUARD PASSTHROUGH — PUMP ACTIVE
 * ================================================================ */

/**
 * @brief Verify that a press–release cycle during pump activity
 *        does NOT change configured_duration_level.
 *
 * handle_button calls update_pump_duration, which is guarded by
 * remaining_cycle_levels == 0. This tests the end-to-end guard.
 */
void test_press_release_during_pump_active_no_update(void)
{
    data.pump.configured_duration_level = 5U;
    data.pump.remaining_cycle_levels = 3U;

    /* Press */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();

    /* Release */
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_EQUAL_UINT8(5U, data.pump.configured_duration_level);
}

/**
 * @brief Verify button_was_pressed is still cleared after release
 *        even when pump is active and update is blocked.
 *
 * Critical: if the flag is not cleared, the next release after
 * the pump stops would trigger a spurious update.
 */
void test_flag_cleared_even_when_update_blocked(void)
{
    data.pump.remaining_cycle_levels = 2U;

    /* Press */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();
    TEST_ASSERT_TRUE(data.button_was_pressed);

    /* Release — update blocked, but flag must clear */
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();
    TEST_ASSERT_FALSE(data.button_was_pressed);
}

/**
 * @brief Verify no display pulse when press–release occurs during
 *        pump activity.
 */
void test_no_display_pulse_during_pump_active(void)
{
    data.pump.remaining_cycle_levels = 1U;
    data.send_pulse_to_display = false;

    /* Press */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();

    /* Release */
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_FALSE(data.send_pulse_to_display);
}

/**
 * @brief Verify that after pump becomes idle, a new press–release
 *        cycle successfully updates the level.
 *
 * Sequence: press–release while active (blocked), pump finishes,
 * press–release while idle (succeeds).
 */
void test_update_succeeds_after_pump_becomes_idle(void)
{
    data.pump.configured_duration_level = 4U;
    data.pump.remaining_cycle_levels = 1U;

    /* Press–release while active — blocked */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_EQUAL_UINT8(4U, data.pump.configured_duration_level);

    /* Pump finishes */
    data.pump.remaining_cycle_levels = 0U;

    /* Press–release while idle — succeeds */
    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_EQUAL_UINT8(5U, data.pump.configured_duration_level);
}

/* ================================================================
 * GROUP 9: STATE ISOLATION
 * ================================================================ */

/**
 * @brief Verify remaining_cycle_levels is not modified by
 *        handle_button during press.
 */
void test_press_does_not_modify_remaining(void)
{
    data.pump.remaining_cycle_levels = 0U;
    BUTTON_PIN = BUTTON_PRESSED;

    handle_button();

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
}

/**
 * @brief Verify level_remaining_seconds is not modified by
 *        handle_button during press.
 */
void test_press_does_not_modify_seconds(void)
{
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;
    BUTTON_PIN = BUTTON_PRESSED;

    handle_button();

    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
}

/**
 * @brief Verify remaining_cycle_levels is not modified by
 *        handle_button on release edge.
 */
void test_release_does_not_modify_remaining(void)
{
    data.pump.remaining_cycle_levels = 0U;

    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_EQUAL_UINT8(0U, data.pump.remaining_cycle_levels);
}

/**
 * @brief Verify level_remaining_seconds is not modified by
 *        handle_button on release edge.
 */
void test_release_does_not_modify_seconds(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.pump.level_remaining_seconds = PUMP_STEP_DURATION_SECONDS;

    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_EQUAL_UINT8(PUMP_STEP_DURATION_SECONDS,
                            data.pump.level_remaining_seconds);
}

/**
 * @brief Verify sending_pulse_to_display is not set to true
 *        by a press–release cycle.
 *
 * update_pump_duration sets send_pulse_to_display and clears
 * sending_pulse_to_display. Verify sending is not set to true.
 */
void test_release_does_not_set_sending_pulse(void)
{
    data.pump.remaining_cycle_levels = 0U;
    data.sending_pulse_to_display = false;

    BUTTON_PIN = BUTTON_PRESSED;
    handle_button();
    BUTTON_PIN = BUTTON_RELEASED;
    handle_button();

    TEST_ASSERT_FALSE(data.sending_pulse_to_display);
}

/* ================================================================
 * GROUP 10: BUTTON ACTIVE LEVEL POLARITY
 * ================================================================ */

/**
 * @brief Verify LOW pin level is interpreted as pressed.
 *
 * Explicit polarity check. Detects inverted button logic.
 */
void test_low_is_pressed(void)
{
    BUTTON_PIN = GPIO_LEVEL_LOW;
    data.button_was_pressed = false;

    handle_button();

    TEST_ASSERT_TRUE(data.button_was_pressed);
}

/**
 * @brief Verify HIGH pin level is interpreted as released.
 *
 * Explicit polarity check. Detects inverted button logic.
 */
void test_high_is_released(void)
{
    BUTTON_PIN = GPIO_LEVEL_HIGH;
    data.button_was_pressed = true;
    data.pump.configured_duration_level = 2U;
    data.pump.remaining_cycle_levels = 0U;

    handle_button();

    /* Release edge detected: update triggered */
    TEST_ASSERT_EQUAL_UINT8(3U, data.pump.configured_duration_level);
    TEST_ASSERT_FALSE(data.button_was_pressed);
}

/* Unity test runner */
int main(void)
{
    UNITY_BEGIN();

    /* Group 1: Idle */
    RUN_TEST(test_idle_no_action);
    RUN_TEST(test_idle_no_display_pulse);
    RUN_TEST(test_idle_consecutive_calls_no_change);

    /* Group 2: Press detection */
    RUN_TEST(test_press_sets_flag);
    RUN_TEST(test_press_held_flag_stays_true);
    RUN_TEST(test_press_does_not_trigger_update);
    RUN_TEST(test_press_no_display_pulse);
    RUN_TEST(test_long_press_no_update);

    /* Group 3: Release edge */
    RUN_TEST(test_release_after_press_triggers_update);
    RUN_TEST(test_release_clears_flag);
    RUN_TEST(test_release_triggers_display_pulse);
    RUN_TEST(test_release_increments_exactly_once);

    /* Group 4: Release without prior press */
    RUN_TEST(test_release_without_press_no_update);
    RUN_TEST(test_release_without_press_no_display_pulse);

    /* Group 5: Single-action guarantee */
    RUN_TEST(test_second_release_no_action);
    RUN_TEST(test_many_releases_after_press_single_update);

    /* Group 6: Multiple cycles */
    RUN_TEST(test_two_press_release_cycles);
    RUN_TEST(test_full_cycle_traversal_via_button);
    RUN_TEST(test_every_level_during_traversal);

    /* Group 7: Realistic timing */
    RUN_TEST(test_realistic_multi_tick_press_release);
    RUN_TEST(test_two_realistic_cycles);

    /* Group 8: Guard passthrough — pump active */
    RUN_TEST(test_press_release_during_pump_active_no_update);
    RUN_TEST(test_flag_cleared_even_when_update_blocked);
    RUN_TEST(test_no_display_pulse_during_pump_active);
    RUN_TEST(test_update_succeeds_after_pump_becomes_idle);

    /* Group 9: State isolation */
    RUN_TEST(test_press_does_not_modify_remaining);
    RUN_TEST(test_press_does_not_modify_seconds);
    RUN_TEST(test_release_does_not_modify_remaining);
    RUN_TEST(test_release_does_not_modify_seconds);
    RUN_TEST(test_release_does_not_set_sending_pulse);

    /* Group 10: Polarity */
    RUN_TEST(test_low_is_pressed);
    RUN_TEST(test_high_is_released);

    return UNITY_END();
}